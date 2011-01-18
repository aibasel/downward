#include "option_parser.h"

#include "heuristic.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

OptionParser *OptionParser::instance_ = 0;

OptionParser *OptionParser::instance() {
    if (instance_ == 0)
        instance_ = new OptionParser();
    return instance_;
}

void OptionParser::register_search_engine(const string &key,
                                          EngineFactory func) {
    engine_map[key] = func;
}

void OptionParser::register_scalar_evaluator(const string &key,
                                             ScalarEvalFactory func) {
    scalar_evaluator_map[key] = func;
}

void OptionParser::register_lm_graph_factory(const std::string &key,
                                             LandmarkGraphFactory func) {
    lm_graph_map[key] = func;
}


void OptionParser::register_synergy(const string &key, SynergyFactory func) {
    synergy_map[key] = func;
}


ScalarEvaluator *OptionParser::parse_scalar_evaluator(
    const vector<string> &input, int start, int &end, bool dry_run) {
    // predefined evaluator
    map<string, Heuristic *>::iterator iter;
    iter = predefined_heuristics.find(input[start]);
    if (iter != predefined_heuristics.end()) {
        end = start;
        return (ScalarEvaluator *)iter->second;
    }

    // scalar evaluater definition
    map<string, ScalarEvalFactory>::iterator it;
    it = scalar_evaluator_map.find(input[start]);
    if (it == scalar_evaluator_map.end())
        throw ParseError(start);
    return it->second(input, start, end, dry_run);
}

LandmarksGraph *OptionParser::parse_lm_graph(
    const vector<string> &input, int start, int &end, bool dry_run) {
    // predefined object
    map<string, LandmarksGraph *>::iterator iter;
    iter = predefined_lm_graphs.find(input[start]);
    if (iter != predefined_lm_graphs.end()) {
        end = start;
        return iter->second;
    }

    // object definition
    map<string, LandmarkGraphFactory>::iterator it;
    it = lm_graph_map.find(input[start]);
    if (it == lm_graph_map.end())
        throw ParseError(start);
    return it->second(input, start, end, dry_run);
}



Heuristic *OptionParser::parse_heuristic(const vector<string> &input,
                                         int start, int &end, bool dry_run) {
    ScalarEvaluator *eval = parse_scalar_evaluator(input, start, end, dry_run);
    Heuristic *h = dynamic_cast<Heuristic *>(eval); // HACK?
    if (!dry_run && h == 0)
        throw ParseError(start);
    // FIXME: With dry_run we need a different implementation to
    // check that we would get a heuristic
    return h;
}

void OptionParser::parse_synergy_heuristics(const vector<string> &input,
                                            int start, int &end,
                                            vector<Heuristic *> &heuristics) {
    map<string, SynergyFactory>::iterator it;
    it = synergy_map.find(input[start]);
    if (it == synergy_map.end())
        throw ParseError(start);
    it->second(input, start, end, heuristics);
}

bool OptionParser::knows_scalar_evaluator(const string &name) const {
    return scalar_evaluator_map.find(name) != scalar_evaluator_map.end() ||
           predefined_heuristics.find(name) != predefined_heuristics.end();
}

bool OptionParser::knows_lm_graph(const string &name) const {
    return lm_graph_map.count(name) || predefined_lm_graphs.count(name);
}

bool OptionParser::knows_search_engine(const string &name) const {
    return engine_map.find(name) != engine_map.end();
}

SearchEngine *OptionParser::parse_search_engine(const char *str) {
    vector<string> tokens;
    tokenize_options(str, tokens);

    SearchEngine *engine;
    try {
        int end = 0;
        engine = parse_search_engine(tokens, 0, end, false);
    } catch (ParseError &e) {
        print_parse_error(tokens, e);
        exit(2);
    }
    return engine;
}

SearchEngine *OptionParser::parse_search_engine(
    const vector<string> &input, int start, int &end, bool dry_run) {
    map<string, EngineFactory>::iterator it;
    it = engine_map.find(input[start]);
    if (it == engine_map.end())
        throw ParseError(start);
    return it->second(input, start, end, dry_run);
}

void OptionParser::predefine_heuristic(const char *str) {
    vector<string> tokens;
    tokenize_options(str, tokens);
    try {
        predefine_heuristic(tokens);
    } catch (ParseError &e) {
        print_parse_error(tokens, e);
        exit(2);
    }
}

