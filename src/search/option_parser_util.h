#ifndef OPTION_PARSER_UTIL_H
#define OPTION_PARSER_UTIL_H

#include "utilities.h"

#include <boost/any.hpp>
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

// TODO: The following forward declaration can hopefully go away eventually.
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

struct ArgError {
    ArgError(std::string msg);

    std::string msg;

    friend std::ostream &operator<<(std::ostream &out, const ArgError &err) {
        return out << "argument error: " << err.msg;
    }
};

struct ParseError {
    ParseError(std::string m, ParseTree pt);
    ParseError(std::string m, ParseTree pt, std::string correct_substring);

    std::string msg;
    ParseTree parse_tree;
    std::string substr;

    friend std::ostream &operator<<(std::ostream &out, const ParseError &pe) {
        out << "parse error: " << std::endl
            << pe.msg << " at: " << std::endl;
        kptree::print_tree_bracketed<ParseNode>(pe.parse_tree, out);
        if (pe.substr.size() > 0) {
            out << " (cannot continue parsing after \"" << pe.substr << "\")" << std::endl;
        }
        return out;
    }
};


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


//a registry<T> maps a string to a T-factory
template<typename T>
class Registry {
public:
    typedef T (*Factory)(OptionParser &);
    static Registry<T> *instance() {
        static Registry<T> instance_;
        return &instance_;
    }

    void insert(const std::string &k, Factory f) {
        if (registered.count(k)) {
            std::cerr << "duplicate key in registry: " << k << std::endl;
            exit_with(EXIT_CRITICAL_ERROR);
        }
        registered[k] = f;
    }

    bool contains(const std::string &k) {
        return registered.find(k) != registered.end();
    }

    Factory get(const std::string &k) {
        return registered[k];
    }

    std::vector<std::string> get_keys() {
        std::vector<std::string> keys;
        for (auto it : registered) {
            keys.push_back(it.first);
        }
        return keys;
    }

private:
    Registry() = default;
    std::map<std::string, Factory> registered;
};


/*
  The plugin type info class contains meta-information for a given
  type of plugins (e.g. "SearchEngine" or "MergeStrategy").
*/
class PluginTypeInfo {
    std::type_index type;

    /*
      The type name should be "user-friendly". It is for example used
      as the name of the wiki page that documents this plugin type.
      It follows wiki conventions (e.g. "Heuristic", "SearchEngine",
      "ShrinkStrategy").
    */
    std::string type_name;

    /*
      General documentation for the plugin type. This is included at
      the top of the wiki page for this plugin type.
    */
    std::string documentation;
public:
    PluginTypeInfo(const std::type_index &type,
                   const std::string &type_name,
                   const std::string &documentation)
        : type(type),
          type_name(type_name),
          documentation(documentation) {
    }

    ~PluginTypeInfo() {
    }

    const std::type_index &get_type() const {
        return type;
    }

    const std::string &get_type_name() const {
        return type_name;
    }

    const std::string &get_documentation() const {
        return documentation;
    }
};

/*
  The plugin type registry collects information about all plugin types
  in use and gives access to the underlying information. This is used,
  for example, to generate the complete help output.

  Note that the information for individual plugins (rather than plugin
  types) is organized in separate registries, one for each plugin
  type. For example, there is a Registry<Heuristic> that organizes the
  Heuristic plugins.
*/

// TODO: Reduce code duplication with Registry<T>.
class PluginTypeRegistry {
    using Map = std::map<std::type_index, PluginTypeInfo>;
    PluginTypeRegistry() = default;
    ~PluginTypeRegistry() = default;
    Map registry;
public:
    static PluginTypeRegistry *instance();
    void insert(const PluginTypeInfo &info);
    const PluginTypeInfo &get(const std::type_index &type) const;

    Map::const_iterator begin() const {
        /*
          TODO (post-issue586): We want plugin types sorted by name in
          output. One way to achieve this is by defining the map's
          comparison function to sort first by the name and then by
          the type_index, but this is actually a bit difficult if the
          name isn't part of the key. One option to work around this
          is to use a set instead of a map as the internal data
          structure here.
        */
        return registry.cbegin();
    }

