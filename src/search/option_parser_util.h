#ifndef OPTION_PARSER_UTIL_H_
#define OPTION_PARSER_UTIL_H_

#include <string>
#include <vector>
#include "open_lists/open_list.h"

class ParseTree;
class LandmarksGraph;
class Heuristic;
class ScalarEvaluator;

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
struct TypeNamer<string> {
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
struct TypeNamer<ParseTree> {
    static std::string name() {
        return "parse tree (this just means the input is parsed at a later point. The real type is probably a search engine.)";
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
