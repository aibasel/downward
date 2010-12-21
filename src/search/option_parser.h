#ifndef OPTION_PARSER_H_
#define OPTION_PARSER_H_

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>
#include <memory>
#include "boost/any.hpp"
#include "option_parser_util.h"
#include "heuristic.h"

class OptionParser;
class LandmarksGraph;
template<class Entry>
class OpenList;
class SearchEngine;


//Options is just a wrapper for map<string, boost::any>
class Options{
public:
    std::map<std::string, boost::any> storage;
    
    template <class T> void set(std::string key, T value) {
        storage[key] = value;
    }

    template <class T> T get(std::string key) const {
        std::map<std::string, boost::any>::const_iterator it;
        it = storage.find(key);
        return boost::any_cast<T>(it->second);
    }

    template <class T> std::vector<T> get_list(std::string key) const {
        return get<std::vector<T> >(key);
    }

    int get_enum(std::string key) const {
        return get<int>(key);
    }

    bool contains(std::string key) const {
        return storage.find(key) != storage.end();
    }
};


/*The TokenParser<T> wraps functions to parse supported types T. 
To add support for a new type T, it should suffice 
to implement the corresponding TokenParser<T> class
 */

template <class T>
class TokenParser {
public:
    //if T has no template specialization, 
    //try to parse it directly from the input string
    static inline T parse(OptionParser &p);
};

template <> 
class TokenParser<bool> {
public: 
    static inline bool parse(OptionParser &p); 
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
class TokenParser<LandmarksGraph *> {
public:
    static inline LandmarksGraph *parse(OptionParser &p);
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

template <class T>
class TokenParser<std::vector<T > > {
public:
    static inline std::vector<T> parse(OptionParser &p);
};


struct HelpElement {
    HelpElement(std::string k, std::string h, std::string tn);
    void set_default_value(std::string d_v);
    std::string kwd;
    std::string help;
    std::string type_name;
    std::string default_value;
};


/*The OptionParser stores a parse tree, and a Options. 
By calling addArgument, the parse tree is partially parsed, 
and the result is added to the Options.
 */
class OptionParser{
public:
    OptionParser(std::string config, bool dr);
    OptionParser(ParseTree pt, bool dr);
    
    static ParseTree generate_parse_tree(const std::string config); //TODO: move this outside

    //this is where input from the commandline goes:
    static SearchEngine *parse_cmd_line(int argc, const char **argv, bool dr);
    

    //Note: originally the following function was templated predefine<T>,
    //but there is no Synergy<LandmarksGraph>, so I split it up for now.
    static void predefine_heuristic(std::string s, bool dry_run);
    static void predefine_lmgraph(std::string s, bool dry_run);

    //this function initiates parsing of T (the root node of parse_tree
    //will be parsed as T). Usually T=SearchEngine*, ScalarEvaluator* or LandmarksGraph*
    template <class T> T start_parsing();
    
    template <class T> void add_option(std::string k, std::string h="");
       
    //add option with default value
    template <class T> void add_option(
        std::string k, T def_val, std::string h="");


    void add_enum_option(std::string k, 
                         const std::vector<std::string >& enumeration, 
                         std::string def_val = "", std::string h="");

    template <class T>
        void add_list_option(std::string k, 
                             std::vector<T> def_val, std::string h="");

    template <class T> 
        void add_list_option(std::string k, std::string h="");

    void error(std::string msg);
    void warning(std::string msg);
    
    Options parse();
    ParseTree* get_parse_tree();
    void set_parse_tree(const ParseTree& pt); 
    void set_help_mode(bool m);
    
