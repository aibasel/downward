#ifndef PROBESTATESPACESAMPLE_H_
#define PROBESTATESPACESAMPLE_H_

#include "state_space_sample.h"
#include "../globals.h"
#include "../heuristic.h"

class ProbeStateSpaceSample: public StateSpaceSample {
protected:
	// parameters
	Heuristic *heuristic;
    int goal_depth_estimate;
    int max_num_probes;
    int min_training_set_size;

    int expanded;
    int generated;

    void send_probe(int depth_limit);
public:
    ProbeStateSpaceSample(Heuristic *h, int goal_depth, int probes, int size);
    virtual ~ProbeStateSpaceSample();
    virtual int collect();

    int get_goal_depth_estimate() const {return goal_depth_estimate;}
    void set_goal_depth_estimate(int depth) {goal_depth_estimate = depth;}

    int get_max_num_probes() const {return max_num_probes;}
    void set_max_num_probes(int num_probes) {max_num_probes = num_probes;}

    int get_min_training_set_size() const {return min_training_set_size;}
    void set_min_training_set_size(int training_set_size) {min_training_set_size = training_set_size;}

    Heuristic* get_heuristic() const {return heuristic;}
    void set_heuristic(Heuristic *h) {heuristic = h;}
};

#endif /* PROBESTATESPACESAMPLE_H_ */
