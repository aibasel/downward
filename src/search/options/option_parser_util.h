#ifndef OPTIONS_OPTION_PARSER_UTIL_H
#define OPTIONS_OPTION_PARSER_UTIL_H

#include "../utilities.h"

#include <tree.hh>
#include <tree_util.hh>

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <ios>
#include <iostream>
#include <sstream>
#include <typeindex>
#include <typeinfo>
#include <utility>

class Heuristic;
class OptionParser;


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

    friend std::ostream &operator<<(std::ostream &out, const ParseNode &pn) {
        if (pn.key.compare("") != 0)
            out << pn.key << " = ";
        out << pn.value;
        return out;
    }
};

typedef tree<ParseNode> ParseTree;

/*
  TODO: Due to cyclic dependencies, we must currently have this
  include *after* defining ParseTree. Needs to be fixed.
*/
#include "type_namer.h"

struct Bounds {
    std::string min;
    std::string max;

public:
    Bounds(std::string min, std::string max)
        : min(min), max(max) {}
    ~Bounds() = default;

    bool has_bound() const {
        return !min.empty() || !max.empty();
    }

    static Bounds unlimited() {
        return Bounds("", "");
    }

    friend std::ostream &operator<<(std::ostream &out, const Bounds &bounds);
};


/*
  Predefinitions<T> maps strings to pointers to already created
  plug-in objects.
*/
template<typename T>
class Predefinitions {
public:
    static Predefinitions<T> *instance() {
        static Predefinitions<T> instance_;
        return &instance_;
    }

    void predefine(std::string k, T obj) {
        transform(k.begin(), k.end(), k.begin(), ::tolower);
        predefined[k] = obj;
    }

    bool contains(const std::string &k) {
        return predefined.find(k) != predefined.end();
    }

    T get(const std::string &k) {
        return predefined[k];
    }

private:
    Predefinitions<T>() = default;
    std::map<std::string, T> predefined;
};


class Synergy {
public:
    std::vector<Heuristic *> heuristics;
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

#endif
