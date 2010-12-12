#ifndef OPTION_PARSER_H_
#define OPTION_PARSER_H_

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>
#include "boost/any.hpp"
#include "open_lists/open_list.h"

class OptionParser;

//this class is responsible for holding parsed input as a tree of strings
class ParseTree{
public:
    ParseTree();
    ParseTree(ParseTree* parent, std::string value = "", std::string key = "");

    std::string value;
    std::string key;

    std::vector<ParseTree>* get_children();
    void add_child(std::string value = "", std::string key = "");
    ParseTree* last_child();
    ParseTree* find_child(std::string key);
    ParseTree* get_parent() const;
    bool is_root() const;

    
    bool operator == (ParseTree& pt);
    bool operator != (ParseTree& pt);

private:
    std::vector<ParseTree> children_;
    ParseTree* parent_;
};

class ParseError{
public:
    ParseError(std::string _msg, ParseTree pt = ParseTree());
    
    std::string msg;
    ParseTree parse_tree;
};


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
        return boost::any_cast<T>(*it);
    }

    template <class T> vector<T> get_list(std::string key) const {
        return get<vector<T> >(key);
    }

    bool contains(std::string key) {
        return storage.find(key) != storage.end();
    }
};

//a registry<T> maps a string (e.g. "ff") to a T-factory
template <class T>
class Registry {
public:
    typedef T* (*Factory)(OptionParser&);
    static Registry<T>* instance()
    {
        if (!instance_) {
            instance_ = new Registry<T>();
        }
        return instance_;
    }
            
    void register_object(std::string k, Factory);
    bool contains(std::string k);
    Factory get(std::string k);
private:
    Registry(){};
    static Registry<T>* instance_;
    std::map<std::string, Factory> registered;
};

template <class T> Registry<T>* Registry<T>::instance_ = 0;

//pseudoclass Synergy for the registry
class Synergy {
};

template <>
class Registry<Synergy> {
public:
    typedef void (*Factory)(OptionParser&);
    static Registry<Synergy>* instance()
    {
        if (!instance_) {
            instance_ = new Registry<Synergy>();
        }
        return instance_;
    }
            
    void register_object(std::string k, Factory);
    bool contains(std::string k);
    Factory get(std::string k);
private:
    Registry(){};
    static Registry<Synergy>* instance_;
    std::map<std::string, Factory> registered;
};

Registry<Synergy>* Registry<Synergy>::instance_ = 0;


//Predefinitions<T> maps strings to already created Heuristics/LandmarksGraphs
template <class T>
class Predefinitions {
public:
    static Predefinitions<T>* instance()
    {
        if (!instance_) {
            instance_ = new Predefinitions<T>();
        }
        return instance_;
    }

    void predefine(std::string k, T*);
    bool contains(std::string k);
    T* get(std::string k);
private:
    Predefinitions<T>(){};
    static Predefinitions<T>* instance_;
    std::map<std::string, T*> predefined;
};

template <class T> Predefinitions<T>* Predefinitions<T>::instance_ = 0;

struct ParserState {
    Options opts;
    ParseTree* parse_tree;
    bool dry_run;
    std::string help;
    std::vector<ParseTree>::iterator next_unparsed_argument;
    std::vector<std::string> valid_keys;
    std::vector<std::string> helpstrings;
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
    static T parse(ParserState ps) {
        ParseTree *pt = ps.parse_tree;
        std::stringstream str_stream(pt->value);
        T x;
        if ((str_stream >> x).fail()) {
            throw ParseError(pt);
        }
        return x;
    }
};


template <class Entry>
class TokenParser<OpenList<Entry > > {
public:
    static OpenList<Entry> parse(ParserState ps) {
        if(Registry<OpenList<Entry > >::instance()->contains()){}
    }
};

template <>
class TokenParser<Heuristic *> {
public:
    static Heuristic* parse(ParserState ps) {
        ParseTree *pt = ps.parse_tree;
        if(Predefinitions<Heuristic>::instance()->contains(pt->value)) {
            return Predefinitions<Heuristic>::instance()->get(pt->value);
        }
        if(Registry<Heuristic>::instance()->contains(pt->value)) {
            return Registry<Heuristic>::instance()->get(pt->value)(OptionParser(ps));
        }
        OptionParser(ps).error("heuristic not found");
    }
}

template <> 
class TokenParser<bool> {
public: 
    static bool parse(ParserState ps) {
        ParseTree *pt = ps.parse_tree;
        if(pt->value.compare("false") == 0) {
            return false;
        } else {
            return true;
        }
    }
};


template <class S>
class TokenParser<std::vector<S > > {
public:
    static std::vector<S> parse(ParserState ps) {
        ParseTree *pt = ps.parse_tree;
        std::vector<S> results;
        if (pt->value.compare("list") != 0) {
            throw ParseError("list expected here", pt);
        }
        for (size_t i(0); i != pt->get_children()->size(); ++i) {
            ParserState substate = ps;
            substate.parse_tree = &pt->get_children()->at(i);
            results.push_back(
                TokenParser<S>::parse(substate));
        }
        return results;
    }      
};



/*The OptionParser stores a parse tree, and a Options. 
By calling addArgument, the parse tree is partially parsed, 
and the result is added to the Options.
 */
class OptionParser{
public:
    OptionParser(std::string config);
    OptionParser(ParseTree pt);
    OptionParser(ParseState ps);
    
    static ParseTree generate_parse_tree(const std::string config);

    //this is where all parsing starts:
    static void parse_cmd_line(const char **argv, bool dry_run);

    template <class T> void add_option(
        std::string k, std::string h="") {
        state.helpstrings.push_back(h);
        state.valid_keys.push_back(k);
        T result;
        if (state.next_unparsed_argument 
            >= state.parse_tree->get_children()->end()) {
            if (state.opts.contains(k)){
                return; //use default value
            } else {
                //throw error: not enough arguments supplied
            }
        }
        ParseTree* arg = &*state.next_unparsed_argument;
        if (arg->key.size() > 0) {
            arg = state.parse_tree->find_child(k);
            if (!arg) {
                if (!state.opts.contains(k)) {
                    //throw error
                } else {
                    return; //use default value
                }
            }
        } 
       
        ParserState substate = state;
        state.parse_tree = arg;
        result = TokenParser<T>::parse(substate);
        state.opts.set(k, result);
        //if we have not reached the keyword parameters yet, 
        //increment the argument position pointer
        if (arg->key.size() == 0)
            ++state.next_unparsed_argument;        
    }

    //add option with default value
    template <class T> void add_option(
        std::string k, T def_val, std::string h="") {
        state.opts.set(k, def_val);
        add_option<T>(k, h);
    }


    void add_enum_option(std::string k, 
                         const std::vector<std::string >& enumeration, 
                         std::string def_val = "", std::string h="");

    template <class T>
        void add_list_option(std::string k, 
                             vector<T> def_val, std::string h="") {
        state.opts.set(k, def_val);
        add_list_option<T>(k,h);
    }

    template <class T> 
        void add_list_option(std::string k, std::string h="") {
        add_option<vector<T> >(k, h);
    }

    void error(std::string msg);
    
    Options parse();
    ParseTree* get_parse_tree();
    
    bool dry_run();

private: 
    ParserState state;
};

#endif /* OPTION_PARSER_H_ */
