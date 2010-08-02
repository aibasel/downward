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
    if( instance_ == 0 ) 
        instance_ = new OptionParser(); 
    return instance_;
}

void OptionParser::register_search_engine(string key, 
        SearchEngine* func(const vector<string> &, int, int&)) {
        engine_map[key] = func;
}

void OptionParser::register_scalar_evaluator(string key, 
    ScalarEvaluator* func(const vector<string> &, int, int&)) {
    scalar_evaluator_map[key] = func;
}

void OptionParser::register_synergy(string key, 
    void func(const vector<string> &, int, int&, vector<Heuristic *> &)) {
    synergy_map[key] = func;
}


ScalarEvaluator *OptionParser::parse_scalar_evaluator(
    const vector<string> &input, int start, int & end) {
    // predefined evaluator
    map<string, Heuristic*>::iterator iter;
    iter = predefined_heuristics.find(input[start]);
    if (iter != predefined_heuristics.end()) {
        end = start;
        return (ScalarEvaluator*) iter->second;
    }
   
    // scalar evaluater definition
    map<string, ScalarEvalCreatorFunc>::iterator it;
    it = scalar_evaluator_map.find(input[start]);
    if (it == scalar_evaluator_map.end()) throw ParseError(start);
    return it->second(input, start, end);
}

Heuristic *OptionParser::parse_heuristic(const vector<string> &input, 
                                         int start, int & end) {
    ScalarEvaluator* eval = parse_scalar_evaluator(input, start, end);
    Heuristic *h = dynamic_cast<Heuristic *>(eval); // HACK?
    if (h == 0) throw ParseError(start);
    return h;
}

void OptionParser::parse_synergy_heuristics(const vector<string> &input, 
                                            int start, int &end,
                                            vector<Heuristic *> &heuristics) {
    map<string, SynergyCreatorFunc>::iterator it;
    it = synergy_map.find(input[start]);
    if (it == synergy_map.end()) throw ParseError(start);
    it->second(input, start, end, heuristics);
}

bool OptionParser::knows_scalar_evaluator(string name) {
    return (scalar_evaluator_map.find(name) != scalar_evaluator_map.end() ||
            predefined_heuristics.find(name) != predefined_heuristics.end());
}

SearchEngine *OptionParser::parse_search_engine(const vector<string> &input, 
                                                int start, int & end) {

    map<string, EngineCreatorFunc>::iterator it;
    it = engine_map.find(input[start]);
    if (it == engine_map.end()) throw ParseError(start);
    return it->second(input, start, end);
}
    
