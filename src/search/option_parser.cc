#include "globals.h"
#include "option_parser.h"
#include "ext/tree_util.hh"
#include "plugin.h"
#include "rng.h"
#include <string>
#include <algorithm>
#include <iostream>

using namespace std;

ParseError::ParseError(string m, ParseTree pt)
    : msg(m),
      parse_tree(pt) {
}

ParseError::ParseError(string m, ParseTree pt, string correct_substring)
    : msg(m),
      parse_tree(pt),
      substr(correct_substring) {
}

HelpElement::HelpElement(string k, string h, string t_n)
    : kwd(k),
      help(h),
      type_name(t_n) {
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
        cout << pt.begin()->value << " is a " << TypeNamer<T>::name()
             << endl << "Usage: " << endl;
        OptionParser p(pt, true);
        p.set_help_mode(true);
        p.start_parsing<T>();
        cout << endl;
    }
}

static void get_help(string k) {
    ParseTree pt;
    pt.insert(pt.begin(), ParseNode(k));
    get_help_templ<SearchEngine *>(pt);
    get_help_templ<Heuristic *>(pt);
    get_help_templ<ScalarEvaluator *>(pt);
    get_help_templ<Synergy *>(pt);
    get_help_templ<LandmarkGraph *>(pt);
    Plugin<OpenList<int> >::register_open_lists();
    get_help_templ<OpenList<int> *>(pt);
    get_help_templ<ShrinkStrategy *>(pt);
}

template <class T>
static void get_full_help_templ() {
    cout << endl << "Help for " << TypeNamer<T>::name() << "s:" << endl << endl;
    vector<string> keys = Registry<T>::instance()->get_keys();
    for (size_t i(0); i != keys.size(); ++i) {
        ParseTree pt;
        pt.insert(pt.begin(), ParseNode(keys[i]));
        get_help_templ<T>(pt);
    }
}

static void get_full_help() {
    get_full_help_templ<SearchEngine *>();
    get_full_help_templ<Heuristic *>();
    get_full_help_templ<ScalarEvaluator *>();
    get_full_help_templ<Synergy *>();
    get_full_help_templ<LandmarkGraph *>();
    Plugin<OpenList<int> >::register_open_lists();
    get_full_help_templ<OpenList<int> *>();
    get_full_help_templ<ShrinkStrategy *>();
}


/*
Predefining landmarks and heuristics:
*/

