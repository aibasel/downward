#ifndef OPTION_PARSER_UTIL_H_
#define OPTION_PARSER_UTIL_H_

#include <string>
#include <vector>

class ParseTree;
class LandmarksGraph;
class Heuristic;
class ScalarEvaluator;
class Synergy;
class SearchEngine;
class OptionParser;
template<class Entry>
class OpenList;


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
//There's something built in for this (typeid().name(), but the output is not always very readable

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
struct TypeNamer<float> {
    static std::string name() {
        return "float";
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


#endif /* OPTION_PARSER_UTIL_H_ */
