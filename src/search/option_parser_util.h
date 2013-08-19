#ifndef OPTION_PARSER_UTIL_H
#define OPTION_PARSER_UTIL_H

#include "merge_and_shrink/shrink_strategy.h"
#include "utilities.h"

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <ios>
#include <tree.hh>
#include <tree_util.hh>
#include <boost/any.hpp>


class LandmarkGraph;
class Heuristic;
class ScalarEvaluator;
class Synergy;
class SearchEngine;
class OptionParser;
template<class Entry>
class OpenList;


struct ParseNode {
    ParseNode()
        : value(""),
          key("") {
    }

    ParseNode(std::string val, std::string k = "")
        : value(val),
          key(k) {
    }
    std::string value;
    std::string key;

    friend std::ostream & operator<<(std::ostream &out, const ParseNode &pn) {
        if (pn.key.compare("") != 0)
            out << pn.key << " = ";
        out << pn.value;
        return out;
    }
};

typedef tree<ParseNode> ParseTree;

struct ParseError {
    ParseError(std::string m, ParseTree pt);
    ParseError(std::string m, ParseTree pt, std::string correct_substring);

    std::string msg;
    ParseTree parse_tree;
    std::string substr;

    friend std::ostream &operator<<(std::ostream &out, const ParseError &pe) {
        out << "Parse Error: " << std::endl
        << pe.msg << " at: " << std::endl;
        kptree::print_tree_bracketed<ParseNode>(pe.parse_tree, out);
        if(pe.substr.size() > 0) {
            out << " (cannot continue parsing after \"" << pe.substr << "\")" << std::endl;
        }
        return out;
    }
};


//a registry<T> maps a string to a T-factory
template <class T>
class Registry {
public:
    typedef T (*Factory)(OptionParser &);
    static Registry<T> *instance() {
        if (!instance_) {
            instance_ = new Registry<T>();
        }
        return instance_;
    }

    void register_object(std::string k, Factory f) {
        transform(k.begin(), k.end(), k.begin(), ::tolower); //k to lowercase
        registered[k] = f;
    }

    bool contains(std::string k) {
        return registered.find(k) != registered.end();
    }

    Factory get(std::string k) {
        return registered[k];
    }

    std::vector<std::string> get_keys() {
        std::vector<std::string> keys;
        for (typename std::map<std::string, Factory>::iterator it =
                 registered.begin();
             it != registered.end(); ++it) {
            keys.push_back(it->first);
        }
        return keys;
    }

private:
    Registry() {}
    static Registry<T> *instance_;
    std::map<std::string, Factory> registered;
};

template <class T>
Registry<T> *Registry<T>::instance_ = 0;




//Predefinitions<T> maps strings to pointers to
//already created Heuristics/LandmarkGraphs
template <class T>
class Predefinitions {
public:
    static Predefinitions<T> *instance() {
        if (!instance_) {
            instance_ = new Predefinitions<T>();
        }
        return instance_;
    }

    void predefine(std::string k, T obj) {
        transform(k.begin(), k.end(), k.begin(), ::tolower);
        predefined[k] = obj;
    }

    bool contains(std::string k) {
        return predefined.find(k) != predefined.end();
    }

    T get(std::string k) {
        return predefined[k];
    }

private:
    Predefinitions<T>() {}
    static Predefinitions<T> *instance_;
    std::map<std::string, T> predefined;
};

template <class T>
Predefinitions<T> *Predefinitions<T>::instance_ = 0;



struct Synergy {
    std::vector<Heuristic *> heuristics;
};

//TypeNamer prints out names of types.
//There's something built in for this (typeid().name()), but the output is not always very readable

template <class T>
struct TypeNamer {
    static std::string name() {
        return typeid(T()).name();
    }
};

template <>
struct TypeNamer<int> {
    static std::string name() {
        return "int";
    }
};

template <>
struct TypeNamer<bool> {
    static std::string name() {
        return "bool";
    }
};

template <>
struct TypeNamer<double> {
    static std::string name() {
        return "double";
    }
};

template <>
struct TypeNamer<std::string> {
    static std::string name() {
        return "string";
    }
};

template <>
struct TypeNamer<Heuristic *> {
    static std::string name() {
        return "heuristic";
    }
};

template <>
struct TypeNamer<LandmarkGraph *> {
    static std::string name() {
        return "landmarks graph";
    }
};

template <>
struct TypeNamer<ScalarEvaluator *> {
    static std::string name() {
        return "scalar evaluator";
    }
};

template <>
struct TypeNamer<SearchEngine *> {
    static std::string name() {
        return "search engine";
    }
};

template <>
struct TypeNamer<ParseTree> {
    static std::string name() {
        return "parse tree (this just means the input is parsed at a later point. The real type is probably a search engine.)";
    }
};

