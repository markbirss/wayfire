#include <wayfire/plugin.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/view-transform.hpp>
#include <wayfire/view.hpp>
#include <wayfire/workspace-manager.hpp>

/*
 * This plugin provides abilities to switch between views.
 * It works similarly to the alt-esc binding in Windows or GNOME
 */

class wayfire_fast_switcher : public wf::plugin_interface_t
{
    wf::option_wrapper_t<wf::keybinding_t> activate_key{"fast-switcher/activate"};
    std::vector<wayfire_view> views; // all views on current viewport
    size_t current_view_index = 0;
    bool active = false;

  public:
    void init() override
    {
        grab_interface->name = "fast-switcher";
        grab_interface->capabilities = wf::CAPABILITY_MANAGE_COMPOSITOR;

        output->add_key(activate_key, &fast_switch);

        using namespace std::placeholders;
        grab_interface->callbacks.keyboard.mod =
            std::bind(std::mem_fn(&wayfire_fast_switcher::handle_mod),
                this, _1, _2);

        grab_interface->callbacks.cancel = [=] () { switch_terminate(); };
    }

    void handle_mod(uint32_t mod, uint32_t st)
    {
        bool mod_released =
            (mod == ((wf::keybinding_t)activate_key).get_modifiers() &&
                st == WLR_KEY_RELEASED);

        if (mod_released)
        {
            switch_terminate();
        }
    }

    void view_chosen(int i, bool reorder_only)
    {
        /* No view available */
        if (!((0 <= i) && (i < (int)views.size())))
        {
            return;
        }

        set_view_alpha(views[i], 1.0);
        for (int i = (int)views.size() - 1; i >= 0; i--)
        {
            output->workspace->bring_to_front(views[i]);
        }

        if (reorder_only)
        {
            output->workspace->bring_to_front(views[i]);
        } else
        {
            output->focus_view(views[i], true);
        }
    }

    wf::signal_callback_t cleanup_view = [=] (wf::signal_data_t *data)
    {
        auto view = get_signaled_view(data);

        size_t i = 0;
        for (; i < views.size() && views[i] != view; i++)
        {}

        if (i == views.size())
        {
            return;
        }

        views.erase(views.begin() + i);

        if (views.empty())
        {
            switch_terminate();

            return;
        }

        if (i <= current_view_index)
        {
            current_view_index =
                (current_view_index + views.size() - 1) % views.size();
            view_chosen(current_view_index, true);
        }
    };

    const std::string transformer_name = "fast-switcher";

    void set_view_alpha(wayfire_view view, float alpha)
    {
        if (!view->get_transformer(transformer_name))
        {
            view->add_transformer(
                std::make_unique<wf::view_2D>(view), transformer_name);
        }

        auto tr = dynamic_cast<wf::view_2D*>(
            view->get_transformer(transformer_name).get());
        tr->alpha = alpha;
        view->damage();
    }

    void update_views()
    {
        views = output->workspace->get_views_on_workspace(
            output->workspace->get_current_workspace(), wf::WM_LAYERS);

        std::sort(views.begin(), views.end(), [] (wayfire_view& a, wayfire_view& b)
        {
            return a->last_focus_timestamp > b->last_focus_timestamp;
        });
    }

    wf::key_callback fast_switch = [=] (auto)
    {
        if (active)
        {
            switch_next();

            return true;
        }

        if (!output->activate_plugin(grab_interface))
        {
            return false;
        }

        update_views();

        if (views.size() < 1)
        {
            output->deactivate_plugin(grab_interface);

            return false;
        }

        current_view_index = 0;
        active = true;

        /* Set all to semi-transparent */
        for (auto view : views)
        {
            set_view_alpha(view, 0.7);
        }

        grab_interface->grab();
        switch_next();

        output->connect_signal("view-disappeared", &cleanup_view);

        return true;
    };

    void switch_terminate()
    {
        for (auto view : views)
        {
            view->pop_transformer(transformer_name);
        }

        grab_interface->ungrab();
        output->deactivate_plugin(grab_interface);
        view_chosen(current_view_index, false);
        active = false;

        output->disconnect_signal("view-disappeared", &cleanup_view);
    }

    void switch_next()
    {
#define index current_view_index
        set_view_alpha(views[index], 0.7);
        index = (index + 1) % views.size();
#undef index
        view_chosen(current_view_index, true);
    }

    void fini() override
    {
        if (active)
        {
            switch_terminate();
        }

        output->rem_binding(&fast_switch);
    }
};

DECLARE_WAYFIRE_PLUGIN(wayfire_fast_switcher);