void OptionParser::predefine_heuristic(const vector<string> &input) {
    if (input[1].compare("=") == 0) {
        // define one heuristic (standard case)
        string name = input[0];
        if (knows_scalar_evaluator(name) || name == "(" || name == ")")
            throw ParseError(0);
        int end = 2;
        predefined_heuristics[name] = parse_heuristic(input, 2, end, false);
    } else if (input[1].compare(",") == 0) {
        // define several heuristics (currently only lama ff synergy)

        vector<string>::const_iterator it = find(input.begin(),
                                                 input.end(), "=");
        if (it == input.end())
            throw ParseError(input.size());
        int heuristic_pos = it - input.begin() + 1;

        vector<Heuristic *> heuristics;
        int end = 0;
        parse_synergy_heuristics(input, heuristic_pos, end, heuristics);

        for (unsigned int i = 0; i < heuristics.size(); i++) {
            int name_pos = 2 * i;
            string name = input[name_pos];
            if (knows_scalar_evaluator(name) || name == "(" || name == ")")
                throw ParseError(name_pos);
            predefined_heuristics[name] = heuristics[i];
            if (i == heuristics.size() - 1 && input[name_pos + 1] != "=") {
                throw ParseError(name_pos + 1);
            } else if (i < heuristics.size() - 1 && input[name_pos + 1] != ",") {
                throw ParseError(name_pos + 1);
            }
        }
    } else {
        throw ParseError(1);
    }
}

void OptionParser::predefine_lm_graph(const char *str) {
    vector<string> tokens;
    tokenize_options(str, tokens);
    try {
        predefine_lm_graph(tokens);
    } catch (ParseError &e) {
        print_parse_error(tokens, e);
        exit(2);
    }
}

void OptionParser::predefine_lm_graph(const vector<string> &input) {
    if (input[1].compare("=") == 0) {
        string name = input[0];
        if (knows_lm_graph(name) || name == "(" || name == ")")
            throw ParseError(0);
        int end = 2;
        predefined_lm_graphs[name] = parse_lm_graph(input, 2, end, false);
    } else {
        throw ParseError(1);
    }
}


// sets "end" on the last position of the last scalar evaluator
// in the list.
// if the list does not contain any scalar evaluator then end=start
void OptionParser::parse_scalar_evaluator_list(
    const vector<string> &input, int start, int &end,
    bool only_one_eval, vector<ScalarEvaluator *> &evals, bool dry_run) {
    end = start;
    bool break_loop = false;
    while (knows_scalar_evaluator(input[end])) {
        if (only_one_eval && evals.size() > 0)
            throw ParseError(end);
        ScalarEvaluator *eval =
            parse_scalar_evaluator(input, end, end, dry_run);
        evals.push_back(eval);
        end++;
        if (input[end] != ",") {
            break_loop = true;
            break;
        }
        end++;
    }
    if (!evals.empty()) {
        end--;
        if (!break_loop) {
            end--;
        }
    }
}

void OptionParser::parse_heuristic_list(
    const vector<string> &input, int start, int &end, bool only_one_eval,
    vector<Heuristic *> &evals, bool dry_run) {
    end = start;
    bool break_loop = false;
    while (knows_scalar_evaluator(input[end])) {
        if (only_one_eval && evals.size() > 0)
            throw ParseError(end);
        Heuristic *heur =
            parse_heuristic(input, end, end, dry_run);
        evals.push_back(heur);
        end++;
        if (input[end] != ",") {
            break_loop = true;
            break;
        }
        end++;
    }
    if (!evals.empty()) {
        end--;
        if (!break_loop) {
            end--;
        }
    }
}

void OptionParser::parse_landmark_graph_list(
    const vector<string> &input, int start, int &end, bool only_one,
    vector<LandmarksGraph *> &lm_graphs, bool dry_run) {
    end = start;
    bool break_loop = false;
    while (knows_lm_graph(input[end])) {
        if (only_one && lm_graphs.size() > 0)
            throw ParseError(end);
        LandmarksGraph *lmg =
            parse_lm_graph(input, end, end, dry_run);
        lm_graphs.push_back(lmg);
        end++;
        if (input[end] != ",") {
            break_loop = true;
            break;
        }
        end++;
    }
    if (!lm_graphs.empty()) {
        end--;
        if (!break_loop) {
            end--;
        }
    }
}

void OptionParser::parse_search_engine_list(
    const vector<string> &input, int start, int &end, bool only_one,
    vector<int> &engines, bool dry_run) {
    end = start;
    bool break_loop = false;
    while (knows_search_engine(input[end])) {
        if (only_one && engines.size() > 0)
            throw ParseError(end);
        engines.push_back(end);
        parse_search_engine(input, end, end, dry_run);
        end++;
        if (input[end] != ",") {
            break_loop = true;
            break;
        }
        end++;
    }
    if (!engines.empty()) {
        end--;
        if (!break_loop) {
            end--;
        }
    }
}

