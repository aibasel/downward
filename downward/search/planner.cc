#include "best_first_search.h"
#include "cyclic_cg_heuristic.h"
#include "cg_heuristic.h"
#include "ff_heuristic.h"
#include "lm_cut_heuristic.h"
#include "mas_heuristic.h"
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
#include "utilities.h"
#include "iterated_search.h"

#include <iostream>
#include <fstream>
#include <vector>
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
        "See http://alfons.informatik.uni-freiburg.de/downward/HomePage for " 
        "details.";

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

    for (unsigned int i = 1; i < argc; ++ i) {
        string arg = string(argv[i]);
        if (arg.compare("--heuristic") == 0) {
            ++ i;
            OptionParser::instance()->predefine_heuristic(argv[i]);
        } else if (arg.compare("--search") == 0) {
            ++ i;
            engine = OptionParser::instance()->parse_search_engine(argv[i]); 
        } else if (arg.compare("--random-seed") == 0) {
            ++ i;
            srand(atoi(argv[i]));
            cout << "random seed " << argv[i] << endl;
        } else {
            cerr << "unknown option " << arg << endl << endl;
            cout << usage << endl;
            exit(1);
        }
    }

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
    OptionParser::instance()->register_search_engine("eager", 
        GeneralEagerBestFirstSearch::create);
    OptionParser::instance()->register_search_engine("astar", 
        GeneralEagerBestFirstSearch::create_astar);
    OptionParser::instance()->register_search_engine("eager_greedy", 
        GeneralEagerBestFirstSearch::create_greedy);
    OptionParser::instance()->register_search_engine("lazy", 
        GeneralLazyBestFirstSearch::create);
    OptionParser::instance()->register_search_engine("lazy_greedy", 
        GeneralLazyBestFirstSearch::create_greedy);
    OptionParser::instance()->register_search_engine("lazy_wastar", 
        GeneralLazyBestFirstSearch::create_weighted_astar);
    OptionParser::instance()->register_search_engine("ehc", 
        EnforcedHillClimbingSearch::create);
    OptionParser::instance()->register_search_engine("old_greedy", 
        BestFirstSearchEngine::create);
    OptionParser::instance()->register_search_engine("iterated",
            IteratedSearch::create);

    // Register heuristics
    OptionParser::instance()->register_scalar_evaluator("add", 
        AdditiveHeuristic::create);
    OptionParser::instance()->register_scalar_evaluator("blind", 
        BlindSearchHeuristic::create);
    OptionParser::instance()->register_scalar_evaluator("cg", 
        CGHeuristic::create);
    OptionParser::instance()->register_scalar_evaluator("cea", 
        CyclicCGHeuristic::create);
    OptionParser::instance()->register_scalar_evaluator("ff", 
        FFHeuristic::create);
    OptionParser::instance()->register_scalar_evaluator("goalcount", 
        GoalCountHeuristic::create);
    OptionParser::instance()->register_scalar_evaluator("hm", 
        HMHeuristic::create);
    OptionParser::instance()->register_scalar_evaluator("hmax", 
        HSPMaxHeuristic::create);
    OptionParser::instance()->register_scalar_evaluator("lmcount", 
        LandmarksCountHeuristic::create);
    OptionParser::instance()->register_scalar_evaluator("lmcut", 
        LandmarkCutHeuristic::create);
    OptionParser::instance()->register_scalar_evaluator("mas", 
        MergeAndShrinkHeuristic::create);
    OptionParser::instance()->register_scalar_evaluator("selmax", 
        SelectiveMaxHeuristic::create);
    
    // Register synergy heuristics
    OptionParser::instance()->register_synergy("lm_ff_syn", 
        LamaFFSynergy::create_heuristics);
   
    // register combinations and g evaluator
    OptionParser::instance()->register_scalar_evaluator("sum", 
        SumEvaluator::create);
    OptionParser::instance()->register_scalar_evaluator("weight", 
        WeightedEvaluator::create);
    OptionParser::instance()->register_scalar_evaluator("g", 
        GEvaluator::create);
    OptionParser::instance()->register_scalar_evaluator("pref", 
        PrefEvaluator::create);

    // Note:
    // open lists are registered in the constructor of OpenListParser.
    // This is necessary because the open list entries are specified
    // as template parameter and we would have template parameters everywhere
    // otherwise 
}