//takes a string of the form "word1, word2, word3 " and converts it to a vector
//(used for predefining synergies)
static std::vector<std::string> to_list(std::string s) {
    std::vector<std::string> result;
    std::string buffer;
    for (size_t i(0); i != s.size(); ++i) {
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
        Predefinitions<Heuristic * >::instance()->predefine(
            definees[0], op.start_parsing<Heuristic *>());
    } else if (definees.size() > 1) { //synergy
        if (!dry_run) {
            std::vector<Heuristic *> heur =
                op.start_parsing<Synergy *>()->heuristics;
            for (size_t i(0); i != definees.size(); ++i) {
                Predefinitions<Heuristic *>::instance()->predefine(
                    definees[i], heur[i]);
            }
        } else {
            for (size_t i(0); i != definees.size(); ++i) {
                Predefinitions<Heuristic *>::instance()->predefine(
                    definees[i], 0);
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

SearchEngine *OptionParser::parse_cmd_line(
    int argc, const char **argv, bool dry_run) {
    SearchEngine *engine(0);
    for (int i = 1; i < argc; ++i) {
        string arg = string(argv[i]);
        if (arg.compare("--heuristic") == 0) {
            ++i;
            predefine_heuristic(argv[i], dry_run);
        } else if (arg.compare("--landmarks") == 0) {
            ++i;
            predefine_lmgraph(argv[i], dry_run);
        } else if (arg.compare("--search") == 0) {
            ++i;
            OptionParser p(argv[i], dry_run);
            engine = p.start_parsing<SearchEngine *>();
        } else if (arg.compare("--random-seed") == 0) {
            ++i;
            srand(atoi(argv[i]));
            g_rng.seed(atoi(argv[i]));
            cout << "random seed " << argv[i] << endl;
        } else if ((arg.compare("--help") == 0) && dry_run) {
            cout << "Help:" << endl;
            if (i + 1 < argc) {
                string helpiand = string(argv[i + 1]);
                get_help(helpiand);
            } else {
                get_full_help();
            }
            cout << "Help output finished." << endl;
            exit(0);
        } else if (arg.compare("--plan-file") == 0) {
            ++i;
            g_plan_filename = argv[i];
        } else {
            cerr << "unknown option " << arg << endl << endl;
            cout << OptionParser::usage(argv[0]) << endl;
            exit_with(EXIT_INPUT_ERROR);
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
        "--plan-file FILENAME\n"
        "    Plan will be output to a file called FILENAME\n\n"
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
    for (size_t i(0); i != config.size(); ++i) {
        next = config.at(i);
        if ((next == '(' || next == ')' || next == ',') && buffer.size() > 0) {
            tr.append_child(cur_node, ParseNode(buffer, key));
            buffer.clear();
            key.clear();
        } else if(next == '(' && buffer.size() == 0) {
            throw ParseError("misplaced opening bracket (", *cur_node, config.substr(0,i));
        }
        switch (next) {
        case ' ':
            break;
        case '(':
            cur_node = last_child(tr, cur_node);
            break;
        case ')':
            if (cur_node == pseudoroot)
                throw ParseError("missing (", *cur_node, config.substr(0,i));
            cur_node = tr.parent(cur_node);
            break;
        case '[':
            if (!buffer.empty())
                throw ParseError("misplaced opening bracket [", *cur_node, config.substr(0,i));
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
                throw ParseError("mismatched brackets", *cur_node, config.substr(0,i));
            }
            cur_node = tr.parent(cur_node);
            break;
        case ',':
            break;
        case '=':
            if (buffer.empty())
                throw ParseError("expected keyword before =", *cur_node, config.substr(0,i));
            key = buffer;
            buffer.clear();
            break;
        default:
            buffer.push_back(tolower(next));
            break;
        }
    }
    if (next != ')')
        throw ParseError("expected ) at end of configuration after " + buffer, *cur_node);
    if (cur_node->value.compare("pseudoroot") != 0)
        throw ParseError("missing )", *cur_node);

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
}


OptionParser::OptionParser(ParseTree pt, bool dr)
    : opts(false),
      parse_tree(pt),
      dry_run_(dr),
      help_mode_(false),
      next_unparsed_argument(first_child_of_root(parse_tree)) {
}


string str_to_lower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

void OptionParser::add_enum_option(string k,
                                   vector<string > enumeration,
                                   string def_val, string h,
                                   OptionFlags flags) {
    if (help_mode_) {
        string enum_descr = "{";
        for (size_t i(0); i != enumeration.size(); ++i) {
            enum_descr += enumeration[i];
            if (i != enumeration.size() - 1) {
                enum_descr += ", ";
            }
        }
        enum_descr += "}";

        helpers.push_back(HelpElement(k, h, enum_descr));
        if (def_val.compare("") != 0) {
            helpers.back().default_value = def_val;
        }
        return;
    }

    //enum arguments can be given by name or by number:
    //first parse the corresponding string like a normal argument...
    if (def_val.compare("") != 0) {
        add_option<string>(k, def_val, h);
    } else {
        add_option<string>(k, h, flags.mandatory);
    }

    if (!flags.mandatory && !opts.contains(k))
        return;

    string name = str_to_lower(opts.get<string>(k));

    //...then check if the parsed string can be treated as a number
    stringstream str_stream(name);
    int x;
    if (!(str_stream >> x).fail()) {
        if (x > enumeration.size()) {
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
    if (help_mode_) {
        //print out collected help information
        cout << parse_tree.begin()->value << "(";
        for (size_t i(0); i != helpers.size(); ++i) {
            cout << helpers[i].kwd
                 << (helpers[i].default_value.compare("") != 0 ? " = " : "")
                 << helpers[i].default_value;
            if (i != helpers.size() - 1) {
                cout << ", ";
            }
        }
        cout << ")" << endl;
        for (size_t i(0); i != helpers.size(); ++i) {
            cout << helpers[i].kwd << "(" << helpers[i].type_name << "): "
                 << helpers[i].help << endl;
        }
    }
    //check if there were any arguments with invalid keywords,
    //or positional arguments after keyword arguments
    string last_key = "";
    for (ParseTree::sibling_iterator pti = first_child_of_root(parse_tree);
         pti != end_of_roots_children(parse_tree); ++pti) {
        if (pti->key.compare("") != 0) {
            bool valid_key = false;
            for (size_t i(0); i != valid_keys.size(); ++i) {
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
    return opts;
}

bool OptionParser::dry_run() const {
    return dry_run_;
}

bool OptionParser::help_mode() const {
    return help_mode_;
}

void OptionParser::set_parse_tree(const ParseTree &pt) {
    parse_tree = pt;
}

ParseTree *OptionParser::get_parse_tree() {
    return &parse_tree;
}
