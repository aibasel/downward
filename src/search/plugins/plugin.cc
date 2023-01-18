#include "plugin.h"

using namespace std;

namespace plugins {
PluginGroupPlugin::PluginGroupPlugin(const string &group_id,
                                     const string &doc_title) {
    RawRegistry::instance()->insert_plugin_group_data(group_id, doc_title);
}
}
