#ifndef OPTION_PARSER_UTIL_H_
#define OPTION_PARSER_UTIL_H_

#include <string>
#include <vector>
#include <map>
#include <ios>
#include "tree.hh"


class LandmarksGraph;
class Heuristic;
class ScalarEvaluator;
class Synergy;
class SearchEngine;
class OptionParser;
template<class Entry>
class OpenList;


struct ParseNode {
    ParseNode()
        :value(""),
         key("")
    {
    }

    ParseNode(std::string val, std::string k = "")
        :value(val),
         key(k) 
    {
    }        
    std::string value;
    std::string key;

    friend std::ostream& operator<< (std::ostream &out, const ParseNode &pn) {
        if (pn.key.compare("") != 0) 
            out << pn.key << " = ";
        out << pn.value;
        return out;
    }
};

typedef tree<ParseNode> ParseTree;

struct ParseError{
    ParseError(std::string _msg, ParseTree pt);
    
    std::string msg;
    ParseTree parse_tree;
};


//a registry<T> maps a string to a T-factory
template <class T>
class Registry{
public:
    typedef T (*Factory)(OptionParser&);
    static Registry<T>* instance()
    {
        if (!instance_) {
            instance_ = new Registry<T>();
        }
        return instance_;
    }
            
    void register_object(std::string k, Factory f) {
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
        for(typename std::map<std::string,Factory>::iterator it = 
                registered.begin(); 
            it != registered.end(); ++it) {
            keys.push_back(it->first);
        }
        return keys;
    }

private:
    Registry(){};
    static Registry<T> *instance_;
    std::map<std::string, Factory> registered;
};

template <class T> Registry<T>* Registry<T>::instance_ = 0;




//Predefinitions<T> maps strings to pointers to
//already created Heuristics/LandmarksGraphs
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

    void predefine(std::string k, T obj) {
        predefined[k] = obj;
        std::cout << "predefined " << k << std::endl;
    }

    bool contains(std::string k) {
        return predefined.find(k) != predefined.end();
    }

    T get(std::string k) {
        return predefined[k];
    }

private:
    Predefinitions<T>(){};
    static Predefinitions<T>* instance_;
    std::map<std::string, T> predefined;
};

template <class T> Predefinitions<T>* Predefinitions<T>::instance_ = 0;



struct Synergy {
    std::vector<Heuristic *> heuristics;
};

//TypeNamer prints out names of types. 
//There's something built in for this (typeid().name()), but the output is not always very readable

template <class T>
struct TypeNamer {
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
struct TypeNamer<LandmarksGraph *> {
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

template <class Entry>
struct TypeNamer<OpenList<Entry> *> {
    static std::string name() {
        return "openlist";
    }
};

template <class T>
struct TypeNamer<std::vector<T> > {
    static std::string name() {
        return "list of "+TypeNamer<T>::name();
    }
};

//DefaultValueNamer is for printing default values.
//Maybe a better solution would be to implement "<<" for everything that's needed.

template <class T>
struct DefaultValueNamer {
    static std::string toStr(T val) {
        std::ostringstream strs;
        strs << val;
        return strs.str();
    }
};

template <>
struct DefaultValueNamer<bool> {
    static std::string toStr(bool val) {
        if(val) {
            return "true";
        } else {
            return "false";
        }
    }
};

template <>
struct DefaultValueNamer<ParseTree> {
    static std::string toStr(ParseTree val) {
        return "implement default value naming for parse trees!" 
            + val.begin()->value;
    }
};

template <class T>
struct DefaultValueNamer<std::vector<T> > {
    static std::string toStr(std::vector<T> val) {
        std::string s = "[";
        for(size_t i(0); i != val.size(); ++i) {
            s += DefaultValueNamer<T>::toStr(val[i]);
        }
        s += "]";
        return s;
    }
};


#endif /* OPTION_PARSER_UTIL_H_ */