// helper method for simple configs that may have the
// form "<name>()" (e.g. "ff()" for the ff heuristic)
void OptionParser::set_end_for_simple_config(
    const vector<string> &config, int start, int &end) {
    if (config.size() <= start)
        throw ParseError(start);

    // "<name>()"
    if (config.size() > start + 2 && config[start + 1] == "(" &&
        config[start + 2] == ")") {
        end = start + 2;
    } else {
        throw ParseError(start + 2);
    }
    return;
}

double OptionParser::parse_double(
    const vector<string> &config, int start, int &end) {
    start = end;
    string str = config[end];
    if (str.find_first_not_of(".0123456789") != string::npos)
        throw ParseError(end);
    double val = atof(str.c_str());
    if (val == 0 && str.find_first_not_of(".0") != string::npos)
        throw ParseError(end);

    return val;
}

int OptionParser::parse_int(
    const vector<string> &config, int start, int &end) {
    start = end;
    int val = atoi(config[end].c_str());
    if (val == 0 && config[end] != "0")
        throw ParseError(end);

    // check that there are no additional characters
    ostringstream temp;
    temp << val;
    if (temp.str() != config[end])
        throw ParseError(end);
    return val;
}

void OptionParser::parse_evals_and_options(
    const vector<string> &config, int start, int &end,
    vector<ScalarEvaluator *> &evaluators,
    NamedOptionParser &option_parser, bool only_one_eval, bool dry_run) {
    if (config[start + 1] != "(")
        throw ParseError(start + 1);
    // create evaluators
    parse_scalar_evaluator_list(config, start + 2, end,
                                only_one_eval, evaluators, dry_run);

    if (evaluators.empty())
        throw ParseError(end);
    // need at least one scalar evaluator (e.g. a heuristic)

    // parse options
    end++;
    if (config[end] != ")") {
        if (config[end] != ",")
            throw ParseError(end);
        end++;
        option_parser.parse_options(config, end, end, dry_run);
        end++;
    }
    if (config[end] != ")")
        throw ParseError(end);
}

void NamedOptionParser::add_bool_option(const string &name,
                                        bool *var, const string &desc) {
    bool_options[name] = var;
    help[name] = "bool " + name + " " + desc;
    // TODO: add default value to help string
}

void NamedOptionParser::add_string_option(
    const string &name, string *var, const string &desc) {
    str_options[name] = var;
    help[name] = "string " + name + " " + desc;
    // TODO: add default value to help string
}

void NamedOptionParser::add_double_option(
    const string &name, double *var, const string &desc) {
    double_options[name] = var;
    help[name] = "double " + name + " " + desc;
    // TODO: add default value to help string
}

void NamedOptionParser::add_int_option(
    const string &name, int *var, const string &desc, bool allow_infinity) {
    if (allow_infinity) {
        can_be_infinity.insert(name);
        cout << "insert " << name << endl;
    }
    int_options[name] = var;
    help[name] = "int " + name + " " + desc;
    // TODO: add default value to help string
}

void NamedOptionParser::add_scalar_evaluator_option(
    const string &name, ScalarEvaluator **var,
    const string &desc, bool allow_none) {
    scalar_evaluator_options[name] = var;
    if (allow_none)
        can_be_none.insert(name);
    help[name] = "scalar evaluator " + name + " " + desc;
}

void NamedOptionParser::add_heuristic_list_option(
    const string &name, vector<Heuristic *> *var, const string &desc) {
    heuristic_list_options[name] = var;
    help[name] = "heuristic list " + name + " " + desc;
}

void NamedOptionParser::parse_options(
    const vector<string> &config, int start, int &end, bool dry_run) {
    bool first_option = true;
    end = start;
    while (first_option || config[end] == ",") {
        if (first_option) {
            first_option = false;
        } else {
            end++;
        }

        if (bool_options.count(config[end]) > 0) {
            parse_bool_option(config, end, end);
        } else if (str_options.count(config[end]) > 0) {
            parse_string_option(config, end, end);
        } else if (int_options.count(config[end]) > 0) {
            parse_int_option(config, end, end);
        } else if (double_options.count(config[end]) > 0) {
            parse_double_option(config, end, end);
        } else if (scalar_evaluator_options.count(config[end]) > 0) {
            parse_scalar_evaluator_option(config, end, end, dry_run);
        } else if (heuristic_list_options.count(config[end]) > 0) {
            parse_heuristic_list_option(config, end, end, dry_run);
        } else {
            throw ParseError(end);
        }
        end++;
    }
    end--;
}

