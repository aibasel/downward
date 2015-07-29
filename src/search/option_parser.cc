#include "globals.h"
#include "option_parser.h"
#include "ext/tree_util.hh"
#include "plugin.h"
#include "rng.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;


const string OptionParser::NONE = "<none>";


ArgError::ArgError(std::string msg_) : msg(msg_) {
}


ParseError::ParseError(string m, ParseTree pt)
    : msg(m),
      parse_tree(pt) {
}

ParseError::ParseError(string m, ParseTree pt, string correct_substring)
    : msg(m),
      parse_tree(pt),
      substr(correct_substring) {
}

void OptionParser::error(string msg) {
    throw ParseError(msg, *this->get_parse_tree());
}

void OptionParser::warning(string msg) {
    cout << "Parser Warning: " << msg << endl;
}

/*
Functions for printing help:
*/

void OptionParser::set_help_mode(bool m) {
    dry_run_ = dry_run_ && m;
    help_mode_ = m;
    opts.set_help_mode(m);
}

template <class T>
static void get_help_templ(const ParseTree &pt) {
    if (Registry<T>::instance()->contains(pt.begin()->value)) {
        OptionParser p(pt, true);
        p.set_help_mode(true);
        p.start_parsing<T>();
    }
}

static void get_help(string k) {
    ParseTree pt;
    pt.insert(pt.begin(), ParseNode(k));
    get_help_templ<SearchEngine *>(pt);
    get_help_templ<Heuristic *>(pt);
    get_help_templ<shared_ptr<AbstractTask> >(pt);
    get_help_templ<ScalarEvaluator *>(pt);
    get_help_templ<Synergy *>(pt);
    get_help_templ<LandmarkGraph *>(pt);
    Plugin<OpenList<int> >::register_open_lists();
    get_help_templ<OpenList<int> *>(pt);
    get_help_templ<shared_ptr<MergeStrategy> >(pt);
    get_help_templ<shared_ptr<ShrinkStrategy> >(pt);
    get_help_templ<shared_ptr<Labels> >(pt);
}

template <class T>
static void get_full_help_templ() {
    DocStore::instance()->set_synopsis(TypeNamer<T>::name(), "",
                                       TypeDocumenter<T>::synopsis());
    vector<string> keys = Registry<T>::instance()->get_keys();
    for (size_t i = 0; i < keys.size(); ++i) {
        ParseTree pt;
        pt.insert(pt.begin(), ParseNode(keys[i]));
        get_help_templ<T>(pt);
    }
}

static void get_full_help() {
    get_full_help_templ<SearchEngine *>();
    get_full_help_templ<Heuristic *>();
    get_full_help_templ<shared_ptr<AbstractTask> >();
    get_full_help_templ<ScalarEvaluator *>();
    get_full_help_templ<Synergy *>();
    get_full_help_templ<LandmarkGraph *>();
    Plugin<OpenList<int> >::register_open_lists();
    get_full_help_templ<OpenList<int> *>();
    get_full_help_templ<shared_ptr<MergeStrategy> >();
    get_full_help_templ<shared_ptr<ShrinkStrategy> >();
    get_full_help_templ<shared_ptr<Labels> >();
}


/*
Predefining landmarks and heuristics:
*/

//takes a string of the form "word1, word2, word3 " and converts it to a vector
//(used for predefining synergies)
static std::vector<std::string> to_list(std::string s) {
    std::vector<std::string> result;
    std::string buffer;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == ',') {
            result.push_back(buffer);
            buffer.clear();
        } else if (s[i] == ' ') {
            continue;
        } else {
            buffer.push_back(s[i]);
        }
    }
    result.push_back(buffer);
    return result;
}

//Note: originally the following function was templated (predefine<T>),
//but there is no Synergy<LandmarkGraph>, so I split it up for now.
static void predefine_heuristic(std::string s, bool dry_run) {
    //remove newlines so they don't mess anything up:
    s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());

    size_t split = s.find("=");
    std::string ls = s.substr(0, split);
    std::vector<std::string> definees = to_list(ls);
    std::string rs = s.substr(split + 1);
    OptionParser op(rs, dry_run);
    if (definees.size() == 1) { //normal predefinition
        Predefinitions<Heuristic *>::instance()->predefine(
            definees[0], op.start_parsing<Heuristic *>());
    } else if (definees.size() > 1) { //synergy
        if (!dry_run) {
            std::vector<Heuristic *> heur =
                op.start_parsing<Synergy *>()->heuristics;
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
        op.error("predefinition has invalid left side");
    }
}

