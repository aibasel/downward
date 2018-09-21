#include "option_parser.h"

#include "errors.h"

#include "../ext/tree_util.hh"

#include <iostream>


using namespace std;

namespace landmarks {
class LandmarkFactory;
}

namespace options {
const string OptionParser::NONE = "<none>";



template<class T>
void _check_bounds(
    OptionParser &parser, const string &key, T value,
    T lower_bound, T upper_bound) {
    if (lower_bound > upper_bound)
        ABORT("lower bound is greater than upper bound for " + key);
    if (value < lower_bound || value > upper_bound) {
        ostringstream stream;
        stream << key << " (" << value << ") must be in range ["
               << lower_bound << ", " << upper_bound << "]";
        parser.error(stream.str());
    }
}

template<>
void OptionParser::check_bounds<int>(
    const string &key, const int &value, const Bounds &bounds) {
    int lower_bound = numeric_limits<int>::lowest();
    int upper_bound = numeric_limits<int>::max();
    if (!bounds.min.empty()) {
        OptionParser bound_parser(bounds.min, registry, predefinitions, dry_run());
        lower_bound = TokenParser<int>::parse(bound_parser);
    }
    if (!bounds.max.empty()) {
        OptionParser bound_parser(bounds.max, registry, predefinitions, dry_run());
        upper_bound = TokenParser<int>::parse(bound_parser);
    }
    _check_bounds(*this, key, value, lower_bound, upper_bound);
}

template<>
void OptionParser::check_bounds<double>(
    const string &key, const double &value, const Bounds &bounds) {
    double lower_bound = -numeric_limits<double>::infinity();
    double upper_bound = numeric_limits<double>::infinity();
    if (!bounds.min.empty()) {
        OptionParser bound_parser(bounds.min, registry, predefinitions, dry_run());
        lower_bound = TokenParser<double>::parse(bound_parser);
    }
    if (!bounds.max.empty()) {
        OptionParser bound_parser(bounds.max, registry, predefinitions, dry_run());
        upper_bound = TokenParser<double>::parse(bound_parser);
    }
    _check_bounds(*this, key, value, lower_bound, upper_bound);
}




static ParseTree generate_parse_tree(const string &config) {
    ParseTree tree;
    ParseTree::sibling_iterator pseudoroot =
        tree.insert(tree.begin(), ParseNode("pseudoroot", ""));
    ParseTree::sibling_iterator cur_node = pseudoroot;
    string buffer;
    string key;
    for (size_t i = 0; i < config.size(); ++i) {
        char next = config.at(i);
        if ((next == '(' || next == ')' || next == ',') && !buffer.empty()) {
            tree.append_child(cur_node, ParseNode(buffer, key));
            buffer.clear();
            key.clear();
        } else if (next == '(' && buffer.empty()) {
            throw ParseError("misplaced opening bracket (", *cur_node, config.substr(0, i));
        }
        switch (next) {
        case ' ':
            break;
        case '(':
            cur_node = last_child(tree, cur_node);
            break;
        case ')':
            if (cur_node == pseudoroot)
                throw ParseError("missing (", *cur_node, config.substr(0, i));
            cur_node = tree.parent(cur_node);
            break;
        case '[':
            if (!buffer.empty())
                throw ParseError("misplaced opening bracket [", *cur_node, config.substr(0, i));
            tree.append_child(cur_node, ParseNode("list", key));
            key.clear();
            cur_node = last_child(tree, cur_node);
            break;
        case ']':
            if (!buffer.empty()) {
                tree.append_child(cur_node, ParseNode(buffer, key));
                buffer.clear();
                key.clear();
            }
            if (cur_node->value != "list") {
                throw ParseError("mismatched brackets", *cur_node, config.substr(0, i));
            }
            cur_node = tree.parent(cur_node);
            break;
        case ',':
            break;
        case '=':
            if (buffer.empty())
                throw ParseError("expected keyword before =", *cur_node, config.substr(0, i));
            key = buffer;
            buffer.clear();
            break;
        default:
            buffer.push_back(next);
            break;
        }
    }
    if (cur_node->value != "pseudoroot")
        throw ParseError("missing )", *cur_node);
    if (!buffer.empty())
        tree.append_child(cur_node, ParseNode(buffer, key));

    // The real parse tree is the first (and only) child of the pseudoroot placeholder.
    return subtree(tree, tree.begin(pseudoroot));
}


OptionParser::OptionParser(const ParseTree &parse_tree, Registry &registry,
                           const Predefinitions &predefinitions,
                           bool dry_run, bool help_mode)
    : opts(help_mode),
      parse_tree(parse_tree),
      registry(registry),
      predefinitions(predefinitions),
      dry_run_(dry_run),
      help_mode_(help_mode),
      next_unparsed_argument(first_child_of_root(this->parse_tree)) {
}

OptionParser::OptionParser(const string &config, Registry &registry,
                           const Predefinitions &predefinitions,
                           bool dry_run, bool help_mode)
    : OptionParser(generate_parse_tree(config), registry, predefinitions,
                   dry_run, help_mode) {
}

string OptionParser::get_unparsed_config() const {
    ostringstream stream;
    kptree::print_tree_bracketed<ParseNode>(parse_tree, stream);
    return stream.str();
}

void OptionParser::add_enum_option(
    const string &key,
    const vector<string> &names,
    const string &help,
    const string &default_value,
    const vector<string> &docs) {
    if (help_mode_) {
        string enum_descr = "{";
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
    add_option<string>(key, help, default_value);

    if (!opts.contains(key))
        return;

    string value = opts.get<string>(key);

    // ... then check if the parsed string can be treated as a number.
    istringstream stream(value);
    int choice;
    if (!(stream >> choice).fail()) {
        int max_choice = names.size();
        if (choice > max_choice) {
            error("invalid enum argument " + value + " for option " + key);
        }
        opts.set<int>(key, choice);
    } else {
        // ... otherwise map the string to its position in the enumeration vector.
        auto it = find_if(names.begin(), names.end(),
                          [&](const string &name) {
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
        opts.set<int>(key, it - names.begin());
    }
}

Options OptionParser::parse() {
    /* Check if there were any arguments with invalid keywords,
       or positional arguments after keyword arguments. */
    string last_key;
    for (auto tree_it = first_child_of_root(parse_tree);
         tree_it != end_of_roots_children(parse_tree);
         ++tree_it) {
        if (!tree_it->key.empty()) {
            if (find(valid_keys.begin(), valid_keys.end(), tree_it->key) == valid_keys.end()) {
                error("invalid keyword " + tree_it->key + " for " + get_root_value());
            }
        }
        if (tree_it->key.empty() && !last_key.empty()) {
            error("positional argument after keyword argument");
        }
        last_key = tree_it->key;
    }
    opts.set_unparsed_config(get_unparsed_config());
    return opts;
}

void OptionParser::error(const string &msg) const {
    throw ParseError(msg, parse_tree);
}

void OptionParser::document_synopsis(const string &name,
                                     const string &note) const {
    registry.set_plugin_info_synopsis(get_root_value(), name, note);
}

void OptionParser::document_property(const string &property,
                                     const string &note) const {
    registry.add_plugin_info_property(get_root_value(), property, note);
}

void OptionParser::document_language_support(
    const string &feature, const string &note) const {
    registry.add_plugin_info_feature(get_root_value(), feature, note);
}

void OptionParser::document_note(
    const string &name, const string &note, bool long_text) const {
    registry.add_plugin_info_note(get_root_value(), name, note, long_text);
}

bool OptionParser::dry_run() const {
    return dry_run_;
}

bool OptionParser::help_mode() const {
    return help_mode_;
}

const ParseTree *OptionParser::get_parse_tree() {
    return &parse_tree;
}

Registry &OptionParser::get_registry() {
    return registry;
}

const Predefinitions &OptionParser::get_predefinitions() const {
    return predefinitions;
}

const string &OptionParser::get_root_value() const {
    return parse_tree.begin()->value;
}
}
