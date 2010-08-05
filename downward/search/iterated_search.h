#ifndef ITERATED_SEARCH_H_
#define ITERATED_SEARCH_H_

#include "search_engine.h"
#include "globals.h"
#include "search_progress.h"
#include "option_parser.h"

class IteratedSearch: public SearchEngine {
private:
    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool found_solution;

    SearchEngine *current_search;

    vector<SearchEngine *> engines;
    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    vector<SearchProgress> phase_statistics;
    vector<bool> phase_found_solution;
    vector<int> phase_solution_cost;


    SearchEngine *create_phase(int p);
    int step_return_value();

    virtual void initialize();
    virtual int step();
public:
    IteratedSearch(vector<SearchEngine *> engine_list,
                   bool pass_bound,
                   bool repeat_last_phase,
                   bool continue_on_fail,
                   bool continue_on_solve);
    virtual ~IteratedSearch();
    void statistics() const;
    static SearchEngine* create(
        const std::vector<std::string> &config, int start, int &end, 
        bool dry_run);
};

#endif /* ITERATED_SEARCH_H_ */
