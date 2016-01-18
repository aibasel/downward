#ifndef OPTIONS_PARSE_TREE_H
#define OPTIONS_PARSE_TREE_H

#include <iostream>
#include <string>

#include <tree.hh>
#include <tree_util.hh>

namespace options {
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
}

#endif
