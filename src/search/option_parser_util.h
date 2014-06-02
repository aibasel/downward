#ifndef OPTION_PARSER_UTIL_H
#define OPTION_PARSER_UTIL_H

#include "utilities.h"
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <ios>
#include <iostream>
#include <sstream>
#include <tree.hh>
#include <tree_util.hh>
#include <utility>
#include <boost/any.hpp>


class MergeStrategy;
class ShrinkStrategy;
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

    friend std::ostream &operator<<(std::ostream &out, const ParseNode &pn) {
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
        if (pe.substr.size() > 0) {
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



class Synergy {
public:
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
        return "Heuristic";
    }
};

template <>
struct TypeNamer<LandmarkGraph *> {
    static std::string name() {
        return "LandmarkGraph";
    }
};

template <>
struct TypeNamer<ScalarEvaluator *> {
    static std::string name() {
        return "ScalarEvaluator";
    }
};

template <>
struct TypeNamer<SearchEngine *> {
    static std::string name() {
        return "SearchEngine";
    }
};

template <>
struct TypeNamer<ParseTree> {
    static std::string name() {
        return "ParseTree (this just means the input is parsed at a later point. The real type is probably a search engine.)";
    }
};

template <>
struct TypeNamer<Synergy *> {
    static std::string name() {
        return "Synergy";
    }
};

template <>
struct TypeNamer<MergeStrategy *> {
    static std::string name() {
        return "MergeStrategy";
    }
};

template <>
struct TypeNamer<ShrinkStrategy *> {
    static std::string name() {
        return "ShrinkStrategy";
    }
};

template <class Entry>
struct TypeNamer<OpenList<Entry> *> {
    static std::string name() {
        return "OpenList";
    }
};

template <class T>
struct TypeNamer<std::vector<T> > {
    static std::string name() {
        return "list of " + TypeNamer<T>::name();
    }
};

// TypeDocumenter allows to add global documentation to the types.
// This cannot be done during parsing, because types are not parsed.

template <class T>
struct TypeDocumenter {
    static std::string synopsis() {
        return "";
    }
};

template <>
struct TypeDocumenter<Heuristic *> {
    static std::string synopsis() {
        return "A heuristic specification is either a newly created heuristic "
               "instance or a heuristic that has been defined previously. "
               "This page describes how one can specify a new heuristic instance. "
               "For re-using heuristics, see OptionSyntax#Heuristic_Predefinitions.\n\n"
               "Definitions of //properties// in the descriptions below:\n\n"
               " * **admissible:** h(s) <= h*(s) for all states s\n"
               " * **consistent:** h(s) + c(s, s') >= h(s') for all states s "
               "connected to states s' by an action with cost c(s, s')\n"
               " * **safe:** h(s) = infinity is only true for states "
               "with h*(s) = infinity\n"
               " * **preferred operators:** this heuristic identifies "
               "preferred operators ";
    }
};

template <>
struct TypeDocumenter<LandmarkGraph *> {
    static std::string synopsis() {
        return "A landmark graph specification is either a newly created "
               "instance or a landmark graph that has been defined previously. "
               "This page describes how one can specify a new landmark graph instance. "
               "For re-using landmark graphs, see OptionSyntax#Landmark_Predefinitions.\n\n"
               "**Warning:** See OptionCaveats for using cost types with Landmarks";
    }
};

template <>
struct TypeDocumenter<ScalarEvaluator *> {
    static std::string synopsis() {
        return "XXX TODO: description of the role of scalar evaluators and the connection to Heuristic";
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

//TODO: get rid of OptionFlags, instead use default_value = "None" ?
struct OptionFlags {
    explicit OptionFlags(bool mand = true)
        : mandatory(mand) {
    }
    bool mandatory;
};

typedef std::vector<std::pair<std::string, std::string> > ValueExplanations;
struct ArgumentInfo {
    ArgumentInfo(
        std::string k, std::string h, std::string t_n, std::string def_val,
        bool mand, ValueExplanations val_expl)
        : kwd(k),
          help(h),
          type_name(t_n),
          default_value(def_val),
          mandatory(mand),
          value_explanations(val_expl) {
    }
    std::string kwd;
    std::string help;
    std::string type_name;
    std::string default_value;
    bool mandatory;
    std::vector<std::pair<std::string, std::string> > value_explanations;
};

struct PropertyInfo {
    PropertyInfo(std::string prop, std::string descr)
        : property(prop),
          description(descr) {
    }
    std::string property;
    std::string description;
};

struct NoteInfo {
    NoteInfo(std::string n, std::string descr, bool long_text_)
        : name(n),
          description(descr),
          long_text(long_text_){
    }
    std::string name;
    std::string description;
    bool long_text;
};


struct LanguageSupportInfo {
    LanguageSupportInfo(std::string feat, std::string descr)
        : feature(feat),
          description(descr) {
    }
    std::string feature;
    std::string description;
};

//stores documentation for a single type, for use in combination with DocStore
struct DocStruct {
    std::string type;
    std::string full_name;
    std::string synopsis;
    std::vector<ArgumentInfo> arg_help;
    std::vector<PropertyInfo> property_help;
    std::vector<LanguageSupportInfo> support_help;
    std::vector<NoteInfo> notes;
    bool hidden;
};

//stores documentation for types parsed in help mode
class DocStore {
public:
    static DocStore *instance() {
        if (!instance_) {
            instance_ = new DocStore();
        }
        return instance_;
    }

    void register_object(std::string k, std::string type);

    void add_arg(std::string k,
                 std::string arg_name,
                 std::string help,
                 std::string type,
                 std::string default_value,
                 bool mandatory,
                 ValueExplanations value_explanations = ValueExplanations());
    void add_value_explanations(std::string k,
                                std::string arg_name,
                                ValueExplanations value_explanations);
    void set_synopsis(std::string k,
                      std::string name, std::string description);
    void add_property(std::string k,
                      std::string name, std::string description);
    void add_feature(std::string k,
                     std::string feature, std::string description);
    void add_note(std::string k,
                  std::string name, std::string description, bool long_text);
    void hide(std::string k);

    bool contains(std::string k);
    DocStruct get(std::string k);
    std::vector<std::string> get_keys();
    std::vector<std::string> get_types();

private:
    DocStore() {}
    static DocStore *instance_;
    std::map<std::string, DocStruct> registered;
};

class DocPrinter {
public:
    DocPrinter(std::ostream &out);
    virtual ~DocPrinter();

    virtual void print_all();
    virtual void print_category(std::string category_name);
    virtual void print_element(std::string call_name, const DocStruct &info);
protected:
    std::ostream &os;
    virtual void print_synopsis(const DocStruct &info) = 0;
    virtual void print_usage(std::string call_name, const DocStruct &info) = 0;
    virtual void print_arguments(const DocStruct &info) = 0;
    virtual void print_notes(const DocStruct &info) = 0;
    virtual void print_language_features(const DocStruct &info) = 0;
    virtual void print_properties(const DocStruct &info) = 0;
    virtual void print_category_header(std::string category_name) = 0;
    virtual void print_category_footer() = 0;
};

class Txt2TagsPrinter : public DocPrinter {
public:
    Txt2TagsPrinter(std::ostream &out);
    virtual ~Txt2TagsPrinter();
protected:
    virtual void print_synopsis(const DocStruct &info);
    virtual void print_usage(std::string call_name, const DocStruct &info);
    virtual void print_arguments(const DocStruct &info);
    virtual void print_notes(const DocStruct &info);
    virtual void print_language_features(const DocStruct &info);
    virtual void print_properties(const DocStruct &info);
    virtual void print_category_header(std::string category_name);
    virtual void print_category_footer();
};

class PlainPrinter : public DocPrinter {
public:
    PlainPrinter(std::ostream &out, bool print_all = false);
    virtual ~PlainPrinter();
protected:
    virtual void print_synopsis(const DocStruct &info);
    virtual void print_usage(std::string call_name, const DocStruct &info);
    virtual void print_arguments(const DocStruct &info);
    virtual void print_notes(const DocStruct &info);
    virtual void print_language_features(const DocStruct &info);
    virtual void print_properties(const DocStruct &info);
    virtual void print_category_header(std::string category_name);
    virtual void print_category_footer();
private:
    bool print_all; //if this is false, notes, properties and language_features are omitted
};

#endif
