#ifndef LEARNING_PROBE_STATE_SPACE_SAMPLE_H
#define LEARNING_PROBE_STATE_SPACE_SAMPLE_H

#include "state_space_sample.h"
#include "../globals.h"
#include "../heuristic.h"
#include <sys/times.h>


class ProbeStateSpaceSample : public StateSpaceSample {
protected:
    // parameters
    int goal_depth_estimate;
    int max_num_probes;
    int min_training_set_size;
    bool add_every_state;

    int expanded;
    int generated;

    sample_t *temporary_samp;
    void send_probe(int depth_limit);
    int get_aggregate_value(vector<int> &values);
public:
    ProbeStateSpaceSample(int goal_depth, int probes, int size);
    virtual ~ProbeStateSpaceSample();
    virtual int collect();

    int get_goal_depth_estimate() const {return goal_depth_estimate; }
    void set_goal_depth_estimate(int depth) {goal_depth_estimate = depth; }

    int get_max_num_probes() const {return max_num_probes; }
    void set_max_num_probes(int num_probes) {max_num_probes = num_probes; }

    int get_min_training_set_size() const {return min_training_set_size; }
    void set_min_training_set_size(int training_set_size) {min_training_set_size = training_set_size; }

    //void add_heuristic(Heuristic *h) {heuristics.push_back(h); computation_time.push_back(0);}
    //double get_computation_time(int i) {return computation_time[i];}
};

#endif
