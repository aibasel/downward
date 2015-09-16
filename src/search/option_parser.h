#ifndef OPTION_PARSER_H
#define OPTION_PARSER_H

/*
How to add support for a new type NT to the option parser:
In this file, tell the parser how to parse NT:
- Create a template specialization TokenParser<NT> (or overload
  the ">>"-operator for NT).
  See the existing specializations for examples.

If you want classes of type NT to be included in the help output:
In option_parser_util.h:
- Create a template specialization DefaultValueNamer<NT>, or overload
  the "<<"-operator for NT. Not necessary if your type will never have
  a default value.
- (optional) Create a template specialization for TypeNamer<NT> - you
   don't have to do this, but it might print the typename in a
   more readable way.
In option_parser.cc:
- add NT to the functions static void get_help(string k) and
  static void get_full_help()

If NT shall be predefinable:
- See the functions predefine_lmgraph() and predefine_heuristic()
  in option parser.cc for examples.
  You will also need to extend OptionParser::parse_cmd_line(), and
  should add an explanation to OptionParser::usage().
*/

#include "heuristic.h"
#include "option_parser_util.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class AbstractTask;
class LandmarkGraph;
class MergeStrategy;
template<class Entry>
class OpenList;
class OptionParser;
class SearchEngine;
class ShrinkStrategy;
class Labels;

/*
The TokenParser<T> wraps functions to parse supported types T.
 */

template <class T>
class TokenParser {
public:
    //if T has no template specialization,
    //try to parse it directly from the input string
    static inline T parse(OptionParser &p);
};


// int needs a specialization to allow "infinity".
template <>
class TokenParser<int> {
public:
    static inline int parse(OptionParser &p);
};

// double needs a specialization to allow "infinity".
template <>
class TokenParser<double> {
public:
    static inline double parse(OptionParser &p);
};


template <class Entry>
class TokenParser<OpenList<Entry > *> {
public:
    static inline OpenList<Entry> *parse(OptionParser &p);
};

template <>
class TokenParser<Heuristic *> {
public:
    static inline Heuristic *parse(OptionParser &p);
};

template <>
class TokenParser<LandmarkGraph *> {
public:
    static inline LandmarkGraph *parse(OptionParser &p);
};

template <>
class TokenParser<ScalarEvaluator *> {
public:
    static inline ScalarEvaluator *parse(OptionParser &p);
};

template <>
class TokenParser<SearchEngine *> {
public:
    static inline SearchEngine *parse(OptionParser &p);
};

template <>
class TokenParser<Synergy *> {
public:
    static inline Synergy *parse(OptionParser &p);
};


template <>
class TokenParser<ParseTree> {
public:
    static inline ParseTree parse(OptionParser &p);
};

template <>
class TokenParser<std::shared_ptr<MergeStrategy> > {
public:
    static inline std::shared_ptr<MergeStrategy>parse(OptionParser &p);
};

template <>
class TokenParser<std::shared_ptr<ShrinkStrategy> > {
public:
    static inline std::shared_ptr<ShrinkStrategy>parse(OptionParser &p);
};

template <>
class TokenParser<std::shared_ptr<Labels> > {
public:
    static inline std::shared_ptr<Labels>parse(OptionParser &p);
};

template <>
class TokenParser<std::shared_ptr<AbstractTask> > {
public:
    static inline std::shared_ptr<AbstractTask> parse(OptionParser &p);
};

template <class T>
class TokenParser<std::vector<T > > {
public:
    static inline std::vector<T> parse(OptionParser &p);
};



/*The OptionParser stores a parse tree, and a Options.
By calling addArgument, the parse tree is partially parsed,
and the result is added to the Options.
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
    template <class T>
    T start_parsing();

    template<class T>
    void check_bounds(
        const std::string &key, const T &value, const Bounds &bounds);

    /* Add option with default value. Use def_val=NONE for optional
       parameters without default values. */
    template <class T>
    void add_option(
        std::string k, std::string h = "", std::string def_val = "",
        Bounds bounds = Bounds::unlimited());

    void add_enum_option(std::string k,
                         std::vector<std::string > enumeration,
                         std::string h = "", std::string def_val = "",
                         std::vector<std::string> enum_doc = std::vector<std::string>());
    template <class T>
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

    template <class T>
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

