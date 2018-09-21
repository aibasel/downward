#ifndef OPTIONS_OPTION_PARSER_H
#define OPTIONS_OPTION_PARSER_H

#include "doc_utils.h"
#include "options.h"
#include "predefinitions.h"
#include "registries.h"

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class SearchEngine;

namespace options {
/*
  The OptionParser stores a parse tree and an Options object. By
  calling addArgument, the parse tree is partially parsed, and the
  result is added to the Options.
*/
class OptionParser {
    Options opts;
    const ParseTree parse_tree;
    /*
      Cannot be const in the current design. The plugin factory methods insert
      PluginInfo structs into the registry when they are called. This could
      be improved later.
    */
    Registry &registry;
    const Predefinitions &predefinitions;
    const bool dry_run_;
    const bool help_mode_;

    ParseTree::sibling_iterator next_unparsed_argument;
    std::vector<std::string> valid_keys;

    std::string get_unparsed_config() const;

    template<class T>
    void check_bounds(
        const std::string &key, const T &value, const Bounds &bounds);


public:
    OptionParser(const ParseTree &parse_tree, Registry &registry,
                 const Predefinitions &predefinitions,
                 bool dry_run, bool help_mode = false);
    OptionParser(const std::string &config, Registry &registry,
                 const Predefinitions &predefinitions,
                 bool dry_run, bool help_mode = false);
    ~OptionParser() = default;
    OptionParser(const OptionParser &other) = delete;
    OptionParser &operator=(const OptionParser &other) = delete;

    /* This function initiates parsing of T (the root node of parse_tree will be
       parsed as T). Usually T=shared_ptr<SearchEngine>, Evaluator* or
       shared_ptr<LandmarkFactory>. */
    template<typename T>
    T start_parsing();

    /* Add option with default value. Use def_val=NONE for optional
       parameters without default values. */
    template<typename T>
    void add_option(
        const std::string &key,
        const std::string &help = "",
        const std::string &default_value = "",
        const Bounds &bounds = Bounds::unlimited());

    void add_enum_option(
        const std::string &key,
        const std::vector<std::string> &names,
        const std::string &help = "",
        const std::string &default_value = "",
        const std::vector<std::string> &docs = {});

    template<typename T>
    void add_list_option(
        const std::string &key,
        const std::string &help = "",
        const std::string &default_value = "");

    void document_synopsis(
        const std::string &name, const std::string &note) const;

    void document_property(
        const std::string &property, const std::string &note) const;

    void document_language_support(
        const std::string &feature, const std::string &note) const;

    void document_note(
        const std::string &name, const std::string &note, bool long_text = false) const;

    void error(const std::string &msg) const;

    /* TODO: "parse" is not the best name for this function. It just does some
       checks and returns the parsed options. Parsing happens before that. */
    Options parse();

    const ParseTree *get_parse_tree();
    Registry &get_registry();
    const Predefinitions &get_predefinitions() const;
    const std::string &get_root_value() const;

    bool dry_run() const;
    bool help_mode() const;

