#include "best_first_search.h"
#include "cyclic_cg_heuristic.h"
#include "cg_heuristic.h"
#include "ff_heuristic.h"
#include "fd_heuristic.h"
#include "lm_cut_heuristic.h"
#include "max_heuristic.h"
#include "additive_heuristic.h"
#include "goal_count_heuristic.h"
#include "blind_search_heuristic.h"
#include "globals.h"
#include "operator.h"
#include "timer.h"
#include "general_eager_best_first_search.h"
#include "landmarks/lama_ff_synergy.h"
#include "landmarks/landmarks_graph.h"
#include "landmarks/landmarks_graph_rpg_sasp.h"
#include "landmarks/landmarks_count_heuristic.h"
#include "landmarks/exploration.h"
#include "hm_heuristic.h"
#include "general_lazy_best_first_search.h"
#include "learning/selective_max_heuristic.h"
#include "enforced_hill_climbing_search.h"
#include "option_parser.h"
#include "g_evaluator.h"
#include "pref_evaluator.h"
#include "sum_evaluator.h"
#include "weighted_evaluator.h"

#include <iostream>
#include <fstream>
#include <limits>
#include <vector>
#include <signal.h>
using namespace std;

int save_plan(const vector<const Operator *> &plan);

void tokenize_options(const char *str, vector<std::string> &tokens); 

void register_parsers();
void print_parse_error(const std::vector<std::string> tokens, ParseError &err);

void exit_handler(int exit_code, void*);
void signal_handler(int exit_code);

int main(int argc, const char **argv) {
    // Register event handlers
    on_exit(exit_handler, 0);
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGINT,  signal_handler);

    // read prepropressor output first because we need to know the initial
    // state when we create a general lazy search engine
    bool poly_time_method = false;

    istream &in = cin;

    in >> poly_time_method;
    if (poly_time_method) {
        cout << "Poly-time method not implemented in this branch." << endl;
        cout << "Starting normal solver." << endl;
    }

    read_everything(in);

    register_parsers();
    SearchEngine *engine = 0;
    for (unsigned int i = 1; i < argc; ++ i) {
        string arg = string(argv[i]);
        if (arg.compare("--heuristic") == 0 or arg.compare("-h") == 0) {
            ++ i;
            vector<string> tokens;
            tokenize_options(argv[i], tokens);
            try {
                OptionParser::instance()->predefine_heuristic(tokens);
            } catch (ParseError e) {
                print_parse_error(tokens, e);
                return 2;
            }
        } else if (arg.compare("--search") == 0 or arg.compare("-s") == 0) {
            ++ i;
            vector<string> tokens;
            tokenize_options(argv[i], tokens);
            try {
                int end = 0;
                engine = OptionParser::instance()->parse_search_engine(tokens, 0, end);
            } catch (ParseError e) {
                print_parse_error(tokens, e);
                return 2;
            }
        } else if (arg.compare("--random-seed") == 0 or arg.compare("-r") == 0) {
            ++ i;
            srand(atoi(argv[i]));
            cout << "random seed " << argv[i] << endl;
        } else {
            cerr << "unknown option " << arg << endl;
            // XXX TODO: add usage message
            exit(1);
        }
    }

    

    Timer search_timer;
    engine->search();
    search_timer.stop();
    g_timer.stop();

    save_plan(engine->get_plan());
    engine->statistics();
    engine->heuristic_statistics();
    cout << "Search time: " << search_timer << endl;
    cout << "Total time: " << g_timer << endl;
    

    return engine->found_solution() ? 0 : 1;
}

void print_peak_mem_usage() {
    char filename[64];
    sprintf(filename, "/proc/%d/status", (int)getpid());
    string word;
    ifstream proc_is(filename);
    bool done = false;
    while (!done) {
        proc_is >> word;
        if (word == "VmPeak:") {
            proc_is >> word;
            cout << "Peak Memory Usage: " << word << " kB" << endl;;
            done = true;
        }
        else {
            done = proc_is.eof();
        }
    }
}

void exit_handler(int, void*) {
	print_peak_mem_usage();
}

void signal_handler(int ) {
	print_peak_mem_usage();
}


int save_plan(const vector<const Operator *> &plan) {
    ofstream outfile;
    int plan_cost = 0;
    outfile.open("sas_plan", ios::out);
    for (int i = 0; i < plan.size(); i++) {
        cout << plan[i]->get_name() << " (" << plan[i]->get_cost() << ")" << endl;
        outfile << "(" << plan[i]->get_name() << ")" << endl;
        plan_cost += plan[i]->get_cost();
    }
    outfile.close();
    cout << "Plan length: " << plan.size() << " step(s)." << endl;
    cout << "Plan cost: " << plan_cost << endl;
    return plan_cost;
}

