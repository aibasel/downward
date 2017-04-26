#include "option_parser.h"

#include "doc_printer.h"
#include "errors.h"
#include "plugin.h"
#include "synergy.h"
#include "type_documenter.h"

#include "../globals.h"

#include "../ext/tree_util.hh"

#include "../utils/rng.h"
#include "../utils/system.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;


// TODO (post-issue586): Remove this once we no longer need it.
class AbstractTask;
class OpenListFactory;
class PruningMethod;
class SearchEngine;

namespace cegar {
class SubtaskGenerator;
}

namespace landmarks {
class LandmarkFactory;
}

namespace merge_and_shrink {
class LabelReduction;
class MergeScoringFunction;
class MergeSelector;
class MergeStrategyFactory;
class MergeTreeFactory;
class ShrinkStrategy;
}

namespace operator_counting {
class ConstraintGenerator;
}

namespace pdbs {
class PatternCollectionGenerator;
class PatternGenerator;
}

namespace options {
const string OptionParser::NONE = "<none>";


void OptionParser::error(const string &msg) const {
    throw ParseError(msg, parse_tree);
}

/*
  Functions for printing help:
*/

void OptionParser::set_help_mode(bool use_help_mode) {
    dry_run_ = dry_run_ && use_help_mode;
    help_mode_ = use_help_mode;
    opts.set_help_mode(use_help_mode);
}

template<typename T>
static void get_help_templ(const ParseTree &parse_tree) {
    if (Registry<T>::instance()->contains(parse_tree.begin()->value)) {
        OptionParser parser(parse_tree, true);
        parser.set_help_mode(true);
        parser.start_parsing<T>();
    }
}

static void get_help(const string &name) {
    ParseTree parse_tree;
    parse_tree.insert(parse_tree.begin(), ParseNode(name));
    get_help_templ<SearchEngine *>(parse_tree);
    get_help_templ<Evaluator *>(parse_tree);
    get_help_templ<Heuristic *>(parse_tree);
    get_help_templ<shared_ptr<AbstractTask>>(parse_tree);
    get_help_templ<Synergy *>(parse_tree);
    get_help_templ<landmarks::LandmarkFactory *>(parse_tree);
    get_help_templ<shared_ptr<cegar::SubtaskGenerator>>(parse_tree);
    get_help_templ<shared_ptr<OpenListFactory>>(parse_tree);
    get_help_templ<shared_ptr<merge_and_shrink::LabelReduction>>(parse_tree);
    get_help_templ<shared_ptr<merge_and_shrink::MergeScoringFunction>>(parse_tree);
    get_help_templ<shared_ptr<merge_and_shrink::MergeSelector>>(parse_tree);
    get_help_templ<shared_ptr<merge_and_shrink::MergeStrategyFactory>>(parse_tree);
    get_help_templ<shared_ptr<merge_and_shrink::MergeTreeFactory>>(parse_tree);
    get_help_templ<shared_ptr<merge_and_shrink::ShrinkStrategy>>(parse_tree);
    get_help_templ<shared_ptr<operator_counting::ConstraintGenerator>>(parse_tree);
    get_help_templ<shared_ptr<pdbs::PatternCollectionGenerator>>(parse_tree);
    get_help_templ<shared_ptr<pdbs::PatternGenerator>>(parse_tree);
    get_help_templ<shared_ptr<PruningMethod>>(parse_tree);
}

template<typename T>
static void get_full_help_templ() {
    DocStore::instance()->set_synopsis(
        TypeNamer<T>::name(), "", TypeDocumenter<T>::synopsis());
    for (const string &key : Registry<T>::instance()->get_sorted_keys()) {
        ParseTree parse_tree;
        parse_tree.insert(parse_tree.begin(), ParseNode(key));
        get_help_templ<T>(parse_tree);
    }
}

static void get_full_help() {
    get_full_help_templ<SearchEngine *>();
    get_full_help_templ<Evaluator *>();
    get_full_help_templ<Heuristic *>();
    get_full_help_templ<shared_ptr<AbstractTask>>();
    get_full_help_templ<Synergy *>();
    get_full_help_templ<landmarks::LandmarkFactory *>();
    get_full_help_templ<shared_ptr<cegar::SubtaskGenerator>>();
    get_full_help_templ<shared_ptr<OpenListFactory>>();
    get_full_help_templ<shared_ptr<merge_and_shrink::LabelReduction>>();
    get_full_help_templ<shared_ptr<merge_and_shrink::MergeScoringFunction>>();
    get_full_help_templ<shared_ptr<merge_and_shrink::MergeSelector>>();
    get_full_help_templ<shared_ptr<merge_and_shrink::MergeStrategyFactory>>();
    get_full_help_templ<shared_ptr<merge_and_shrink::MergeTreeFactory>>();
    get_full_help_templ<shared_ptr<merge_and_shrink::ShrinkStrategy>>();
    get_full_help_templ<shared_ptr<operator_counting::ConstraintGenerator>>();
    get_full_help_templ<shared_ptr<pdbs::PatternCollectionGenerator>>();
    get_full_help_templ<shared_ptr<pdbs::PatternGenerator>>();
    get_full_help_templ<shared_ptr<PruningMethod>>();
}


/*
  Predefine landmarks and heuristics.
*/

/* Convert a string of the form "word1, word2, word3" and convert it to a vector.
   (used for predefining synergies) */
static vector<string> to_list(const string &s) {
    vector<string> result;
    string buffer;
    for (char c : s) {
        if (c == ',') {
            result.push_back(buffer);
            buffer.clear();
        } else if (c == ' ') {
            continue;
        } else {
            buffer.push_back(c);
        }
    }
    result.push_back(buffer);
    return result;
}

// TODO: Update this function when we get rid of the Synergy object.
static void predefine_heuristic(const string &arg, bool dry_run) {
    size_t split_pos = arg.find("=");
    string lhs = arg.substr(0, split_pos);
    vector<string> definees = to_list(lhs);
    string rhs = arg.substr(split_pos + 1);
    OptionParser parser(rhs, dry_run);
    if (definees.size() == 1) {
        // Normal predefinition
        Predefinitions<Heuristic *>::instance()->predefine(
            definees[0], parser.start_parsing<Heuristic *>());
    } else if (definees.size() > 1) {
        // Synergy
        if (!dry_run) {
            vector<Heuristic *> heur = parser.start_parsing<Synergy *>()->heuristics;
            for (size_t i = 0; i < definees.size(); ++i) {
                Predefinitions<Heuristic *>::instance()->predefine(
                    definees[i], heur[i]);
            }
        } else {
            for (const string &definee : definees) {
                Predefinitions<Heuristic *>::instance()->predefine(
                    definee, nullptr);
            }
        }
    } else {
        parser.error("predefinition has invalid left side");
    }
}

static void predefine_lmgraph(const string &arg, bool dry_run) {
    size_t split_pos = arg.find("=");
    string lhs = arg.substr(0, split_pos);
    vector<string> definees = to_list(lhs);
    string rhs = arg.substr(split_pos + 1);
    OptionParser op(rhs, dry_run);
    if (definees.size() == 1) {
        Predefinitions<landmarks::LandmarkFactory *>::instance()->predefine(
            definees[0], op.start_parsing<landmarks::LandmarkFactory *>());
    } else {
        op.error("predefinition has invalid left side");
    }
}


template<class T>
void _check_bounds(
    OptionParser &parser, const string &key, T value,
    T lower_bound, T upper_bound) {
    if (lower_bound > upper_bound)
        ABORT("lower bound is greater than upper bound for " + key);
    if (value < lower_bound || value > upper_bound) {
        stringstream stream;
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
        OptionParser bound_parser(bounds.min, dry_run());
        lower_bound = TokenParser<int>::parse(bound_parser);
    }
    if (!bounds.max.empty()) {
        OptionParser bound_parser(bounds.max, dry_run());
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
        OptionParser bound_parser(bounds.min, dry_run());
        lower_bound = TokenParser<double>::parse(bound_parser);
    }
    if (!bounds.max.empty()) {
        OptionParser bound_parser(bounds.max, dry_run());
        upper_bound = TokenParser<double>::parse(bound_parser);
    }
    _check_bounds(*this, key, value, lower_bound, upper_bound);
}

SearchEngine *OptionParser::parse_cmd_line(
    int argc, const char **argv, bool dry_run, bool is_unit_cost) {
    vector<string> args;
    bool active = true;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];

