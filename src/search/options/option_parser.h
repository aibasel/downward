#ifndef OPTIONS_OPTION_PARSER_H
#define OPTIONS_OPTION_PARSER_H

#include "doc_store.h"
#include "options.h"

#include <memory>
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
    const bool dry_run_;
    const bool help_mode_;

    ParseTree::sibling_iterator next_unparsed_argument;
    std::vector<std::string> valid_keys;

    std::string get_unparsed_config() const;

    template<class T>
    void check_bounds(
        const std::string &key, const T &value, const Bounds &bounds);

    static int parse_int_arg(const std::string &name, const std::string &value);
    static std::shared_ptr<SearchEngine> parse_cmd_line_aux(
        const std::vector<std::string> &args, bool dry_run);

public:
    OptionParser(const ParseTree &parse_tree, bool dry_run, bool help_mode = false);
    OptionParser(const std::string &config, bool dry_run, bool help_mode = false);
    ~OptionParser() = default;
    OptionParser(const OptionParser &other) = delete;
    OptionParser &operator=(const OptionParser &other) = delete;

    /* This function initiates parsing of T (the root node of parse_tree will be
       parsed as T). Usually T=shared_ptr<SearchEngine>, Evaluator* or LandmarkFactory*. */
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

    void document_values(
        const std::string &argument,
        const ValueExplanations &value_explanations) const;

    void document_synopsis(
        const std::string &name, const std::string &note) const;

    void document_property(
        const std::string &property, const std::string &note) const;

    void document_language_support(
        const std::string &feature, const std::string &note) const;

    void document_note(
        const std::string &name, const std::string &note, bool long_text = false) const;

    void document_hide() const;

    void error(const std::string &msg) const;

    /* TODO: "parse" is not the best name for this function. It just does some
       checks and returns the parsed options. Parsing happens before that. */
    Options parse();

    const ParseTree *get_parse_tree();
    const std::string &get_root_value() const;

    bool dry_run() const;
    bool help_mode() const;

    static const std::string NONE;

    static std::shared_ptr<SearchEngine> parse_cmd_line(
        int argc, const char **argv, bool dry_run, bool is_unit_cost);

    static std::string usage(const std::string &progname);
};

/*
  TODO: OptionParser and TokenParser are currently so tangled that we
  *must* have this #include statement right here, after defining
  OptionParser and before defining its methods. This obviously needs
  more work.
*/
}
#include "token_parser.h"

namespace options {
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
        DocStore::instance()->add_arg(
            get_root_value(),
            key,
            help,
            TypeNamer<T>::name(),
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
        utils::make_unique_ptr<OptionParser>(default_value, dry_run()) :
        utils::make_unique_ptr<OptionParser>(subtree(parse_tree, arg), dry_run());
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