void NamedOptionParser::parse_bool_option(const vector<string> &config,
                                          int start, int &end) {
    end = start;
    bool *val = bool_options[config[end]];
    end++;
    if (config[end] != "=")
        throw ParseError(end);
    end++;
    if (config[end] == "true") {
        *val = true;
    } else if (config[end] == "false") {
        *val = false;
    } else {
        throw ParseError(end);
    }
}

void NamedOptionParser::parse_string_option(const vector<string> &config,
                                            int start, int &end) {
    end = start;
    string *val = str_options[config[end]];
    end++;
    if (config[end] != "=")
        throw ParseError(end);
    end++;
    *val = config[end];
}

void NamedOptionParser::parse_double_option(const vector<string> &config,
                                            int start, int &end) {
    end = start;
    double *val = double_options[config[end]];
    end++;
    if (config[end] != "=")
        throw ParseError(end);
    end++;

    OptionParser *parser = OptionParser::instance();
    *val = parser->parse_double(config, end, end);
}

void NamedOptionParser::parse_int_option(const vector<string> &config,
                                         int start, int &end) {
    end = start;
    int *val = int_options[config[end]];
    string name = config[end];
    end++;
    if (config[end] != "=")
        throw ParseError(end);
    end++;

    string str_val = config[end];
    if (str_val == "Infinity" || str_val == "infinity") {
        if (can_be_infinity.find(name) != can_be_infinity.end()) {
            *val = numeric_limits<int>::max();
            return;
        } else {
            throw ParseError(end);
        }
    }

    OptionParser *parser = OptionParser::instance();
    *val = parser->parse_int(config, end, end);
}

void NamedOptionParser::parse_scalar_evaluator_option(
    const vector<string> &config, int start, int &end, bool dry_run) {
    end = start;
    ScalarEvaluator **pp_eval = scalar_evaluator_options[config[end]];
    string name = config[end];
    ScalarEvaluator *val;
    end++;
    if (config[end] != "=")
        throw ParseError(end);
    end++;

    string str_val = config[end];
    if (str_val == "None" || str_val == "none") {
        if (can_be_none.find(name) != can_be_none.end()) {
            val = NULL;
            return;
        } else {
            throw ParseError(end);
        }
    }

    OptionParser *parser = OptionParser::instance();
    val = parser->parse_scalar_evaluator(config, end, end, dry_run);
    *pp_eval = val;
}

void NamedOptionParser::parse_heuristic_list_option(
    const vector<string> &config, int start, int &end, bool dry_run) {
    end = start;
    vector<Heuristic *> *val =
        heuristic_list_options[config[end]];
    end++;
    if (config[end] != "=")
        throw ParseError(end);
    end++;
    OptionParser *parser = OptionParser::instance();

    if (config[end] == "(" && config[end + 1] == ")") {
        end++;
        return;
    } else if (config[end] == "(") {
        end++;
        parser->parse_heuristic_list(config, end, end, false, *val, dry_run);
        end++;
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        parser->parse_heuristic_list(config, end, end, true, *val, dry_run);
    }
}

void OptionParser::tokenize_options(const char *str, vector<string> &tokens) {
    while (true) {
        const char *begin = str;
        while (*str != '(' && *str != ',' && *str != ')' && *str != '=' && *str)
            str++;
        if (begin != str) {
            string substring = strip_and_to_lower(begin, str);
            if (substring != "") {
                tokens.push_back(substring);
            }
        }
        if (*str)
            tokens.push_back(string(str, str + 1));
        if (0 == *str++)
            break;
    }
}

string OptionParser::strip_and_to_lower(const char *begin, const char *end) {
    const char *b = begin;
    const char *e = end;
    // lstrip
    while ((*b == ' ' || *b == '\n' || *b == '\t') && b < end)
        b++;

    // return if empty string
    if (b == end)
        return "";

    // rstrip
    do {
        --e;
    } while (*e == ' ' && e > b);
    e++;

    string ret_string = string(b, e);

    // to lower
    for (unsigned int i = 0; i < ret_string.length(); i++)
        ret_string[i] = tolower(ret_string[i]);
    return ret_string;
}

void OptionParser::print_parse_error(const vector<string> &tokens,
                                     ParseError &err) {
    cerr << "ParseError: ";
    for (unsigned int i = 0; i < tokens.size(); i++)
        cerr << tokens[i];
    cerr << endl;
    cerr << "            ";  // same length as "ParseError: "
    for (unsigned int i = 0; i < err.pos; i++) {
        for (unsigned int j = 0; j < tokens[i].size(); j++) {
            cerr << " ";
        }
    }
    cerr << "^" << endl;
}
