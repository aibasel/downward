#ifndef OPTIONS_TOKEN_PARSER_H
#define OPTIONS_TOKEN_PARSER_H

#include "predefinitions.h"

class Heuristic;
class ScalarEvaluator;

namespace options {
class OptionParser;

/*
  TokenParser<T> wraps functions to parse supported types T.
*/

template<typename T>
class TokenParser {
public:
    static inline T parse(OptionParser &p);
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
    static inline T *parse(OptionParser &p);
};

template<typename T>
class TokenParser<std::shared_ptr<T>> {
public:
    static inline std::shared_ptr<T> parse(OptionParser &p);
};

template<typename T>
class TokenParser<std::vector<T>> {
public:
    static inline std::vector<T> parse(OptionParser &p);
};

//Definitions of TokenParser<T>:

/*
  If T has no template specialization, try to parse it directly from
  the input string. As of this writing, this default implementation is
  used only for string and bool.
*/
template<typename T>
inline T TokenParser<T>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    std::stringstream str_stream(pt->value);
    T x;
    if ((str_stream >> std::boolalpha >> x).fail()) {
        p.error("could not parse argument " + pt->value + " of type " + TypeNamer<T>::name());
    }
    return x;
}

// int needs a specialization to allow "infinity".
template<>
inline int TokenParser<int>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (pt->value.compare("infinity") == 0) {
        return std::numeric_limits<int>::max();
    } else {
        std::stringstream str_stream(pt->value);
        int x;
        if ((str_stream >> x).fail()) {
            p.error("could not parse int argument " + pt->value);
        }
        return x;
    }
}

// double needs a specialization to allow "infinity".
template<>
inline double TokenParser<double>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (pt->value.compare("infinity") == 0) {
        return std::numeric_limits<double>::infinity();
    } else {
        std::stringstream str_stream(pt->value);
        double x;
        if ((str_stream >> x).fail()) {
            p.error("could not parse double argument " + pt->value);
        }
        return x;
    }
}

//helper functions for the TokenParser-specializations
template<typename T>
static T *lookup_in_registry(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Registry<T *>::instance()->contains(pt->value)) {
        return Registry<T *>::instance()->get(pt->value) (p);
    }
    p.error(TypeNamer<T *>::name() + " " + pt->value + " not found");
    return 0;
}

// TODO: This function will replace lookup_in_registry() once we no longer need to support raw pointers.
template<typename T>
static std::shared_ptr<T> lookup_in_registry_shared(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Registry<std::shared_ptr<T>>::instance()->contains(pt->value)) {
        return Registry<std::shared_ptr<T>>::instance()->get(pt->value) (p);
    }
    p.error(TypeNamer<std::shared_ptr<T>>::name() + " " + pt->value + " not found");
    return 0;
}

template<typename T>
static T *lookup_in_predefinitions(OptionParser &p, bool &found) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Predefinitions<T *>::instance()->contains(pt->value)) {
        found = true;
        return Predefinitions<T *>::instance()->get(pt->value);
    }
    found = false;
    return 0;
}

template<typename T>
static std::shared_ptr<T> lookup_in_predefinitions_shared(OptionParser &p, bool &found) {
    using TPtr = std::shared_ptr<T>;
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Predefinitions<TPtr>::instance()->contains(pt->value)) {
        found = true;
        return Predefinitions<TPtr>::instance()->get(pt->value);
    }
    found = false;
    return nullptr;
}


/*
  TODO (post-issue586): We should decide how to handle subclasses more generally.
  See http://issues.fast-downward.org/msg4686 (which is part of that issue).
*/
template<>
inline ScalarEvaluator *TokenParser<ScalarEvaluator *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Predefinitions<Heuristic *>::instance()->contains(pt->value)) {
        return (ScalarEvaluator *)
               Predefinitions<Heuristic *>::instance()->get(pt->value);
    } else if (Registry<ScalarEvaluator *>::instance()->contains(pt->value)) {
        return Registry<ScalarEvaluator *>::instance()->get(pt->value) (p);
    } else if (Registry<Heuristic *>::instance()->contains(pt->value)) {
        return (ScalarEvaluator *)
               Registry<Heuristic *>::instance()->get(pt->value) (p);
    }
    p.error("ScalarEvaluator " + pt->value + " not found");
    return 0;
}

// TODO: The following method can go away once we use shared pointers for all plugins.
template<typename T>
inline T *TokenParser<T *>::parse(OptionParser &p) {
    bool predefined;
    T *result = lookup_in_predefinitions<T>(p, predefined);
    if (predefined)
        return result;
    return lookup_in_registry<T>(p);
}

template<typename T>
inline std::shared_ptr<T> TokenParser<std::shared_ptr<T>>::parse(OptionParser &p) {
    bool predefined;
    std::shared_ptr<T> result = lookup_in_predefinitions_shared<T>(p, predefined);
    if (predefined)
        return result;
    return lookup_in_registry_shared<T>(p);
}

template<>
inline ParseTree TokenParser<ParseTree>::parse(OptionParser &p) {
    return *p.get_parse_tree();
}

template<typename T>
inline std::vector<T> TokenParser<std::vector<T>>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    std::vector<T> results;
    if (pt->value.compare("list") != 0) {
        //try to parse the next token as list of length 1 given without brackets
        results.push_back(TokenParser<T>::parse(p));
    } else {
        for (ParseTree::sibling_iterator pti =
                 first_child_of_root(*p.get_parse_tree());
             pti != end_of_roots_children(*p.get_parse_tree());
             ++pti) {
            OptionParser subparser(subtree(*p.get_parse_tree(), pti), p.dry_run());
            results.push_back(
                TokenParser<T>::parse(subparser));
        }
    }
    return results;
}
}

#endif