template <>
struct TypeNamer<Synergy *> {
    static std::string name() {
        return "synergy";
    }
};

template <>
struct TypeNamer<ShrinkStrategy *> {
    static std::string name() {
        return "shrink strategy";
    }
};

template <class Entry>
struct TypeNamer<OpenList<Entry> *> {
    static std::string name() {
        return "openlist";
    }
};

template <class T>
struct TypeNamer<std::vector<T> > {
    static std::string name() {
        return "list of " + TypeNamer<T>::name();
    }
};

//DefaultValueNamer is for printing default values.
//Maybe a better solution would be to implement "<<" for everything that's needed. Don't know.

template <class T>
struct DefaultValueNamer {
    static std::string to_str(T val) {
        std::ostringstream strs;
        strs << std::boolalpha << val;
        return strs.str();
    }
};

template <>
struct DefaultValueNamer<ParseTree> {
    static std::string to_str(ParseTree val) {
        return "implement default value naming for parse trees!"
               + val.begin()->value;
    }
};

template <>
struct DefaultValueNamer<ShrinkStrategy *> {
    static std::string to_str(ShrinkStrategy *s) {
        return s->name();
    }
};

template <class T>
struct DefaultValueNamer<std::vector<T> > {
    static std::string to_str(std::vector<T> val) {
        std::string s = "[";
        for (size_t i(0); i != val.size(); ++i) {
            s += DefaultValueNamer<T>::to_str(val[i]);
        }
        s += "]";
        return s;
    }
};


//helper functions for the ParseTree (=tree<ParseNode>)

template<class T>
typename tree<T>::sibling_iterator last_child(
    const tree<T> &tr, typename tree<T>::sibling_iterator ti) {
    return --tr.end(ti);
}

template<class T>
typename tree<T>::sibling_iterator last_child_of_root(const tree<T> &tr) {
    return last_child(tr, tr.begin());
}

template<class T>
typename tree<T>::sibling_iterator first_child(
    const tree<T> &tr, typename tree<T>::sibling_iterator ti) {
    return tr.begin(ti);
}

template<class T>
typename tree<T>::sibling_iterator first_child_of_root(const tree<T> &tr) {
    return first_child(tr, tr.begin());
}

template<class T>
typename tree<T>::sibling_iterator end_of_roots_children(const tree<T> &tr) {
    return tr.end(tr.begin());
}

template<class T>
tree<T> subtree(
    const tree<T> &tr, typename tree<T>::sibling_iterator ti) {
    typename tree<T>::sibling_iterator ti_next = ti;
    ++ti_next;
    return tr.subtree(ti, ti_next);
}


//Options is just a wrapper for map<string, boost::any>
class Options {
public:
    Options(bool hm = false)
        : help_mode(hm) {
    }

    void set_help_mode(bool hm) {
        help_mode = hm;
    }

    std::map<std::string, boost::any> storage;

    template <class T>
    void set(std::string key, T value) {
        storage[key] = value;
    }

    template <class T>
    T get(std::string key) const {
        std::map<std::string, boost::any>::const_iterator it;
        it = storage.find(key);
        if (it == storage.end()) {
            std::cout << "attempt to retrieve nonexisting object of name "
                      << key << " (type: " << TypeNamer<T>::name() << ")"
                      << " from Options. Aborting." << std::endl;
            exit_with(EXIT_CRITICAL_ERROR);
        }
        try {
            T result = boost::any_cast<T>(it->second);
            return result;
        } catch (const boost::bad_any_cast &bac) {
            std::cout << "Invalid conversion while retrieving config options!"
                      << std::endl
                      << key << " is not of type " << TypeNamer<T>::name()
                      << std::endl << "exiting" << std::endl;
            exit_with(EXIT_CRITICAL_ERROR);
        }
    }

    template <class T>
    void verify_list_non_empty(std::string key) const {
        if (!help_mode) {
            std::vector<T> temp_vec = get<std::vector<T> >(key);
            if (temp_vec.empty()) {
                std::cout << "Error: unexpected empty list!"
                          << std::endl
                          << "List " << key << " is empty"
                          << std::endl;
                exit_with(EXIT_INPUT_ERROR);
            }
        }
    }

    template <class T>
    std::vector<T> get_list(std::string key) const {
        return get<std::vector<T> >(key);
    }

    int get_enum(std::string key) const {
        return get<int>(key);
    }

    bool contains(std::string key) const {
        return storage.find(key) != storage.end();
    }
private:
    bool help_mode;
};

struct OptionFlags {
    OptionFlags(bool mand = true)
        : mandatory(mand) {
    }
    bool mandatory;
};


#endif
