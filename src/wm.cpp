#include "wm.hpp"

void Exit::init() {
    this->owner->special = true;

    KeyBinding *exit = new KeyBinding();
    exit->action = [](Context ctx){
        wlc_terminate();
    };

    exit->mod = WLC_BIT_MOD_CTRL | WLC_BIT_MOD_ALT;
    exit->key = XKB_KEY_q;
    core->add_key(exit, true);
}

//TODO: implement refresh using wlc
void Refresh::init() {
    ref.key = XKB_KEY_r;
    ref.type = BindingTypePress;
    ref.mod = WLC_BIT_MOD_CTRL | WLC_BIT_MOD_ALT;
    ref.action = [] (Context ctx) {
        //core->terminate = true;
        //core->mainrestart = true;
    };
    core->add_key(&ref, true);
}

void Close::init() {
    KeyBinding *close = new KeyBinding();
    close->mod = WLC_BIT_MOD_ALT;
    close->type = BindingTypePress;
    close->key = XKB_KEY_F4;
    close->action = [](Context ctx) {
        core->close_window(core->get_active_window());
    };
    core->add_key(close, true);
}

void Focus::init() {
    focus.type = BindingTypePress;

    focus.button = BTN_LEFT;
    focus.mod = 0;
    focus.active = true;

    focus.action = [] (Context ctx){
        auto xev = ctx.xev.xbutton;
        auto w = core->get_view_at_point(xev.x_root, xev.y_root);
        if(w) core->focus_window(w);
    };

    core->add_but(&focus, false);
}
