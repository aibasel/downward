#ifndef OPTIONS_PARSE_TREE_H
#define OPTIONS_PARSE_TREE_H

#include <iostream>
#include <string>

#include <tree.hh>
#include <tree_util.hh>

namespace options {
struct ParseNode {
    std::string value;
    std::string key;

    ParseNode()
        : value(""),
          key("") {
    }

    ParseNode(const std::string &value, const std::string &key = "")
        : value(value),
          key(key) {
    }

    friend std::ostream &operator<<(std::ostream &out, const ParseNode &parse_node) {
        if (!parse_node.key.empty())
            out << parse_node.key << " = ";
        out << parse_node.value;
        return out;
    }
};

using ParseTree = tree<ParseNode>;

// Helper functions for the ParseTree.

template<class T>
typename tree<T>::sibling_iterator last_child(
    const tree<T> &parse_tree, typename tree<T>::sibling_iterator tree_it) {
    return --parse_tree.end(tree_it);
}

template<class T>
typename tree<T>::sibling_iterator last_child_of_root(const tree<T> &parse_tree) {
    return last_child(parse_tree, parse_tree.begin());
}

template<class T>
typename tree<T>::sibling_iterator first_child(
    const tree<T> &parse_tree, typename tree<T>::sibling_iterator tree_it) {
    return parse_tree.begin(tree_it);
}

template<class T>
typename tree<T>::sibling_iterator first_child_of_root(const tree<T> &parse_tree) {
    return first_child(parse_tree, parse_tree.begin());
}

template<class T>
typename tree<T>::sibling_iterator end_of_roots_children(const tree<T> &parse_tree) {
    return parse_tree.end(parse_tree.begin());
}

template<class T>
tree<T> subtree(
    const tree<T> &parse_tree, typename tree<T>::sibling_iterator tree_it) {
    typename tree<T>::sibling_iterator tree_it_next = tree_it;
    ++tree_it_next;
    return parse_tree.subtree(tree_it, tree_it_next);
}
}

#endif
