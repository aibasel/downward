#include "best_first_search.h"
#include "enforced_hill_climbing_search.h"
#include "g_evaluator.h"
#include "general_eager_best_first_search.h"
#include "general_lazy_best_first_search.h"
#include "globals.h"
#include "iterated_search.h"
#include "operator.h"
#include "option_parser.h"
#include "pref_evaluator.h"
#include "sum_evaluator.h"
#include "timer.h"
#include "utilities.h"
#include "weighted_evaluator.h"

#include <iostream>
using namespace std;


void register_parsers();

int main(int argc, const char **argv) {
    register_event_handlers();

    string usage =
        "usage: \n" +
        string(argv[0]) + " [OPTIONS] --search SEARCH < OUTPUT\n\n"
        "* SEARCH (SearchEngine): configuration of the search algorithm\n"
        "* OUTPUT (filename): preprocessor output\n\n"
        "Options:\n"
        "--heuristic HEURISTIC_PREDEFINITION\n"
        "    Predefines a heuristic that can afterwards be referenced\n"
        "    by the name that is specified in the definition.\n"
        "--random-seed SEED\n"
        "    Use random seed SEED\n\n"
        "See http://www.fast-downward.org/ for details.";

    if (argc < 2) {
        cout << usage << endl;
        exit(1);
    }

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

    //the input will be parsed twice: 
    //once in dry-run mode, to check for simple input errors, 
    //then in normal mode
    OptionParser::parse_cmd_line(char **argv, true);
    OptionParser::parse_cmd_line(char **argv, false);
    

    Timer search_timer;
    engine->search();
    search_timer.stop();
    g_timer.stop();

    if (engine->found_solution())
        save_plan(engine->get_plan());
    engine->statistics();
    engine->heuristic_statistics();
    cout << "Search time: " << search_timer << endl;
    cout << "Total time: " << g_timer << endl;

    return engine->found_solution() ? 0 : 1;
}

void register_parsers() {
    // Register search engines
    Registry<SearchEngine>* search_reg = Registry<SearchEngine>::instance();
    search_reg->register("eager", GeneralEagerBestFirstSearch::create);

    search_reg->register("eager", GeneralEagerBestFirstSearch::create);
    search_reg->register("astar", GeneralEagerBestFirstSearch::create_astar);
    search_reg->register("eager_greedy", GeneralEagerBestFirstSearch::create_greedy);
    search_reg->register("lazy", GeneralLazyBestFirstSearch::create);
    search_reg->register("lazy_greedy", GeneralLazyBestFirstSearch::create_greedy);
    search_reg->register("lazy_wastar", GeneralLazyBestFirstSearch::create_weighted_astar);
    search_reg->register("ehc", EnforcedHillClimbingSearch::create);
    search_reg->register("old_greedy", BestFirstSearchEngine::create);
    search_reg->register("iterated", IteratedSearch::create);

    // register combinations and g evaluator
    Registry<ScalarEvaluator>* scalar_eval_reg = Registry<ScalarEvaluator>::instance();
    scalar_eval_reg->register("sum", SumEvaluator::create);
    scalar_eval_reg->register("weight", WeightedEvaluator::create);
    scalar_eval_reg->register("g", GEvaluator::create);
    scalar_eval_reg->register("pref", PrefEvaluator::create);

    // Note:
    // open lists are registered just before creation in the OptionParser,
    // since they are templated.
}
