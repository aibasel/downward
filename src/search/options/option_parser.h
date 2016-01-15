#ifndef OPTIONS_OPTION_PARSER_H
#define OPTIONS_OPTION_PARSER_H

#include "doc_store.h"
#include "options.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
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
    static int parse_int_arg(const std::string &name, const std::string &value);
    static SearchEngine *parse_cmd_line_aux(
        const std::vector<std::string> &args, bool dry_run);
public:
    OptionParser(const OptionParser &other) = delete;
    OptionParser &operator=(const OptionParser &other) = delete;
    OptionParser(std::string config, bool dr);
    OptionParser(ParseTree pt, bool dr);
    ~OptionParser() = default;

    static const std::string NONE;

    //this is where input from the commandline goes:
    static SearchEngine *parse_cmd_line(
        int argc, const char **argv, bool dr, bool is_unit_cost);

    static std::string usage(std::string progname);

    //this function initiates parsing of T (the root node of parse_tree
    //will be parsed as T). Usually T=SearchEngine*, ScalarEvaluator* or LandmarkGraph*
    template<typename T>
    T start_parsing();

    template<class T>
    void check_bounds(
        const std::string &key, const T &value, const Bounds &bounds);

    /* Add option with default value. Use def_val=NONE for optional
       parameters without default values. */
    template<typename T>
    void add_option(
        std::string k, std::string h = "", std::string def_val = "",
        Bounds bounds = Bounds::unlimited());

    void add_enum_option(std::string k,
                         std::vector<std::string > enumeration,
                         std::string h = "", std::string def_val = "",
                         std::vector<std::string> enum_doc = std::vector<std::string>());
    template<typename T>
    void add_list_option(std::string k,
                         std::string h = "", std::string def_val = "");

    bool is_valid_option(const std::string &k) const;

    void document_values(std::string argument,
                         ValueExplanations value_explanations) const;
    void document_synopsis(std::string name, std::string note) const;
    void document_property(std::string property, std::string note) const;
    void document_language_support(std::string feature, std::string note) const;
    void document_note(std::string name, std::string note, bool long_text = false) const;
    void document_hide() const;

    static void static_error(std::string msg);
    void error(std::string msg);
    void warning(std::string msg);

    Options parse(); //parse is not the best name for this function. It just does some checks and returns the parsed options Parsing happens before that. Change?
    const ParseTree *get_parse_tree();
    void set_help_mode(bool m);

    bool dry_run() const;
    bool help_mode() const;

    template<typename T>
    static std::string to_str(T in) {
        std::ostringstream out;
        out << std::boolalpha << in;
        return out.str();
    }

private:
    std::string unparsed_config;
    Options opts;
    const ParseTree parse_tree;
    bool dry_run_;
    bool help_mode_;

    ParseTree::sibling_iterator next_unparsed_argument;
    std::vector<std::string> valid_keys;

    void set_unparsed_config();
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
//Definitions of OptionParsers template functions:

template<typename T>
T OptionParser::start_parsing() {
    if (help_mode()) {
        DocStore::instance()->register_object(parse_tree.begin()->value,
                                              TypeNamer<T>::name());
    }
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
    std::string k, std::string h, std::string default_value, Bounds bounds) {
    if (help_mode()) {
        DocStore::instance()->add_arg(parse_tree.begin()->value,
                                      k, h,
                                      TypeNamer<T>::name(), default_value,
                                      bounds);
        return;
    }
    valid_keys.push_back(k);
    bool use_default(false);
    ParseTree::sibling_iterator arg = next_unparsed_argument;
    //scenario where we have already handled all arguments
    if (arg == parse_tree.end(parse_tree.begin())) {
        if (default_value.empty()) {
            error("missing option: " + k);
        } else if (default_value == NONE) {
            return;
        } else {
            use_default = true;
        }
    } else {
        //handling arguments with explicit keyword:
        if (!arg->key.empty()) { //to check if we reached the params supplied with key
            //try to find a parameter passed with keyword k
            for (; arg != parse_tree.end(parse_tree.begin()); ++arg) {
                if (arg->key.compare(k) == 0)
                    break;
            }
            if (arg == parse_tree.end(parse_tree.begin())) {
                if (default_value.empty()) {
                    error("missing option: " + k);
                } else if (default_value == NONE) {
                    return;
                } else {
                    use_default = true;
                }
            }
        }
    }
    std::unique_ptr<OptionParser> subparser(
        use_default ?
        new OptionParser(default_value, dry_run()) :
        new OptionParser(subtree(parse_tree, arg), dry_run()));
    T result = TokenParser<T>::parse(*subparser);
    check_bounds<T>(k, result, bounds);
    opts.set<T>(k, result);
    //if we have not reached the keyword parameters yet
    //and did not use the default value,
    //increment the argument position pointer
    if (!use_default && arg->key.empty()) {
        ++next_unparsed_argument;
    }
}

template<typename T>
void OptionParser::add_list_option(
    std::string k, std::string h, std::string def_val) {
    add_option<std::vector<T>>(k, h, def_val);
}
}

#endif