        // Ignore case.
        transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

        // Sanitize argument by removing newlines.
        arg.erase(remove(arg.begin(), arg.end(), '\n'), arg.end());

        if (arg == "--if-unit-cost") {
            active = is_unit_cost;
        } else if (arg == "--if-non-unit-cost") {
            active = !is_unit_cost;
        } else if (arg == "--always") {
            active = true;
        } else if (active) {
            args.push_back(arg);
        }
    }
    return parse_cmd_line_aux(args, dry_run);
}


int OptionParser::parse_int_arg(const string &name, const string &value) {
    try {
        return stoi(value);
    } catch (invalid_argument &) {
        throw ArgError("argument for " + name + " must be an integer");
    } catch (out_of_range &) {
        throw ArgError("argument for " + name + " is out of range");
    }
}


SearchEngine *OptionParser::parse_cmd_line_aux(
    const vector<string> &args, bool dry_run) {
    SearchEngine *engine(0);
    // TODO: Remove code duplication.
    for (size_t i = 0; i < args.size(); ++i) {
        string arg = args[i];
        bool is_last = (i == args.size() - 1);
        if (arg.compare("--heuristic") == 0) {
            if (is_last)
                throw ArgError("missing argument after --heuristic");
            ++i;
            predefine_heuristic(args[i], dry_run);
        } else if (arg.compare("--landmarks") == 0) {
            if (is_last)
                throw ArgError("missing argument after --landmarks");
            ++i;
            predefine_lmgraph(args[i], dry_run);
        } else if (arg.compare("--search") == 0) {
            if (is_last)
                throw ArgError("missing argument after --search");
            ++i;
            OptionParser p(args[i], dry_run);
            engine = p.start_parsing<SearchEngine *>();
        } else if ((arg.compare("--help") == 0) && dry_run) {
            cout << "Help:" << endl;
            bool txt2tags = false;
            vector<string> helpiands;
            for (size_t j = i + 1; j < args.size(); ++j) {
                if (args[j] == "--txt2tags") {
                    txt2tags = true;
                } else {
                    helpiands.push_back(string(args[j]));
                }
            }
            if (helpiands.empty()) {
                get_full_help();
            } else {
                for (const string &helpiand : helpiands) {
                    get_help(helpiand);
                }
            }
            unique_ptr<DocPrinter> doc_printer;
            if (txt2tags)
                doc_printer = utils::make_unique_ptr<Txt2TagsPrinter>(cout);
            else
                doc_printer = utils::make_unique_ptr<PlainPrinter>(cout);
            doc_printer->print_all();
            cout << "Help output finished." << endl;
            exit(0);
        } else if (arg.compare("--internal-plan-file") == 0) {
            if (is_last)
                throw ArgError("missing argument after --internal-plan-file");
            ++i;
            g_plan_filename = args[i];
        } else if (arg.compare("--internal-previous-portfolio-plans") == 0) {
            if (is_last)
                throw ArgError("missing argument after --internal-previous-portfolio-plans");
            ++i;
            g_is_part_of_anytime_portfolio = true;
            g_num_previously_generated_plans = parse_int_arg(arg, args[i]);
            if (g_num_previously_generated_plans < 0)
                throw ArgError("argument for --internal-previous-portfolio-plans must be positive");
        } else {
            throw ArgError("unknown option " + arg);
        }
    }
    return engine;
}