template <class T>
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

template <class T>
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

template <class T>
void OptionParser::add_list_option(
    std::string k, std::string h, std::string def_val) {
    add_option<std::vector<T> >(k, h, def_val);
}

//Definitions of TokenParser<T>:
template <class T>
T TokenParser<T>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    std::stringstream str_stream(pt->value);
    T x;
    if ((str_stream >> std::boolalpha >> x).fail()) {
        p.error("could not parse argument " + pt->value + " of type " + TypeNamer<T>::name());
    }
    return x;
}

int TokenParser<int>::parse(OptionParser &p) {
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

double TokenParser<double>::parse(OptionParser &p) {
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
template <class T>
static T *lookup_in_registry(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Registry<T *>::instance()->contains(pt->value)) {
        return Registry<T *>::instance()->get(pt->value) (p);
    }
    p.error(TypeNamer<T *>::name() + " " + pt->value + " not found");
    return 0;
}

// TODO: This function will replace lookup_in_registry() once we no longer need to support raw pointers.
template <class T>
static std::shared_ptr<T> lookup_in_registry_shared(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Registry<std::shared_ptr<T> >::instance()->contains(pt->value)) {
        return Registry<std::shared_ptr<T> >::instance()->get(pt->value) (p);
    }
    p.error(TypeNamer<std::shared_ptr<T> >::name() + " " + pt->value + " not found");
    return 0;
}

template <class T>
static T *lookup_in_predefinitions(OptionParser &p, bool &found) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Predefinitions<T *>::instance()->contains(pt->value)) {
        found = true;
        return Predefinitions<T *>::instance()->get(pt->value);
    }
    found = false;
    return 0;
}

template <class Entry>
OpenList<Entry > *TokenParser<OpenList<Entry > *>::parse(OptionParser &p) {
    return lookup_in_registry<OpenList<Entry > >(p);
}


Heuristic *TokenParser<Heuristic *>::parse(OptionParser &p) {
    bool predefined;
    Heuristic *result = lookup_in_predefinitions<Heuristic>(p, predefined);
    if (predefined)
        return result;
    return lookup_in_registry<Heuristic>(p);
}

LandmarkGraph *TokenParser<LandmarkGraph *>::parse(OptionParser &p) {
    bool predefined;
    LandmarkGraph *result = lookup_in_predefinitions<LandmarkGraph>(p, predefined);
    if (predefined)
        return result;
    return lookup_in_registry<LandmarkGraph>(p);
}

//TODO find a general way to handle parsing of superclasses (see also issue28)
ScalarEvaluator *TokenParser<ScalarEvaluator *>::parse(OptionParser &p) {
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


SearchEngine *TokenParser<SearchEngine *>::parse(OptionParser &p) {
    return lookup_in_registry<SearchEngine>(p);
}

std::shared_ptr<MergeStrategy>TokenParser<std::shared_ptr<MergeStrategy> >::parse(OptionParser &p) {
    return lookup_in_registry_shared<MergeStrategy>(p);
}

std::shared_ptr<ShrinkStrategy>TokenParser<std::shared_ptr<ShrinkStrategy> >::parse(OptionParser &p) {
    return lookup_in_registry_shared<ShrinkStrategy>(p);
}

std::shared_ptr<Labels>TokenParser<std::shared_ptr<Labels> >::parse(OptionParser &p) {
    return lookup_in_registry_shared<Labels>(p);
}


Synergy *TokenParser<Synergy *>::parse(OptionParser &p) {
    return lookup_in_registry<Synergy>(p);
}

std::shared_ptr<AbstractTask> TokenParser<std::shared_ptr<AbstractTask> >::parse(OptionParser &p) {
    return lookup_in_registry_shared<AbstractTask>(p);
}

ParseTree TokenParser<ParseTree>::parse(OptionParser &p) {
    return *p.get_parse_tree();
}

template <class T>
std::vector<T > TokenParser<std::vector<T > >::parse(OptionParser &p) {
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