    Map::const_iterator end() const {
        return registry.cend();
    }
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

/*
  TypeNamer prints out names of types.

  There is no default implementation for Typenamer<T>::name(): the
  template needs to be specialized for each type we want to support.
  However, we have a generic version below for shared_ptr<...> types,
  which are the ones we use for plugins.
*/
template<typename T>
struct TypeNamer {
    static std::string name();
};

/*
  Note: for plug-in types, we use TypeNamer<shared_ptr<T>>::name().
  One might be tempted to strip away the shared_ptr<...> here and use
  TypeNamer<T>::name() instead, but this has the disadvantage that
  typeid(T) requires T to be a complete type, while
  typeid(shared_ptr<T>) also accepts incomplete types.
*/
template<typename T>
struct TypeNamer<std::shared_ptr<T>> {
    static std::string name() {
        using TPtr = std::shared_ptr<T>;
        const PluginTypeInfo &type_info =
            PluginTypeRegistry::instance()->get(std::type_index(typeid(TPtr)));
        return type_info.get_type_name();
    }
};

/*
  The following partial specialization for raw pointers is legacy code.
  This can go away once all plugins use shared_ptr.
*/
template<typename T>
struct TypeNamer<T *> {
    static std::string name() {
        return TypeNamer<std::shared_ptr<T>>::name();
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
struct TypeNamer<ParseTree> {
    static std::string name() {
        return "ParseTree (this just means the input is parsed at a later point. The real type is probably a search engine.)";
    }
};

template<typename Entry>
struct TypeNamer<std::shared_ptr<OpenList<Entry>>> {
    static std::string name() {
        return "OpenList";
    }
};

template<typename T>
struct TypeNamer<std::vector<T>> {
    static std::string name() {
        return "list of " + TypeNamer<T>::name();
    }
};

/*
  TypeDocumenter prints out the documentation synopsis for plug-in types.

  The same comments as for TypeNamer apply.
*/
template<typename T>
struct TypeDocumenter {
    static std::string synopsis() {
        /*
          TODO (post-issue586): once all plugin types are pluginized, this
          default implementation can go away (as in TypeNamer).
        */
        return "";
    }
};

// See comments for TypeNamer.
template<typename T>
struct TypeDocumenter<std::shared_ptr<T>> {
    static std::string synopsis() {
        using TPtr = std::shared_ptr<T>;
        const PluginTypeInfo &type_info =
            PluginTypeRegistry::instance()->get(std::type_index(typeid(TPtr)));
        return type_info.get_documentation();
    }
};

/*
  The following partial specialization for raw pointers is legacy code.
  This can go away once all plugins use shared_ptr.
*/
template<typename T>
struct TypeDocumenter<T *> {
    static std::string synopsis() {
        return TypeDocumenter<std::shared_ptr<T>>::synopsis();
    }
};

template<typename Entry>
struct TypeDocumenter<std::shared_ptr<OpenList<Entry>>> {
    static std::string synopsis() {
        return "";
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
        : unparsed_config("<missing>"),
          help_mode(hm) {
    }

    void set_help_mode(bool hm) {
        help_mode = hm;
    }

    std::map<std::string, boost::any> storage;

    template<typename T>
    void set(std::string key, T value) {
        storage[key] = value;
    }

    template<typename T>
    T get(std::string key) const {
        std::map<std::string, boost::any>::const_iterator it;
        it = storage.find(key);
        if (it == storage.end()) {
            ABORT("Attempt to retrieve nonexisting object of name " +
                  key + " (type: " + TypeNamer<T>::name() +
                  ") from options.");
        }
        try {
            T result = boost::any_cast<T>(it->second);
            return result;
        } catch (const boost::bad_any_cast &) {
            std::cout << "Invalid conversion while retrieving config options!"
                      << std::endl
                      << key << " is not of type " << TypeNamer<T>::name()
                      << std::endl << "exiting" << std::endl;
            exit_with(EXIT_CRITICAL_ERROR);
        }
    }

    template<typename T>
    T get(std::string key, const T &default_value) const {
        if (storage.count(key))
            return get<T>(key);
        else
            return default_value;
    }

    template<typename T>
    void verify_list_non_empty(std::string key) const {
        if (!help_mode) {
            std::vector<T> temp_vec = get<std::vector<T>>(key);
            if (temp_vec.empty()) {
                std::cout << "Error: unexpected empty list!"
                          << std::endl
                          << "List " << key << " is empty"
                          << std::endl;
                exit_with(EXIT_INPUT_ERROR);
            }
        }
    }

    template<typename T>
    std::vector<T> get_list(std::string key) const {
        return get<std::vector<T>>(key);
    }

    int get_enum(std::string key) const {
        return get<int>(key);
    }

    bool contains(std::string key) const {
        return storage.find(key) != storage.end();
    }

    std::string get_unparsed_config() const {
        return unparsed_config;
    }

    void set_unparsed_config(const std::string &config) {
        unparsed_config = config;
    }
private:
    std::string unparsed_config;
    bool help_mode;
};

typedef std::vector<std::pair<std::string, std::string>> ValueExplanations;
struct ArgumentInfo {
    ArgumentInfo(
        std::string k, std::string h, std::string t_n, std::string def_val,
        const Bounds &bounds, ValueExplanations val_expl)
        : kwd(k),
          help(h),
          type_name(t_n),
          default_value(def_val),
          bounds(bounds),
          value_explanations(val_expl) {
    }
    std::string kwd;
    std::string help;
    std::string type_name;
    std::string default_value;
    Bounds bounds;
    std::vector<std::pair<std::string, std::string>> value_explanations;
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
          long_text(long_text_) {
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
        static DocStore instance_;
        return &instance_;
    }

    void register_object(std::string k, std::string type);

    void add_arg(std::string k,
                 std::string arg_name,
                 std::string help,
                 std::string type,
                 std::string default_value,
                 Bounds bounds,
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
    DocStore() = default;
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