string OptionParser::usage(const string &progname) {
    return "usage: \n" +
           progname + " [OPTIONS] --search SEARCH < OUTPUT\n\n"
           "* SEARCH (SearchEngine): configuration of the search algorithm\n"
           "* OUTPUT (filename): translator output\n\n"
           "Options:\n"
           "--help [NAME]\n"
           "    Prints help for all heuristics, open lists, etc. called NAME.\n"
           "    Without parameter: prints help for everything available\n"
           "--landmarks LANDMARKS_PREDEFINITION\n"
           "    Predefines a set of landmarks that can afterwards be referenced\n"
           "    by the name that is specified in the definition.\n"
           "--heuristic HEURISTIC_PREDEFINITION\n"
           "    Predefines a heuristic that can afterwards be referenced\n"
           "    by the name that is specified in the definition.\n"
           "--internal-plan-file FILENAME\n"
           "    Plan will be output to a file called FILENAME\n\n"
           "--internal-previous-portfolio-plans COUNTER\n"
           "    This planner call is part of a portfolio which already created\n"
           "    plan files FILENAME.1 up to FILENAME.COUNTER.\n"
           "    Start enumerating plan files with COUNTER+1, i.e. FILENAME.COUNTER+1\n\n"
           "See http://www.fast-downward.org/ for details.";
}


