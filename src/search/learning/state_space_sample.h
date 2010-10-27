#ifndef LEARNING_STATE_SPACE_SAMPLE_H
#define LEARNING_STATE_SPACE_SAMPLE_H

#include "../search_space.h"
#include "../heuristic.h"
#include <map>
#include <vector>
#include "../exact_timer.h"

using namespace std;

enum state_space_sample_t {Probe = 0, ProbAStar = 1, PDB = 2};
typedef map<State, vector<int> > sample_t;

class StateSpaceSample {
protected:
    // parameters
    bool uniform_sampling;
    vector<Heuristic *> heuristics;

    // gathered data
    ExactTimer computation_timer;
    sample_t samp;
    double branching_factor;
    vector<double> computation_time;

    int choose_operator(vector<int> &h_s);
public:
    StateSpaceSample();
    virtual ~StateSpaceSample();

    bool get_uniform_sampling() const {return uniform_sampling; }
    void set_uniform_sampling(bool us) {uniform_sampling = us; }

    double get_branching_factor() const {return branching_factor; }

    void add_heuristic(Heuristic *h) {
        heuristics.push_back(h);
        computation_time.push_back(0);
    }
    double get_computation_time(int i) {return computation_time[i]; }

    virtual sample_t &get_samp() {return samp; }

    virtual int collect() = 0;
};

#endif
