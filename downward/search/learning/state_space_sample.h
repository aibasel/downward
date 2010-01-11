#ifndef STATESPACESAMPLE_H_
#define STATESPACESAMPLE_H_

#include "../search_space.h"
#include "../heuristic.h"
#include <map>
#include <sys/times.h>
#include <vector>

using namespace std;

enum state_space_sample_t {Probe = 0, ProbAStar = 1, PDB = 2};
typedef map<State, vector<int> > sample_t;

class StateSpaceSample {
protected:
	// parameters
	bool uniform_sampling;
	vector<Heuristic *> heuristics;

	// gathered data
	sample_t samp;
	double branching_factor;
	vector<clock_t> computation_time;

	int choose_operator(vector<int> &h_s);
public:
	StateSpaceSample();
	virtual ~StateSpaceSample();

    bool get_uniform_sampling() const {return uniform_sampling;}
    void set_uniform_sampling(bool us) {uniform_sampling = us;}

    double get_branching_factor() const {return branching_factor;}

    void add_heuristic(Heuristic *h) {heuristics.push_back(h); computation_time.push_back(0);}
    clock_t get_computation_time(int i) {return computation_time[i];}

    virtual sample_t &get_samp() {return samp;}

    virtual int collect() = 0;

};

#endif /* STATESPACESAMPLE_H_ */