    bool dry_run();
    bool help_mode();

private:
    Options opts;
    ParseTree parse_tree;
    bool dry_run_;
    bool help_mode_;
    std::vector<HelpElement> helpers;
    ParseTree::sibling_iterator next_unparsed_argument;
    std::vector<std::string> valid_keys;
    std::vector<std::string> helpstrings; 
};

//Definitions of OptionParsers template functions:

template <class T> T OptionParser::start_parsing() {
    return TokenParser<T>::parse(*this);
}

template <class T> void OptionParser::add_option(
    std::string k, std::string h) {
    std::cout << "adding option " << k << std::endl;
    if(help_mode_) {
        helpers.push_back(HelpElement(k, h, TypeNamer<T>::name()));
        if(opts.contains(k)){
            helpers.back().default_value = 
                DefaultValueNamer<T>::toStr(opts.get<T>(k));
        }
        return;
    }
    valid_keys.push_back(k);
    T result;
    ParseTree::sibling_iterator arg = next_unparsed_argument;
    //scenario where we have already handled all arguments
    if (arg == parse_tree.end(parse_tree.begin())) {
        if (!opts.contains(k)) {
            error("missing option: " + k);
        } else {
            return; //use default value
        }
    } 
    //handling arguments with explicit keyword:
    if (!arg->key.empty()) {
        //try to find a parameter passed with keyword k
        for (; arg != parse_tree.end(parse_tree.begin()); ++arg) {
            if (arg->key.compare(k) == 0) 
                break;
        }
        if (arg == parse_tree.end(parse_tree.begin())) {
            if (!opts.contains(k)) {
                error("missing option: " + k);
            } else {
                return; //use default value
            }
        } 
    }
    OptionParser subparser(subtree(parse_tree,arg), dry_run());
    result = TokenParser<T>::parse(subparser);
    opts.set(k, result);
    //if we have not reached the keyword parameters yet, 
    //increment the argument position pointer
    if (arg->key.size() == 0)
        ++next_unparsed_argument;     
}

template <class T>
void OptionParser::add_list_option(std::string k, 
                     std::vector<T> def_val, std::string h) {
    opts.set(k, def_val);
    add_list_option<T>(k,h);
}

template <class T> void OptionParser::add_option(
    std::string k, T def_val, std::string h) {
    opts.set(k, def_val);
    add_option<T>(k, h);
}

template <class T> 
void OptionParser::add_list_option(std::string k, std::string h) {
    add_option<std::vector<T> >(k, h);
}

//Definitions of TokenParser<T>:
template <class T>
T TokenParser<T>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    std::stringstream str_stream(pt->value);
    T x;
    if ((str_stream >> x).fail()) {
        p.error("could not parse argument");
    }
    return x;
}



bool TokenParser<bool>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if(pt->value.compare("false") == 0) {
        return false;
    } else {
        return true;
    }
}

template <class Entry>
OpenList<Entry > *TokenParser<OpenList<Entry > *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if(Registry<OpenList<Entry > *>::instance()->contains(pt->value)) {
        return Registry<OpenList<Entry > *>::instance()->get(pt->value)(p);
    }
    p.error("openlist not found");
    return 0;
}


Heuristic *TokenParser<Heuristic *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if(Predefinitions<Heuristic *>::instance()->contains(pt->value)) {
        return Predefinitions<Heuristic *>::instance()->get(pt->value);
    } else if(Registry<Heuristic *>::instance()->contains(pt->value)) {
        return Registry<Heuristic *>::instance()->get(pt->value)(p);
        //look if there's a scalar evaluator registered by this name, 
        //assume that's what's meant (same behaviour as old parser)
    } else if(Registry<ScalarEvaluator *>::instance()->contains(pt->value)) {
        ScalarEvaluator *eval = 
            Registry<ScalarEvaluator *>::instance()->get(pt->value)(p);
        return dynamic_cast<Heuristic *>(eval);
    }

    p.error("heuristic not found");
    return 0;
}

LandmarksGraph *TokenParser<LandmarksGraph *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if(Predefinitions<LandmarksGraph *>::instance()->contains(pt->value)) {
        return Predefinitions<LandmarksGraph *>::instance()->get(pt->value);
    }
    if(Registry<LandmarksGraph *>::instance()->contains(pt->value)) {
        return Registry<LandmarksGraph *>::instance()->get(pt->value)(p);
    }
    p.error("landmarks graph not found");
    return 0;
}

ScalarEvaluator *TokenParser<ScalarEvaluator *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if(Predefinitions<Heuristic *>::instance()->contains(pt->value)) {
        return (ScalarEvaluator *)
            Predefinitions<Heuristic *>::instance()->get(pt->value);
    } else if(Registry<ScalarEvaluator *>::instance()->contains(pt->value)) {
        return Registry<ScalarEvaluator *>::instance()->get(pt->value)(p);
    }
    p.error("scalar evaluator not found");
    return 0;
}


SearchEngine *TokenParser<SearchEngine *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if(Registry<SearchEngine *>::instance()->contains(pt->value)) {
        return Registry<SearchEngine *>::instance()->get(pt->value)(p);
    }
    p.error("search engine not found");
    return 0;
}

Synergy *TokenParser<Synergy *>::parse(OptionParser &p) {
    ParseTree::iterator pt = p.get_parse_tree()->begin();
    if(Registry<Synergy *>::instance()->contains(pt->value)) {
        return Registry<Synergy *>::instance()->get(pt->value)(p);
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
        throw ParseError("list expected here", pt);
    }
    for (ParseTree::iterator pti = p.get_parse_tree()->begin(pt);
         pti != p.get_parse_tree()->end(pt);
         ++pti) {
        ParseTree::sibling_iterator pti_end = pti;
        ++pti_end;
        OptionParser subparser(
            p.get_parse_tree()->subtree(pti, pti_end), p.dry_run());
        results.push_back(
            TokenParser<T>::parse(subparser));
    }
    return results;
}      




#endif /* OPTION_PARSER_H_ */
