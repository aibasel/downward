#ifndef PLUGIN_H
#define PLUGIN_H

#include "option_parser_util.h"

#include <memory>
#include <string>

template <class T>
class Plugin {
public:
    Plugin(const std::string &key, typename Registry<T *>::Factory factory) {
        Registry<T *>::instance()->register_object(key, factory);
    }
    ~Plugin() = default;
    Plugin(const Plugin<T> &other) = delete;
};


// TODO: This class will replace Plugin once we no longer need to support raw pointers.
template <class T>
class PluginShared {
public:
    PluginShared(const std::string &key, typename Registry<std::shared_ptr<T> >::Factory factory) {
        Registry<std::shared_ptr<T> >::instance()->register_object(key, factory);
    }
    ~PluginShared() = default;
    PluginShared(const PluginShared<T> &other) = delete;
};

#endif
