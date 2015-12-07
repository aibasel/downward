#ifndef OPTION_PARSER_H
#define OPTION_PARSER_H

#include "option_parser_util.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class Heuristic;
class OptionParser;
class ScalarEvaluator;
class SearchEngine;

/*
The TokenParser<T> wraps functions to parse supported types T.
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

/*
  The OptionParser stores a parse tree and an Options object. By
  calling addArgument, the parse tree is partially parsed, and the
  result is added to the Options.
*/
class OptionParser {
    static int parse_int_arg(const std::string &name, const std::string &value);
    static SearchEngine *parse_cmd_line_aux(
        const std::vector<std::string> &args, bool dry_run);
public:
    OptionParser(const OptionParser &other) = delete;
    OptionParser &operator=(const OptionParser &other) = delete;
    OptionParser(std::string config, bool dr);
    OptionParser(ParseTree pt, bool dr);
    ~OptionParser() = default;

    static const std::string NONE;

    //this is where input from the commandline goes:
    static SearchEngine *parse_cmd_line(
        int argc, const char **argv, bool dr, bool is_unit_cost);

    static std::string usage(std::string progname);

    //this function initiates parsing of T (the root node of parse_tree
    //will be parsed as T). Usually T=SearchEngine*, ScalarEvaluator* or LandmarkGraph*
    template<typename T>
    T start_parsing();

    template<class T>
    void check_bounds(
        const std::string &key, const T &value, const Bounds &bounds);

    /* Add option with default value. Use def_val=NONE for optional
       parameters without default values. */
    template<typename T>
    void add_option(
        std::string k, std::string h = "", std::string def_val = "",
        Bounds bounds = Bounds::unlimited());

    void add_enum_option(std::string k,
                         std::vector<std::string > enumeration,
                         std::string h = "", std::string def_val = "",
                         std::vector<std::string> enum_doc = std::vector<std::string>());
    template<typename T>
    void add_list_option(std::string k,
                         std::string h = "", std::string def_val = "");

    bool is_valid_option(const std::string &k) const;

    void document_values(std::string argument,
                         ValueExplanations value_explanations) const;
    void document_synopsis(std::string name, std::string note) const;
    void document_property(std::string property, std::string note) const;
    void document_language_support(std::string feature, std::string note) const;
    void document_note(std::string name, std::string note, bool long_text = false) const;
    void document_hide() const;

    static void static_error(std::string msg);
    void error(std::string msg);
    void warning(std::string msg);

    Options parse(); //parse is not the best name for this function. It just does some checks and returns the parsed options Parsing happens before that. Change?
    const ParseTree *get_parse_tree();
    void set_help_mode(bool m);

    bool dry_run() const;
    bool help_mode() const;

    template<typename T>
    static std::string to_str(T in) {
        std::ostringstream out;
        out << std::boolalpha << in;
        return out.str();
    }

private:
    std::string unparsed_config;
    Options opts;
    const ParseTree parse_tree;
    bool dry_run_;
    bool help_mode_;

    ParseTree::sibling_iterator next_unparsed_argument;
    std::vector<std::string> valid_keys;

    void set_unparsed_config();
};

//Definitions of OptionParsers template functions:

template<typename T>
T OptionParser::start_parsing() {
    if (help_mode()) {
        DocStore::instance()->register_object(parse_tree.begin()->value,
                                              TypeNamer<T>::name());
    }
    return TokenParser<T>::parse(*this);
}

template<class T>
void OptionParser::check_bounds(
    const std::string &, const T &, const Bounds &) {
}

template<>
void OptionParser::check_bounds<int>(
    const std::string &key, const int &value, const Bounds &bounds);

template<>
void OptionParser::check_bounds<double>(
    const std::string &key, const double &value, const Bounds &bounds);

template<typename T>
void OptionParser::add_option(
    std::string k, std::string h, std::string default_value, Bounds bounds) {
    if (help_mode()) {
        DocStore::instance()->add_arg(parse_tree.begin()->value,
                                      k, h,
                                      TypeNamer<T>::name(), default_value,
                                      bounds);
        return;
    }
    valid_keys.push_back(k);
    bool use_default(false);
    ParseTree::sibling_iterator arg = next_unparsed_argument;
    //scenario where we have already handled all arguments
    if (arg == parse_tree.end(parse_tree.begin())) {
        if (default_value.empty()) {
            error("missing option: " + k);
        } else if (default_value == NONE) {
            return;
        } else {
            use_default = true;
        }
    } else {
        //handling arguments with explicit keyword:
        if (!arg->key.empty()) { //to check if we reached the params supplied with key
            //try to find a parameter passed with keyword k
            for (; arg != parse_tree.end(parse_tree.begin()); ++arg) {
                if (arg->key.compare(k) == 0)
                    break;
            }
            if (arg == parse_tree.end(parse_tree.begin())) {
                if (default_value.empty()) {
                    error("missing option: " + k);
                } else if (default_value == NONE) {
                    return;
                } else {
                    use_default = true;
                }
            }
        }
    }
    std::unique_ptr<OptionParser> subparser(
        use_default ?
        new OptionParser(default_value, dry_run()) :
        new OptionParser(subtree(parse_tree, arg), dry_run()));
    T result = TokenParser<T>::parse(*subparser);
    check_bounds<T>(k, result, bounds);
    opts.set<T>(k, result);
    //if we have not reached the keyword parameters yet
    //and did not use the default value,
    //increment the argument position pointer
    if (!use_default && arg->key.empty()) {
        ++next_unparsed_argument;
    }
}

template<typename T>
void OptionParser::add_list_option(
    std::string k, std::string h, std::string def_val) {
    add_option<std::vector<T>>(k, h, def_val);
}

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

#endif
