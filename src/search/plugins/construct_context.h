#ifndef PLUGINS_CONSTRUCT_CONTEXT_H
#define PLUGINS_CONSTRUCT_CONTEXT_H

#include "options.h"

#include "../utils/language.h"

#include <string>
#include <vector>

namespace plugins {
/*
  ConstructContext specifies how to handle warnings and errors that happen
  during the construction of a feature. For example, if we construct features
  in our parser, errors/warnings should be printed together with where in the
  parse tree they occur. So far, this is our only use case, but we want to keep
  the plugin namespace independent of the parser namespace. In the future,
  features could for example also be created through a Python interface
  directly, without the need of a parser.
*/
class ConstructContext {
public:
    virtual ~ConstructContext() = default;

    NO_RETURN
    virtual void construction_error(const std::string &message) const = 0;
    virtual void construction_warning(const std::string &message) const = 0;

    template<typename T>
    void verify_list_non_empty(const plugins::Options &opts, const std::string &key) const {
        std::vector<T> list = opts.get_list<T>(key);
        if (list.empty()) {
            construction_error("List argument '" + key + "' has to be non-empty.");
        }
    }
};
}

#endif