void register_parsers() {
    // Register search engines
    OptionParser::instance()->register_search_engine("eager", 
        GeneralEagerBestFirstSearch::create_engine);
    OptionParser::instance()->register_search_engine("eager_astar", 
        GeneralEagerBestFirstSearch::create_astar_engine);
    OptionParser::instance()->register_search_engine("eager_standard_greedy", 
        GeneralEagerBestFirstSearch::create_standard_greedy_engine);
    OptionParser::instance()->register_search_engine("lazy", 
        GeneralLazyBestFirstSearch::create_engine);
    OptionParser::instance()->register_search_engine("lazy_standard_greedy", 
        GeneralLazyBestFirstSearch::create_standard_greedy_engine);
    OptionParser::instance()->register_search_engine("lazy_wastar", 
        GeneralLazyBestFirstSearch::create_weighted_astar_engine);
    OptionParser::instance()->register_search_engine("ehc", 
        EnforcedHillClimbingSearch::create_engine);
    OptionParser::instance()->register_search_engine("old_greedy", 
        BestFirstSearchEngine::create_engine);

    // Register heuristics
    OptionParser::instance()->register_scalar_evaluator("ff", 
        FFHeuristic::create_heuristic);
    OptionParser::instance()->register_scalar_evaluator("cg", 
        CGHeuristic::create_heuristic);
    OptionParser::instance()->register_scalar_evaluator("cea", 
        CyclicCGHeuristic::create_heuristic);
    OptionParser::instance()->register_scalar_evaluator("add", 
        AdditiveHeuristic::create_heuristic);
    OptionParser::instance()->register_scalar_evaluator("blind", 
        BlindSearchHeuristic::create_heuristic);
    OptionParser::instance()->register_scalar_evaluator("goalcount", 
        GoalCountHeuristic::create_heuristic);
    OptionParser::instance()->register_scalar_evaluator("mas", 
        FinkbeinerDraegerHeuristic::create_heuristic);
    OptionParser::instance()->register_scalar_evaluator("lmcut", 
        LandmarkCutHeuristic::create_heuristic);
    OptionParser::instance()->register_scalar_evaluator("hmax", 
        HSPMaxHeuristic::create_heuristic);
    OptionParser::instance()->register_scalar_evaluator("hm", 
        HMHeuristic::create_heuristic);
   
    // register combinations and g evaluator
    OptionParser::instance()->register_scalar_evaluator("sum", 
        SumEvaluator::create_sum_evaluator);
    OptionParser::instance()->register_scalar_evaluator("weight", 
        WeightedEvaluator::create_weighted_evaluator);
    OptionParser::instance()->register_scalar_evaluator("g", 
        GEvaluator::create_g_evaluator);
    OptionParser::instance()->register_scalar_evaluator("pref", 
        PrefEvaluator::create_pref_evaluator);

    // Note:
    // open lists are registered in the constructor of OpenListParser.
    // This is necessary because the open list entries are specified
    // as template parameter and we otherwise would have template
    // parameters everywhere
}

std::string strip_and_to_lower(const char* begin, const char* end) {
    const char *b = begin;
    const char *e = end;
    // lstrip
    while (*b == ' ' && b < end) 
        b++;
    
    // return if empty string
    if (b == end) return "";

    // rstrip
    do {
        -- e;
    } while (*e == ' ' && e > b);
    e ++;

    string ret_string = string(b, e);

    // to lower
    for (unsigned int i = 0; i < ret_string.length(); i++) 
        ret_string[i] = tolower(ret_string[i]);
    return ret_string;
}

void tokenize_options(const char *str, vector<std::string> &tokens) 
{
    while (true) {
        const char *begin = str;
        while (*str != '(' && *str != ',' && *str != ')' && *str != '=' && *str)
            str ++;
        if (begin != str) {
            string substring = strip_and_to_lower(begin, str);
            if (substring != "") {
                tokens.push_back(substring);
            }
        }
        if (*str)
            tokens.push_back(string(str, str+1));
        if (0 == *str++)
            break;
    }
}

void print_parse_error(const std::vector<std::string> tokens, ParseError &err) {
    cerr << "ParseError: ";
    for (unsigned int i = 0; i < tokens.size(); i ++)
        cerr << tokens[i];
    cerr << endl;
    cerr << "            ";  // same lenght as "ParseError: "
    for (unsigned int i = 0; i < err.pos; i ++) {
        for (unsigned int j = 0; j < tokens[i].size(); j ++) {
            cerr << " ";            
        }
    }
    cerr << "^" << endl;
}
