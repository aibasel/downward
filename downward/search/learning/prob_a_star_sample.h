#ifndef PROB_A_STAR_SAMPLE_H
#define PROB_A_STAR_SAMPLE_H

#include <vector>

#include "../fh_open_list.h"
#include "../search_engine.h"
#include "../search_space.h"
#include "../state.h"
#include "../timer.h"
#include "../a_star_search.h"
#include "../heuristic.h"
#include "state_space_sample.h"

class Operator;

class ProbAStarSample : public StateSpaceSample {
private:
	Heuristic *heuristic;
	int min_training_set_size;

	double expand_all_prob;

	int exp;
	int gen;
public:
    ProbAStarSample(Heuristic *h,  int size);
    ~ProbAStarSample();

    virtual int step();
    virtual int collect();
};

#endif