    static const std::string NONE;
};


/*
  TokenParser<T> wraps functions to parse supported types T.
*/
template<typename T>
class TokenParser {
public:
    static inline T parse(OptionParser &parser);
};

/*
  We need to give specializations of the class for the cases we want
  to *partially* specialize, i.e., give a specialization that is still
  templated. For fully specialized cases (e.g. parsing "int"), it is
  not necessary to specialize the class; we just need to specialize
  the method.
*/
template<typename T>
class TokenParser<std::shared_ptr<T>> {
public:
    static inline std::shared_ptr<T> parse(OptionParser &parser);
};

template<typename T>
class TokenParser<std::vector<T>> {
public:
    static inline std::vector<T> parse(OptionParser &parser);
};

/*
  If T has no template specialization, try to parse it directly from
  the input string. As of this writing, this default implementation is
  used only for string and bool.
*/
template<typename T>
inline T TokenParser<T>::parse(OptionParser &parser) {
    const std::string &value = parser.get_root_value();
    std::istringstream stream(value);
    T x;
    if ((stream >> std::boolalpha >> x).fail()) {
        parser.error("could not parse argument " + value + " of type " +
                     TypeNamer<T>::name(parser.get_registry()));
    }
    return x;
}

// int needs a specialization to allow "infinity".
template<>
inline int TokenParser<int>::parse(OptionParser &parser) {
    const std::string &value = parser.get_root_value();
    if (value == "infinity") {
        return std::numeric_limits<int>::max();
    } else {
        std::istringstream stream(value);
        int x;
        if ((stream >> x).fail()) {
            parser.error("could not parse int argument " + value);
        }
        return x;
    }
}

// double needs a specialization to allow "infinity".
template<>
inline double TokenParser<double>::parse(OptionParser &parser) {
    const std::string &value = parser.get_root_value();
    if (value == "infinity") {
        return std::numeric_limits<double>::infinity();
    } else {
        std::istringstream stream(value);
        double x;
        if ((stream >> x).fail()) {
            parser.error("could not parse double argument " + value);
        }
        return x;
    }
}

// Helper functions for the TokenParser-specializations.

template<typename T>
static std::shared_ptr<T> lookup_in_registry(OptionParser &parser) {
    const std::string &value = parser.get_root_value();
    try {
        return parser.get_registry().get_factory<std::shared_ptr<T>>(value)(parser);
    } catch (const std::out_of_range &) {
        parser.error(TypeNamer<std::shared_ptr<T>>::name(parser.get_registry()) + " " + value + " not found");
    }
    return nullptr;
}

template<typename T>
static std::shared_ptr<T> lookup_in_predefinitions(OptionParser &parser, bool &found) {
    using TPtr = std::shared_ptr<T>;
    const std::string &value = parser.get_root_value();
    if (parser.get_predefinitions().contains<TPtr>(value)) {
        found = true;
        return parser.get_predefinitions().get<TPtr>(value);
    }
    found = false;
    return nullptr;
}

template<typename T>
inline std::shared_ptr<T> TokenParser<std::shared_ptr<T>>::parse(OptionParser &parser) {
    bool predefined;
    std::shared_ptr<T> result = lookup_in_predefinitions<T>(parser, predefined);
    if (predefined)
        return result;
    return lookup_in_registry<T>(parser);
}

// Needed for iterated search.
template<>
inline ParseTree TokenParser<ParseTree>::parse(OptionParser &parser) {
    return *parser.get_parse_tree();
}

template<typename T>
inline std::vector<T> TokenParser<std::vector<T>>::parse(OptionParser &parser) {
    if (parser.get_parse_tree()->begin()->value != "list") {
        parser.error("expected list");
    }
    std::vector<T> results;
    for (auto tree_it = first_child_of_root(*parser.get_parse_tree());
         tree_it != end_of_roots_children(*parser.get_parse_tree());
         ++tree_it) {
        OptionParser subparser(subtree(*parser.get_parse_tree(), tree_it),
                               parser.get_registry(), parser.get_predefinitions(),
                               parser.dry_run());
        results.push_back(TokenParser<T>::parse(subparser));
    }
    return results;
}

template<typename T>
T OptionParser::start_parsing() {
    return TokenParser<T>::parse(*this);
}

template<class T>
void OptionParser::check_bounds(
    const std::string &, const T &, const Bounds &) {
}

template<>
void OptionParser::check_bounds<int>(
    const std::string &key, const int &value, const Bounds &bounds);

template<>
void OptionParser::check_bounds<double>(
    const std::string &key, const double &value, const Bounds &bounds);

template<typename T>
void OptionParser::add_option(
    const std::string &key,
    const std::string &help,
    const std::string &default_value,
    const Bounds &bounds) {
    if (help_mode()) {
        registry.add_plugin_info_arg(
            get_root_value(),
            key,
            help,
            TypeNamer<T>::name(registry),
            default_value,
            bounds);
        return;
    }
    valid_keys.push_back(key);
    bool use_default = false;
    ParseTree::sibling_iterator arg = next_unparsed_argument;
    if (arg == parse_tree.end(parse_tree.begin())) {
        // We have already handled all arguments.
        if (default_value.empty()) {
            error("missing option: " + key);
        } else if (default_value == NONE) {
            return;
        } else {
            use_default = true;
        }
    } else if (!arg->key.empty()) {
        // Handle arguments with explicit keyword.
        // Try to find a parameter passed with keyword key.
        for (; arg != parse_tree.end(parse_tree.begin()); ++arg) {
            if (arg->key == key)
                break;
        }
        if (arg == parse_tree.end(parse_tree.begin())) {
            if (default_value.empty()) {
                error("missing option: " + key);
            } else if (default_value == NONE) {
                return;
            } else {
                use_default = true;
            }
        }
    }
    std::unique_ptr<OptionParser> subparser =
        use_default ?
        utils::make_unique_ptr<OptionParser>(default_value, registry,
                                             predefinitions, dry_run()) :
        utils::make_unique_ptr<OptionParser>(subtree(parse_tree, arg), registry,
                                             predefinitions, dry_run());
    T result = TokenParser<T>::parse(*subparser);
    check_bounds<T>(key, result, bounds);
    opts.set<T>(key, result);
    /* If we have not reached the keyword parameters yet and have not used the
       default value, increment the argument position pointer. */
    if (!use_default && arg->key.empty()) {
        ++next_unparsed_argument;
    }
}

template<typename T>
void OptionParser::add_list_option(
    const std::string &key,
    const std::string &help,
    const std::string &default_value) {
    add_option<std::vector<T>>(key, help, default_value);
}
}

#endif