void OptionParser::predefine_heuristic(const vector<string> &input) {
    if (input[1].compare("=") == 0) {
        // define one heuristic (standard case) 
        string name = input[0];
        if (knows_scalar_evaluator(name) || name == "(" || name == ")")
            throw ParseError(0);
        int end = 2;
        predefined_heuristics[name] = parse_heuristic(input, 2, end);
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

// sets "end" on the last position of the last scalar evaluator
// in the list.
// if the list does not contain any scalar evaluator then end=start
void OptionParser::parse_scalar_evaluator_list(const vector<string> &input, 
                                           int start, int & end, 
                                           bool only_one_eval,
                                           vector<ScalarEvaluator *> &evals) {
    end = start;
    bool break_loop = false;
    while (knows_scalar_evaluator(input[end])) {
        if (only_one_eval && evals.size() > 0)
            throw ParseError(end);
        ScalarEvaluator* eval = 
            parse_scalar_evaluator(input, end, end);
        evals.push_back(eval);
        end ++;
        if (input[end] != ",") {
            break_loop = true;
            break;
        }
        end ++;
    }
    if (!evals.empty()) {
        end --;
        if (!break_loop) {
            end --;
        }
    }
}

void OptionParser::parse_heuristic_list(const vector<string> &input, int start,
                                        int & end, bool only_one_eval,
                                        vector<Heuristic *> &evals) {
    end = start;
    bool break_loop = false;
    while (knows_scalar_evaluator(input[end])) {
        if (only_one_eval && evals.size() > 0)
            throw ParseError(end);
        Heuristic* heur = 
            parse_heuristic(input, end, end);
        evals.push_back(heur);
        end ++;
        if (input[end] != ",") {
            break_loop = true;
            break;
        }
        end ++;
    }
    if (!evals.empty()) {
        end --;
        if (!break_loop) {
            end --;
        }
    }
}

// helper method for simple configs that may have the
// form "<name>()" (e.g. "ff()" for the ff heuristic) 
void OptionParser::set_end_for_simple_config(const vector<string> &config, 
                                             int start, int &end) {
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

double OptionParser::parse_double(const vector<string> &config, 
                                  int start, int &end) {
    start = end;
    string str = config[end];
    if (str.find_first_not_of(".0123456789") != string::npos)
        throw ParseError(end);
    double val = atof(str.c_str());
    if (val == 0 && str.find_first_not_of(".0") != string::npos)
            throw ParseError(end);
   
    return val;
}

int OptionParser::parse_int(const vector<string> &config, 
                            int start, int &end) {
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

void OptionParser::parse_evals_and_options(const vector<string> &config, 
                                 int start, int &end,
                                 vector<ScalarEvaluator *> &evaluators, 
                                 NamedOptionParser &option_parser,
                                 bool only_one_eval
                               ) {
    if (config[start+1] != "(") throw ParseError(start+1);
    // create evaluators
    parse_scalar_evaluator_list(config, start + 2, end,
                                only_one_eval, evaluators);

    if (evaluators.empty()) throw ParseError(end);
    // need at least one scalar evaluator (e.g. a heuristic)

    // parse options
    end ++;
    if (config[end] != ")") {
        if (config[end] != ",") throw ParseError(end);
        end ++;
        option_parser.parse_options(config, end, end);
        end ++;
    }
    if (config[end] != ")") throw ParseError(end);
}

void NamedOptionParser::add_bool_option(string name, bool *var, string desc) {
    bool_options[name] = var;
    help[name] = "bool " + name + " " + desc;
    // TODO: add default value to help string
}

void NamedOptionParser::add_string_option(string name, string *var, 
                                          string desc) {
    str_options[name] = var;
    help[name] = "string " + name + " " + desc;
    // TODO: add default value to help string
}

void NamedOptionParser::add_double_option(string name, double *var, string desc) {
    double_options[name] = var;
    help[name] = "double " + name + " " + desc;
    // TODO: add default value to help string
}

void NamedOptionParser::add_int_option(string name, int *var, string desc, 
                                       bool allow_infinity) {
    if (allow_infinity) 
        can_be_infinity.insert(name);
    int_options[name] = var;
    help[name] = "int " + name + " " + desc;
    // TODO: add default value to help string
}

void NamedOptionParser::add_scalar_evaluator_option(string name, 
                                                    ScalarEvaluator *var, 
                                                    string desc, 
                                                    bool allow_none) {
    scalar_evaluator_options[name] = var;
    if (allow_none) 
        can_be_none.insert(name);
    help[name] = "scalar evaluator " + name + " " + desc;
}

void NamedOptionParser::add_heuristic_list_option(string name, 
                                                  vector<Heuristic *> *var, 
                                                  string desc) {
    heuristic_list_options[name] = var;
    help[name] = "heuristic list " + name + " " + desc;
}

void NamedOptionParser::parse_options(const vector<string> &config, 
                                      int start, int & end) {
    bool first_option = true;
    end = start;
    while (first_option || config[end] == ",") {
        if (first_option) {
            first_option = false;
        } else {
            end ++;
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
            parse_scalar_evaluator_option(config, end, end);
        } else if (heuristic_list_options.count(config[end]) > 0) {
            parse_heuristic_list_option(config, end, end);
        } else {
            throw ParseError(end);
        }
        end ++;
    }
    end --;
}

void NamedOptionParser::parse_bool_option(const vector<string> &config, 
                                          int start, int &end) {
    end = start;
    bool *val = bool_options[config[end]];
    end ++;
    if (config[end] != "=") throw ParseError(end);
    end ++;
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
    end ++;
    if (config[end] != "=")
        throw ParseError(end);
    end ++;
    *val = config[end];
}

void NamedOptionParser::parse_double_option(const vector<string> &config, 
                                            int start, int &end) {
    end = start;
    double *val = double_options[config[end]];
    end ++;
    if (config[end] != "=")
        throw ParseError(end);
    end ++;

    OptionParser *parser = OptionParser::instance();
    *val = parser->parse_double(config, end, end);
}

void NamedOptionParser::parse_int_option(const vector<string> &config, 
                                         int start, int &end) {
    end = start;
    int *val = int_options[config[end]];
    end ++;
    if (config[end] != "=") throw ParseError(end);
    end ++;
  
    string name = config[end];
    if (name == "Infinity" || name == "infinity") {
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
    const vector<string> &config, int start, int &end) {
    end = start;
    ScalarEvaluator *val = scalar_evaluator_options[config[end]];
    end ++;
    if (config[end] != "=") throw ParseError(end);
    end ++;

    string name = config[end];
    if (name == "None" || name == "none") {
        if (can_be_none.find(name) != can_be_none.end()) {
            val = NULL;
            return;
        } else {
            throw ParseError(end);
        }
    }

    OptionParser *parser = OptionParser::instance();
    val = parser->parse_scalar_evaluator(config, end, end);
}

void NamedOptionParser::parse_heuristic_list_option(
    const vector<string> &config, int start, int &end) {
    end = start;
    vector<Heuristic *> *val = 
        heuristic_list_options[config[end]];
    end ++;
    if (config[end] != "=")
        throw ParseError(end);
    end ++;
    OptionParser *parser = OptionParser::instance();

    if (config[end] == "(" && config[end + 1] == ")") {
        return;
    } else if (config[end] == "(") {
        end ++;
        parser->parse_heuristic_list(config, end, end, false, *val);
        end ++;
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        parser->parse_heuristic_list(config, end, end, true, *val);
    }
}
