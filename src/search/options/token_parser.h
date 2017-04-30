#ifndef OPTIONS_TOKEN_PARSER_H
#define OPTIONS_TOKEN_PARSER_H

#include "predefinitions.h"

#include <sstream>

class Evaluator;
class Heuristic;

namespace options {
class OptionParser;

/*
  TokenParser<T> wraps functions to parse supported types T.
*/
template<typename T>
class TokenParser {
public:
    static inline T parse(OptionParser &parser);
};

/*
  We need to give specializations of the class for the cases we want
  to *partially* specialize, i.e., give a specialization that is still
  templated. For fully specialized cases (e.g. parsing "int"), it is
  not necessary to specialize the class; we just need to specialize
  the method.
*/

template<typename T>
class TokenParser<T *> {
public:
    static inline T *parse(OptionParser &parser);
};

template<typename T>
class TokenParser<std::shared_ptr<T>> {
public:
    static inline std::shared_ptr<T> parse(OptionParser &parser);
};

template<typename T>
class TokenParser<std::vector<T>> {
public:
    static inline std::vector<T> parse(OptionParser &parser);
};

/*
  If T has no template specialization, try to parse it directly from
  the input string. As of this writing, this default implementation is
  used only for string and bool.
*/
template<typename T>
inline T TokenParser<T>::parse(OptionParser &parser) {
    const std::string &value = parser.get_root_value();
    std::istringstream stream(value);
    T x;
    if ((stream >> std::boolalpha >> x).fail()) {
        parser.error("could not parse argument " + value + " of type " + TypeNamer<T>::name());
    }
    return x;
}

// int needs a specialization to allow "infinity".
template<>
inline int TokenParser<int>::parse(OptionParser &parser) {
    const std::string &value = parser.get_root_value();
    if (value == "infinity") {
        return std::numeric_limits<int>::max();
    } else {
        std::istringstream stream(value);
        int x;
        if ((stream >> x).fail()) {
            parser.error("could not parse int argument " + value);
        }
        return x;
    }
}

// double needs a specialization to allow "infinity".
template<>
inline double TokenParser<double>::parse(OptionParser &parser) {
    const std::string &value = parser.get_root_value();
    if (value == "infinity") {
        return std::numeric_limits<double>::infinity();
    } else {
        std::istringstream stream(value);
        double x;
        if ((stream >> x).fail()) {
            parser.error("could not parse double argument " + value);
        }
        return x;
    }
}

// Helper functions for the TokenParser-specializations.

/*
  This function is legacy code. It can go away once all plugins use shared_ptr.
*/
template<typename T>
static T *lookup_in_registry(OptionParser &parser) {
    const std::string &value = parser.get_root_value();
    if (Registry<T *>::instance()->contains(value)) {
        return Registry<T *>::instance()->get(value)(parser);
    }
    parser.error(TypeNamer<T *>::name() + " " + value + " not found");
    return nullptr;
}

// TODO: Rename to lookup_in_registry() once all plugins use shared_ptr.
template<typename T>
static std::shared_ptr<T> lookup_in_registry_shared(OptionParser &parser) {
    const std::string &value = parser.get_root_value();
    if (Registry<std::shared_ptr<T>>::instance()->contains(value)) {
        return Registry<std::shared_ptr<T>>::instance()->get(value)(parser);
    }
    parser.error(TypeNamer<std::shared_ptr<T>>::name() + " " + value + " not found");
    return nullptr;
}

template<typename T>
static T *lookup_in_predefinitions(OptionParser &parser, bool &found) {
    const std::string &value = parser.get_root_value();
    if (Predefinitions<T *>::instance()->contains(value)) {
        found = true;
        return Predefinitions<T *>::instance()->get(value);
    }
    found = false;
    return nullptr;
}

template<typename T>
static std::shared_ptr<T> lookup_in_predefinitions_shared(OptionParser &parser, bool &found) {
    using TPtr = std::shared_ptr<T>;
    const std::string &value = parser.get_root_value();
    if (Predefinitions<TPtr>::instance()->contains(value)) {
        found = true;
        return Predefinitions<TPtr>::instance()->get(value);
    }
    found = false;
    return nullptr;
}


/*
  TODO (post-issue586): We should decide how to handle subclasses more generally.
  See http://issues.fast-downward.org/msg4686 (which is part of that issue).

  For now, we use C-style casts since C++-style casts need complete types.
*/
template<>
inline Evaluator *TokenParser<Evaluator *>::parse(OptionParser &parser) {
    const std::string &value = parser.get_root_value();
    if (Predefinitions<Heuristic *>::instance()->contains(value)) {
        return (Evaluator *)Predefinitions<Heuristic *>::instance()->get(value);
    } else if (Registry<Evaluator *>::instance()->contains(value)) {
        return Registry<Evaluator *>::instance()->get(value)(parser);
    } else if (Registry<Heuristic *>::instance()->contains(value)) {
        return (Evaluator *)Registry<Heuristic *>::instance()->get(value)(parser);
    }
    parser.error("Evaluator " + value + " not found");
    return nullptr;
}

// TODO: The following method can go away once we use shared pointers for all plugins.
template<typename T>
inline T *TokenParser<T *>::parse(OptionParser &parser) {
    bool predefined;
    T *result = lookup_in_predefinitions<T>(parser, predefined);
    if (predefined)
        return result;
    return lookup_in_registry<T>(parser);
}

template<typename T>
inline std::shared_ptr<T> TokenParser<std::shared_ptr<T>>::parse(OptionParser &parser) {
    bool predefined;
    std::shared_ptr<T> result = lookup_in_predefinitions_shared<T>(parser, predefined);
    if (predefined)
        return result;
    return lookup_in_registry_shared<T>(parser);
}

// Needed for iterated search.
template<>
inline ParseTree TokenParser<ParseTree>::parse(OptionParser &parser) {
    return *parser.get_parse_tree();
}

template<typename T>
inline std::vector<T> TokenParser<std::vector<T>>::parse(OptionParser &parser) {
    if (parser.get_parse_tree()->begin()->value != "list") {
        parser.error("expected list");
    }
    std::vector<T> results;
    for (auto tree_it = first_child_of_root(*parser.get_parse_tree());
         tree_it != end_of_roots_children(*parser.get_parse_tree());
         ++tree_it) {
        OptionParser subparser(subtree(*parser.get_parse_tree(), tree_it), parser.dry_run());
        results.push_back(TokenParser<T>::parse(subparser));
    }
    return results;
}
}

#endif