static void predefine_lmgraph(std::string s, bool dry_run) {
    //remove newlines so they don't mess anything up:
    s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());

    size_t split = s.find("=");
    std::string ls = s.substr(0, split);
    std::vector<std::string> definees = to_list(ls);
    std::string rs = s.substr(split + 1);
    OptionParser op(rs, dry_run);
    if (definees.size() == 1) {
        Predefinitions<LandmarkGraph *>::instance()->predefine(
            definees[0], op.start_parsing<LandmarkGraph *>());
    } else {
        op.error("predefinition has invalid left side");
    }
}


/*
Parse command line options
*/

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
        } else if (arg.compare("--random-seed") == 0) {
            if (is_last)
                throw ArgError("missing argument after --random-seed");
            ++i;
            int seed = parse_int_arg(arg, args[i]);
            g_rng.seed(seed);
            cout << "random seed: " << seed << endl;
        } else if ((arg.compare("--help") == 0) && dry_run) {
            cout << "Help:" << endl;
            bool txt2tags = false;
            vector<string> helpiands;
            if (i + 1 < args.size()) {
                for (size_t j = i + 1; j < args.size(); ++j) {
                    if (args[j] == "--txt2tags") {
                        txt2tags = true;
                    } else {
                        helpiands.push_back(string(args[j]));
                    }
                }
            }
            if (helpiands.empty()) {
                get_full_help();
            } else {
                for (size_t j = 0; j != helpiands.size(); ++j) {
                    get_help(helpiands[j]);
                }
            }
            DocPrinter *dp;
            if (txt2tags) {
                dp = new Txt2TagsPrinter(cout);
            } else {
                dp = new PlainPrinter(cout);
            }
            dp->print_all();
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

string OptionParser::usage(string progname) {
    string usage =
        "usage: \n" +
        progname + " [OPTIONS] --search SEARCH < OUTPUT\n\n"
        "* SEARCH (SearchEngine): configuration of the search algorithm\n"
        "* OUTPUT (filename): preprocessor output\n\n"
        "Options:\n"
        "--help [NAME]\n"
        "    Prints help for all heuristics, openlists, etc. called NAME.\n"
        "    Without parameter: prints help for everything available\n"
        "--landmarks LANDMARKS_PREDEFINITION\n"
        "    Predefines a set of landmarks that can afterwards be referenced\n"
        "    by the name that is specified in the definition.\n"
        "--heuristic HEURISTIC_PREDEFINITION\n"
        "    Predefines a heuristic that can afterwards be referenced\n"
        "    by the name that is specified in the definition.\n"
        "--random-seed SEED\n"
        "    Use random seed SEED\n\n"
        "--internal-plan-file FILENAME\n"
        "    Plan will be output to a file called FILENAME\n\n"
        "--internal-previous-portfolio-plans COUNTER\n"
        "    This planner call is part of a portfolio which already created\n"
        "    plan files FILENAME.1 up to FILENAME.COUNTER.\n"
        "    Start enumerating plan files with COUNTER+1, i.e. FILENAME.COUNTER+1\n\n"
        "See http://www.fast-downward.org/ for details.";
    return usage;
}


static ParseTree generate_parse_tree(string config) {
    //remove newlines so they don't mess anything up:
    config.erase(std::remove(config.begin(), config.end(), '\n'), config.end());

    ParseTree tr;
    ParseTree::iterator top = tr.begin();
    ParseTree::sibling_iterator pseudoroot =
        tr.insert(top, ParseNode("pseudoroot", ""));
    ParseTree::sibling_iterator cur_node = pseudoroot;
    string buffer(""), key("");
    char next = ' ';
    for (size_t i = 0; i < config.size(); ++i) {
        next = config.at(i);
        if ((next == '(' || next == ')' || next == ',') && buffer.size() > 0) {
            tr.append_child(cur_node, ParseNode(buffer, key));
            buffer.clear();
            key.clear();
        } else if (next == '(' && buffer.size() == 0) {
            throw ParseError("misplaced opening bracket (", *cur_node, config.substr(0, i));
        }
        switch (next) {
        case ' ':
            break;
        case '(':
            cur_node = last_child(tr, cur_node);
            break;
        case ')':
            if (cur_node == pseudoroot)
                throw ParseError("missing (", *cur_node, config.substr(0, i));
            cur_node = tr.parent(cur_node);
            break;
        case '[':
            if (!buffer.empty())
                throw ParseError("misplaced opening bracket [", *cur_node, config.substr(0, i));
            tr.append_child(cur_node, ParseNode("list", key));
            key.clear();
            cur_node = last_child(tr, cur_node);
            break;
        case ']':
            if (!buffer.empty()) {
                tr.append_child(cur_node, ParseNode(buffer, key));
                buffer.clear();
                key.clear();
            }
            if (cur_node->value.compare("list") != 0) {
                throw ParseError("mismatched brackets", *cur_node, config.substr(0, i));
            }
            cur_node = tr.parent(cur_node);
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
            buffer.push_back(tolower(next));
            break;
        }
    }
    if (cur_node->value.compare("pseudoroot") != 0)
        throw ParseError("missing )", *cur_node);
    if (buffer.size() > 0)
        tr.append_child(cur_node, ParseNode(buffer, key));


    //the real parse tree is the first (and only) child of the pseudoroot.
    //pseudoroot is only a placeholder.
    ParseTree real_tr = subtree(tr, tr.begin(pseudoroot));
    return real_tr;
}

OptionParser::OptionParser(const string config, bool dr)
    : opts(false),
      parse_tree(generate_parse_tree(config)),
      dry_run_(dr),
      help_mode_(false),
      next_unparsed_argument(first_child_of_root(parse_tree)) {
    set_unparsed_config();
}


OptionParser::OptionParser(ParseTree pt, bool dr)
    : opts(false),
      parse_tree(pt),
      dry_run_(dr),
      help_mode_(false),
      next_unparsed_argument(first_child_of_root(parse_tree)) {
    set_unparsed_config();
}

void OptionParser::set_unparsed_config() {
    ostringstream stream;
    kptree::print_tree_bracketed<ParseNode>(parse_tree, stream);
    unparsed_config = stream.str();
}

static string str_to_lower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

void OptionParser::add_enum_option(string k,
                                   vector<string > enumeration,
                                   string h, string def_val,
                                   vector<string> enum_docs) {
    if (help_mode_) {
        ValueExplanations value_explanations;
        string enum_descr = "{";
        for (size_t i = 0; i < enumeration.size(); ++i) {
            enum_descr += enumeration[i];
            if (i != enumeration.size() - 1) {
                enum_descr += ", ";
            }
            if (enum_docs.size() > i) {
                value_explanations.push_back(make_pair(enumeration[i],
                                                       enum_docs[i]));
            }
        }
        enum_descr += "}";

        DocStore::instance()->add_arg(parse_tree.begin()->value,
                                      k, h,
                                      enum_descr, def_val,
                                      Bounds::unlimited(),
                                      value_explanations);
        return;
    }

    //enum arguments can be given by name or by number:
    //first parse the corresponding string like a normal argument...
    add_option<string>(k, h, def_val);

    if (!opts.contains(k))
        return;

    string name = str_to_lower(opts.get<string>(k));

    //...then check if the parsed string can be treated as a number
    stringstream str_stream(name);
    int x;
    if (!(str_stream >> x).fail()) {
        int max_choice = enumeration.size();
        if (x > max_choice) {
            error("invalid enum argument " + name
                  + " for option " + k);
        }
        opts.set<int>(k, x);
    } else {
        //...otherwise try to map the string to its position in the enumeration vector
        transform(enumeration.begin(), enumeration.end(), enumeration.begin(),
                  str_to_lower); //make the enumeration lower case
        vector<string>::const_iterator it =
            find(enumeration.begin(), enumeration.end(), name);
        if (it == enumeration.end()) {
            error("invalid enum argument " + name
                  + " for option " + k);
        }
        opts.set<int>(k, it - enumeration.begin());
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
                error("invalid keyword "
                      + pti->key + " for "
                      + parse_tree.begin()->value);
            }
        }
        if (pti->key.compare("") == 0 &&
            last_key.compare("") != 0) {
            error("positional argument after keyword argument");
        }
        last_key = pti->key;
    }
    opts.set_unparsed_config(unparsed_config);
    return opts;
}

bool OptionParser::is_valid_option(const std::string &k) const {
    assert(!help_mode());
    return find(valid_keys.begin(), valid_keys.end(), k) != valid_keys.end();
}

void OptionParser::document_values(string argument,
                                   ValueExplanations value_explanations) const {
    DocStore::instance()->add_value_explanations(
        parse_tree.begin()->value,
        argument, value_explanations);
}

void OptionParser::document_synopsis(string name, string note) const {
    DocStore::instance()->set_synopsis(parse_tree.begin()->value,
                                       name, note);
}

void OptionParser::document_property(string property, string note) const {
    DocStore::instance()->add_property(parse_tree.begin()->value,
                                       property, note);
}

void OptionParser::document_language_support(string feature,
                                             string note) const {
    DocStore::instance()->add_feature(parse_tree.begin()->value,
                                      feature, note);
}

void OptionParser::document_note(string name,
                                 string note, bool long_text) const {
    DocStore::instance()->add_note(parse_tree.begin()->value,
                                   name, note, long_text);
}

void OptionParser::document_hide() const {
    DocStore::instance()->hide(parse_tree.begin()->value);
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
