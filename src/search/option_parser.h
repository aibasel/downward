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



#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <limits>
#include <algorithm>
#include <map>
#include <memory>
#include "option_parser_util.h"
#include "heuristic.h"

class OptionParser;
class LandmarkGraph;
template<class Entry>
class OpenList;
class SearchEngine;
class ShrinkStrategy;

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


//int needs a specialization to allow "infinity" (=numeric_limits<int>::max())
template <>
class TokenParser<int> {
public:
    static inline int parse(OptionParser &p);
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
class TokenParser<ShrinkStrategy *> {
public:
    static inline ShrinkStrategy *parse(OptionParser &p);
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
public:
    OptionParser(std::string config, bool dr);
    OptionParser(ParseTree pt, bool dr);

    //this is where input from the commandline goes:
    static SearchEngine *parse_cmd_line(int argc, const char **argv, bool dr);
    static std::string usage(std::string progname);

    //this function initiates parsing of T (the root node of parse_tree
    //will be parsed as T). Usually T=SearchEngine*, ScalarEvaluator* or LandmarkGraph*
    template <class T>
    T start_parsing();

    //add option with default value
    //call with mandatory = false to specify an optional parameter without default value
    template <class T>
    void add_option(
        std::string k, std::string def_val, std::string h,
        const OptionFlags &flags = OptionFlags());

    template <class T>
        void add_option(
            std::string k, std::string h="",
            const OptionFlags &flags = OptionFlags());

    void add_enum_option(std::string k,
                         std::vector<std::string > enumeration,
                         std::string def_val = "", std::string h = "",
                         std::vector<std::string> enum_doc = std::vector<std::string>(),
                         const OptionFlags &flags = OptionFlags());
    template <class T>
    void add_list_option(std::string k,
                         std::string def_val, std::string h,
                         const OptionFlags &flags = OptionFlags());

    template <class T>
    void add_list_option(std::string k, std::string h = "",
                         const OptionFlags &flags = OptionFlags());

    void document_values(std::string argument,
                         ValueExplanations value_explanations) const;
    void document_synopsis(std::string name, std::string note) const;
    void document_property(std::string property, std::string note) const;
    void document_language_support(std::string feature, std::string note) const;
    void document_note(std::string name, std::string note) const;

    void error(std::string msg);
    void warning(std::string msg);

    Options parse(); //parse is not the best name for this function. It just does some checks and returns the parsed options Parsing happens before that. Change?
    ParseTree *get_parse_tree();
    void set_parse_tree(const ParseTree &pt);
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
    Options opts;
    ParseTree parse_tree;
    bool dry_run_;
    bool help_mode_;

    ParseTree::sibling_iterator next_unparsed_argument;
    std::vector<std::string> valid_keys;
};

//Definitions of OptionParsers template functions:

template <class T>
T OptionParser::start_parsing() {
    if (help_mode()){
        DocStore::instance()->register_object(parse_tree.begin()->value,
                                              TypeNamer<T>::name());
    }
    return TokenParser<T>::parse(*this);
}

template <class T>
void OptionParser::add_option(
    std::string k,
    std::string default_value, std::string h, const OptionFlags &flags) {
    if (help_mode()) {
        DocStore::instance()->add_arg(parse_tree.begin()->value,
                                      k, h, 
                                      TypeNamer<T>::name(), default_value,
                                      flags.mandatory);
        return;
    }

    valid_keys.push_back(k);
    bool use_default(false);
    ParseTree::sibling_iterator arg = next_unparsed_argument;
    //scenario where we have already handled all arguments
    if (arg == parse_tree.end(parse_tree.begin())) {
        if (default_value.empty() && flags.mandatory) {
            error("missing option: " + k);
        } else {
            use_default = true;
        }
    }
    //handling arguments with explicit keyword:
    if (!use_default && !arg->key.empty()) {//to check if we reached the params supplied with key
        //try to find a parameter passed with keyword k
        for (; arg != parse_tree.end(parse_tree.begin()); ++arg) {
            if (arg->key.compare(k) == 0)
                break;
        }
        if (arg == parse_tree.end(parse_tree.begin())) {
            if (default_value.empty() && flags.mandatory) {
                error("missing option: " + k);
            } else {
                use_default = true;
            }
        }
    }
    if(use_default) {
        OptionParser subparser(default_value, dry_run());
        T result = TokenParser<T>::parse(subparser);
        opts.set<T>(k, result);
    } else {
        OptionParser subparser(subtree(parse_tree, arg), dry_run());
        T result = TokenParser<T>::parse(subparser);
        opts.set<T>(k, result);
        //if we have not reached the keyword parameters yet
        //and did not use the default value,
        //increment the argument position pointer
        if (arg->key.size() == 0)
            ++next_unparsed_argument;
    }
}

template <class T>
void OptionParser::add_list_option(
    std::string k, std::string def_val, std::string h, const OptionFlags &flags) {
    add_option<std::vector<T> >(k, def_val, h, flags);
}

template <class T>
void OptionParser::add_option(
    std::string k, std::string h, const OptionFlags &flags) {
    add_option<T>(k, "", h, flags);
}

template <class T>
void OptionParser::add_list_option(
    std::string k, std::string h, const OptionFlags &flags) {
    add_list_option<T>(k, "", h, flags);
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


template <class Entry>
OpenList<Entry > *TokenParser<OpenList<Entry > *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Registry<OpenList<Entry > *>::instance()->contains(pt->value)) {
        return Registry<OpenList<Entry > *>::instance()->get(pt->value) (p);
    }
    p.error("openlist " + pt->value + " not found");
    return 0;
}


Heuristic *TokenParser<Heuristic *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Predefinitions<Heuristic *>::instance()->contains(pt->value)) {
        return Predefinitions<Heuristic *>::instance()->get(pt->value);
    } else if (Registry<Heuristic *>::instance()->contains(pt->value)) {
        return Registry<Heuristic *>::instance()->get(pt->value) (p);
    }

    p.error("heuristic " + pt->value + " not found");
    return 0;
}

LandmarkGraph *TokenParser<LandmarkGraph *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Predefinitions<LandmarkGraph *>::instance()->contains(pt->value)) {
        return Predefinitions<LandmarkGraph *>::instance()->get(pt->value);
    }
    if (Registry<LandmarkGraph *>::instance()->contains(pt->value)) {
        return Registry<LandmarkGraph *>::instance()->get(pt->value) (p);
    }
    p.error("landmark graph not found");
    return 0;
}

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
    p.error("scalar evaluator " + pt->value + " not found");
    return 0;
}


SearchEngine *TokenParser<SearchEngine *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Registry<SearchEngine *>::instance()->contains(pt->value)) {
        return Registry<SearchEngine *>::instance()->get(pt->value) (p);
    }
    p.error("search engine not found");
    return 0;
}

ShrinkStrategy *TokenParser<ShrinkStrategy *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Registry<ShrinkStrategy *>::instance()->contains(pt->value)) {
        return Registry<ShrinkStrategy *>::instance()->get(pt->value) (p);
    }
    p.error("Shrink strategy not found");
    return 0;
}


Synergy *TokenParser<Synergy *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if (Registry<Synergy *>::instance()->contains(pt->value)) {
        return Registry<Synergy *>::instance()->get(pt->value) (p);
    }
    p.error("synergy not found");
    return 0;
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