static ParseTree generate_parse_tree(const string &config) {
    ParseTree tree;
    ParseTree::sibling_iterator pseudoroot =
        tree.insert(tree.begin(), ParseNode("pseudoroot", ""));
    ParseTree::sibling_iterator cur_node = pseudoroot;
    string buffer = "";
    string key = "";
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
            if (cur_node->value.compare("list") != 0) {
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
    if (cur_node->value.compare("pseudoroot") != 0)
        throw ParseError("missing )", *cur_node);
    if (!buffer.empty())
        tree.append_child(cur_node, ParseNode(buffer, key));

    // The real parse tree is the first (and only) child of the pseudoroot placeholder.
    return subtree(tree, tree.begin(pseudoroot));
}

OptionParser::OptionParser(const string &config, bool dry_run)
    : opts(false),
      parse_tree(generate_parse_tree(config)),
      dry_run_(dry_run),
      help_mode_(false),
      next_unparsed_argument(first_child_of_root(parse_tree)) {
}


OptionParser::OptionParser(ParseTree pt, bool dry_run)
    : opts(false),
      parse_tree(pt),
      dry_run_(dry_run),
      help_mode_(false),
      next_unparsed_argument(first_child_of_root(parse_tree)) {
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
        for (size_t i = 0; i < names.size(); ++i) {
            // TODO: Either provide help for all or none of the values.
            if (docs.size() > i) {
                value_explanations.emplace_back(names[i], docs[i]);
            }
        }

        DocStore::instance()->add_arg(
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
    stringstream str_stream(value);
    int choice;
    if (!(str_stream >> choice).fail()) {
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
    //check if there were any arguments with invalid keywords,
    //or positional arguments after keyword arguments
    string last_key = "";
    for (ParseTree::sibling_iterator pti = first_child_of_root(parse_tree);
         pti != end_of_roots_children(parse_tree); ++pti) {
        if (pti->key.compare("") != 0) {
            bool valid_key = false;
            for (size_t i = 0; i < valid_keys.size(); ++i) {
                if (valid_keys[i].compare(pti->key) == 0) {
                    valid_key = true;
                    break;
                }
            }
            if (!valid_key) {
                error("invalid keyword " + pti->key + " for " + get_root_value());
            }
        }
        if (pti->key.compare("") == 0 && last_key.compare("") != 0) {
            error("positional argument after keyword argument");
        }
        last_key = pti->key;
    }
    opts.set_unparsed_config(get_unparsed_config());
    return opts;
}

void OptionParser::document_values(
    const string &argument,
    const ValueExplanations &value_explanations) const {
    DocStore::instance()->add_value_explanations(
        get_root_value(), argument, value_explanations);
}

void OptionParser::document_synopsis(const string &name, const string &note) const {
    DocStore::instance()->set_synopsis(get_root_value(), name, note);
}

void OptionParser::document_property(const string &property, const string &note) const {
    DocStore::instance()->add_property(get_root_value(), property, note);
}

void OptionParser::document_language_support(
    const string &feature, const string &note) const {
    DocStore::instance()->add_feature(get_root_value(), feature, note);
}

void OptionParser::document_note(
    const string &name, const string &note, bool long_text) const {
    DocStore::instance()->add_note(get_root_value(), name, note, long_text);
}

void OptionParser::document_hide() const {
    DocStore::instance()->hide(get_root_value());
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

const string &OptionParser::get_root_value() const {
    return parse_tree.begin()->value;
}
}
