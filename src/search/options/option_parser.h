#ifndef OPTIONS_OPTION_PARSER_H
#define OPTIONS_OPTION_PARSER_H

#include "doc_utils.h"
#include "options.h"
#include "predefinitions.h"
#include "registries.h"

#include "../utils/math.h"
#include "../utils/strings.h"

#include <cctype>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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
       parsed as T).*/
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

    template<typename T>
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
    std::string value = parser.get_root_value();

    if (value.empty()) {
        parser.error("int argument must not be empty");
    } else if (value == "infinity") {
        return std::numeric_limits<int>::max();
    }

    char suffix = value.back();
    int factor = 1;
    if (isalpha(suffix)) {
        /* Option values from the command line are already lower case, but
           default values specified in the code might be upper case. */
        suffix = static_cast<char>(std::tolower(suffix));
        if (suffix == 'k') {
            factor = 1000;
        } else if (suffix == 'm') {
            factor = 1000000;
        } else if (suffix == 'g') {
            factor = 1000000000;
        } else {
            parser.error("invalid suffix for int argument (valid: K, M, G)");
        }
        value.pop_back();
    }

    std::istringstream stream(value);
    int x;
    stream >> std::noskipws >> x;
    if (stream.fail() || !stream.eof()) {
        parser.error("could not parse int argument");
    }

    int min_int = std::numeric_limits<int>::min();
    // Reserve highest value for "infinity".
    int max_int = std::numeric_limits<int>::max() - 1;
    if (!utils::is_product_within_limits(x, factor, min_int, max_int)) {
        parser.error("overflow for int argument");
    }
    return x * factor;
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
        stream >> std::noskipws >> x;
        if (stream.fail() || !stream.eof()) {
            parser.error("could not parse double argument");
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
    found = parser.get_predefinitions().contains(value);
    return parser.get_predefinitions().get<TPtr>(value, nullptr);
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
void OptionParser::add_enum_option(
    const std::string &key,
    const std::vector<std::string> &names,
    const std::string &help,
    const std::string &default_value,
    const std::vector<std::string> &docs) {
    if (help_mode_) {
        std::string enum_descr = "{";
        for (size_t i = 0; i < names.size(); ++i) {
            enum_descr += names[i];
            if (i != names.size() - 1) {
                enum_descr += ", ";
            }
        }
        enum_descr += "}";

        ValueExplanations value_explanations;
        if (!docs.empty() && docs.size() != names.size()) {
            ABORT("Please provide documentation for all or none of the values of " + key);
        }
        for (size_t i = 0; i < docs.size(); ++i) {
            value_explanations.emplace_back(names[i], docs[i]);
        }

        registry.add_plugin_info_arg(
            get_root_value(), key, help, enum_descr, default_value,
            Bounds::unlimited(), value_explanations);
        return;
    }

    // Enum arguments can be given by name or by number.
    // First, parse the corresponding string like a normal argument ...
    add_option<std::string>(key, help, default_value);

    if (!opts.contains(key))
        return;

    std::string value = opts.get<std::string>(key);

    // ... then check if the parsed string can be treated as a number.
    std::istringstream stream(value);
    int choice;
    if (!(stream >> choice).fail()) {
        int max_choice = names.size();
        if (choice > max_choice) {
            error("invalid enum argument " + value + " for option " + key);
        }
        opts.set<T>(key, static_cast<T>(choice));
    } else {
        // ... otherwise map the string to its position in the enumeration vector.
        auto it = find_if(names.begin(), names.end(),
                          [&](const std::string &name) {
                              if (name.size() != value.size())
                                  return false;
                              for (size_t i = 0; i < value.size(); ++i) {
                                  // Ignore case.
                                  if (tolower(name[i]) != tolower(value[i]))
                                      return false;
                              }
                              return true;
                          });
        if (it == names.end()) {
            error("invalid enum argument " + value + " for option " + key);
        }
        opts.set<T>(key, static_cast<T>(it - names.begin()));
    }
}

template<typename T>
void OptionParser::add_list_option(
    const std::string &key,
    const std::string &help,
    const std::string &default_value) {
    add_option<std::vector<T>>(key, help, default_value);
}

template<typename T>
void predefine_plugin(const std::string &arg, Registry &registry,
                      Predefinitions &predefinitions, bool dry_run) {
    std::pair<std::string, std::string> predefinition;
    try {
        predefinition = utils::split(arg, "=");
    } catch (utils::StringOperationError &) {
        throw OptionParserError("Predefinition error: Predefinition has to be "
                                "of the form [name]=[definition].");
    }

    std::string key = predefinition.first;
    std::string value = predefinition.second;
    utils::strip(key);
    utils::strip(value);

    OptionParser parser(value, registry, predefinitions, dry_run);
    predefinitions.predefine(key, parser.start_parsing<std::shared_ptr<T>>());
}
}

#endif
