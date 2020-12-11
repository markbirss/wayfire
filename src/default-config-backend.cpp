#include <vector>
#include "wayfire/debug.hpp"
#include <string>
#include <wayfire/config/file.hpp>
#include <wayfire/config-backend.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/core.hpp>

#include <sys/inotify.h>
#include <unistd.h>

#define INOT_BUF_SIZE (1024 * sizeof(inotify_event))
static char buf[INOT_BUF_SIZE];


static std::string config_dir, config_file;
wf::config::config_manager_t *cfg_manager;


static void reload_config(int fd)
{
    wf::config::load_configuration_options_from_file(*cfg_manager, config_file);
    inotify_add_watch(fd, config_dir.c_str(), IN_CREATE);
    inotify_add_watch(fd, config_file.c_str(), IN_MODIFY);
}

static int handle_config_updated(int fd, uint32_t mask, void *data)
{
    LOGD("Reloading configuration file");

    /* read, but don't use */
    read(fd, buf, INOT_BUF_SIZE);
    reload_config(fd);

    wf::get_core().emit_signal("reload-config", nullptr);

    return 0;
}

namespace wf
{
class dynamic_ini_config_t : public wf::config_backend_t
{
  public:
    void init(wl_display *display, config::config_manager_t& config,
        const std::string& cfg_file) override
    {
        cfg_manager = &config;

        if (cfg_file.empty())
        {
            config_dir = nonull(getenv("XDG_CONFIG_HOME"));
            if (!config_dir.compare("nil"))
            {
                config_dir = std::string(nonull(getenv("HOME"))) + "/.config";
            }

            config_file = config_dir + "/wayfire.ini";
        } else
        {
            config_file = cfg_file;
        }

        std::vector<std::string> xmldirs;
        if (char *plugin_xml_path = getenv("WAYFIRE_PLUGIN_XML_PATH"))
        {
            std::stringstream ss(plugin_xml_path);
            std::string entry;
            while (std::getline(ss, entry, ':'))
            {
                xmldirs.push_back(entry);
            }
        }

        xmldirs.push_back(PLUGIN_XML_DIR);

        LOGI("Using config file: ", config_file.c_str());
        config = wf::config::build_configuration(
            xmldirs, SYSCONFDIR "/wayfire/defaults.ini", config_file);

        int inotify_fd = inotify_init1(IN_CLOEXEC);
        reload_config(inotify_fd);

        wl_event_loop_add_fd(wl_display_get_event_loop(display),
            inotify_fd, WL_EVENT_READABLE, handle_config_updated, NULL);
    }
};
}

DECLARE_WAYFIRE_CONFIG_BACKEND(wf::dynamic_ini_config_t);
