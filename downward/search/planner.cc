#include "best_first_search.h"
#include "cg_heuristic.h"
#include "ff_heuristic.h"
#include "goal_count_heuristic.h"
#include "blind_search_heuristic.h"
#include "globals.h"
#include "operator.h"

#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#include <sys/times.h>

void save_plan(const vector<const Operator *> &plan);

int main(int argc, const char **argv) {
    struct tms start, search_start, search_end;
    times(&start);
    bool poly_time_method = false;
    
    bool cg_heuristic = false, cg_preferred_operators = false;
    bool ff_heuristic = false, ff_preferred_operators = false;
    bool goal_count_heuristic = false;
    bool blind_search_heuristic = false;
    for(int i = 1; i < argc; i++) {
	for(const char *c = argv[i]; *c != 0; c++) {
	    if(*c == 'c') {
		cg_heuristic = true;
	    } else if(*c == 'C') {
		cg_preferred_operators = true;
	    } else if(*c == 'f') {
		ff_heuristic = true;
	    } else if(*c == 'F') {
		ff_preferred_operators = true;
	    } else if(*c == 'g') {
		goal_count_heuristic = true;
	    } else if(*c == 'b') {
		blind_search_heuristic = true;
	    } else {
		cerr << "Unknown option: " << *c << endl;
		return 1;
	    }
	}
    }

    if(!cg_heuristic && !ff_heuristic && !goal_count_heuristic
        && !blind_search_heuristic) {
	cerr << "Error: you must select at least one heuristic!" << endl
	     << "If you are unsure, choose options \"cCfF\"." << endl;
	return 2;
    }

    cin >> poly_time_method;
    if(poly_time_method) {
	cout << "Poly-time method not implemented in this branch." << endl;
	cout << "Starting normal solver." << endl;
	// cout << "Aborting." << endl;
	// return 1;
    }

    read_everything(cin);
    // dump_everything();

    BestFirstSearchEngine *engine = new BestFirstSearchEngine;
    if(cg_heuristic || cg_preferred_operators)
	engine->add_heuristic(new CGHeuristic, cg_heuristic,
			      cg_preferred_operators);
    if(ff_heuristic || ff_preferred_operators)
	engine->add_heuristic(new FFHeuristic, ff_heuristic,
			      ff_preferred_operators);
    if(goal_count_heuristic)
	engine->add_heuristic(new GoalCountHeuristic, true, false);
    if(blind_search_heuristic)
	engine->add_heuristic(new BlindSearchHeuristic, true, false);

    times(&search_start);
    engine->search();
    times(&search_end);
    if(engine->found_solution())
	save_plan(engine->get_plan());

    engine->statistics();
    if(cg_heuristic || cg_preferred_operators) {
	cout << "Cache hits: " << g_cache_hits << endl;
	cout << "Cache misses: " << g_cache_misses << endl;
    }
    int search_ms = (search_end.tms_utime - search_start.tms_utime) * 10;
    cout << "Search time: " << search_ms / 1000.0 << " seconds" << endl;
    int total_ms = (search_end.tms_utime - start.tms_utime) * 10;
    cout << "Total time: " << total_ms / 1000.0 << " seconds" << endl;

    return engine->found_solution() ? 0 : 1;
}

void save_plan(const vector<const Operator *> &plan) {
    ofstream outfile;
    outfile.open("sas_plan", ios::out);
    for(int i = 0; i < plan.size(); i++) {
	cout << plan[i]->get_name() << endl;
	outfile << "(" << plan[i]->get_name() << ")" << endl;
    }
    outfile.close();
    cout << "Plan length: " << plan.size() << " step(s)." << endl;
}
