#ifndef STATESPACESAMPLE_H
#define STATESPACESAMPLE_H

#include <vector>
#include "../search_space.h"

using namespace std;

enum state_space_sample_t {Probe = 0, ProbAStar = 1, PDB = 2};

class StateSpaceSample {
protected:
	// parameters
	bool uniform_sampling;

	// gathered data
	SearchSpace sample;
	double branching_factor;


	int choose_operator(int num_ops, vector<int> &h_s);
public:
	StateSpaceSample();
	virtual ~StateSpaceSample();

    bool get_uniform_sampling() const {return uniform_sampling;}
    void set_uniform_sampling(bool us) {uniform_sampling = us;}

    double get_branching_factor() const {return branching_factor;}

    virtual SearchSpace &get_sample() {return sample;}

    virtual int collect() = 0;
};

#endif
