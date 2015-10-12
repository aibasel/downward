#include "probe_state_space_sample.h"

#include "../heuristic.h"
#include "../successor_generator.h"

#include <limits>
#include <cassert>

ProbeStateSpaceSample::ProbeStateSpaceSample(int goal_depth, int probes = 10, int size = 100)
    : goal_depth_estimate(goal_depth), max_num_probes(probes), min_training_set_size(size),
      temporary_samp(&samp) {
    expanded = 0;
    generated = 0;
    add_every_state = true;
}

ProbeStateSpaceSample::~ProbeStateSpaceSample() {
}

int ProbeStateSpaceSample::collect() {
    cout << "Probe state space sample" << endl;
    int num_probes = 0;
    while ((samp.size() < min_training_set_size) && (num_probes < max_num_probes)) {
        ++num_probes;
        send_probe(goal_depth_estimate);
        //cout << "Probe: " << num_probes << " - " << samp.size() << endl;
    }

    branching_factor = (double)generated / (double)expanded;

    return samp.size();
}

int ProbeStateSpaceSample::get_aggregate_value(vector<int> &values) {
    int max = numeric_limits<int>::min();
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] > max)
            max = values[i];
    }
    return max;
}

void ProbeStateSpaceSample::send_probe(int depth_limit) {
    vector<const GlobalOperator *> applicable_ops;
    vector<int> h_s;

    // TODO for now we use the global state registry for all states (even temporary states like this): see issue386
    GlobalState s = g_initial_state();
    sample_t::const_iterator succ_it = temporary_samp->find(s);
    if (succ_it == temporary_samp->end()) {
        (*temporary_samp)[s].reserve(0);
        succ_it = temporary_samp->find(s);
        if (add_every_state) {
            samp[s].reserve(heuristics.size());
            for (size_t i = 0; i < heuristics.size(); ++i) {
                heuristics[i]->evaluate(succ_it->first);
                int val = numeric_limits<int>::max();
                if (!heuristics[i]->is_dead_end()) {
                    val = heuristics[i]->get_heuristic();
                }
                samp[s].push_back(val);
            }
        }
    }


    for (int i = 0; (i < depth_limit) && (samp.size() < min_training_set_size); ++i) {
        ++expanded;
        applicable_ops.clear();
        g_successor_generator->generate_applicable_ops(s, applicable_ops);

        if (applicable_ops.size() == 0) {
            break;
        }
        generated = generated + applicable_ops.size();
        h_s.resize(applicable_ops.size());

        for (size_t op_num = 0; op_num < applicable_ops.size(); ++op_num) {
            // generate and add to training set all successors
            const GlobalOperator *op = applicable_ops[op_num];
            // TODO for now, we only generate registered successors. This is a temporary state that
            // should should not necessarily be registered in the global registry: see issue386.
            GlobalState succ = GlobalState::construct_registered_successor(s, *op);

            succ_it = temporary_samp->find(succ);
            if (succ_it == temporary_samp->end()) {
                // new state
                (*temporary_samp)[succ].resize(0);
            } else {
                // loop - what to do?
            }
            succ_it = temporary_samp->find(succ);

            if (add_every_state) {
                samp[s].resize(heuristics.size());
            }

            for (size_t j = 0; j < heuristics.size(); ++j) {
                double before = computation_timer();
                heuristics[j]->reach_state(s, *op, succ_it->first);
                if (add_every_state) {
                    heuristics[j]->evaluate(succ_it->first);
                    double after = computation_timer();
                    computation_time[j] += after - before;
                    int val = numeric_limits<int>::max();
                    if (!heuristics[j]->is_dead_end()) {
                        val = heuristics[j]->get_heuristic();
                    }
                    samp[succ].push_back(val);
                }
            }

            h_s[op_num] = get_aggregate_value((*temporary_samp)[succ]);
        }

        // choose operator at random
        int op_num = choose_operator(h_s);

        const GlobalOperator *op = applicable_ops[op_num];

        //cout << op->get_name() << endl;

        // TODO for now, we only generate registered successors. This is a temporary state that
        // should should not necessarily be registered in the global registry: see issue386.
        GlobalState succ = GlobalState::construct_registered_successor(s, *op);

        if (test_goal(succ)) {
            //if (succ_node.is_goal()) {
            break;
        }

        //s = sample.get_node(succ).get_state();
        s = succ;
    }

    if (!add_every_state) {
        samp[s].reserve(heuristics.size());
        for (size_t i = 0; i < heuristics.size(); ++i) {
            succ_it = temporary_samp->find(s);
            double before = computation_timer();
            heuristics[i]->evaluate(succ_it->first);
            double after = computation_timer();
            computation_time[i] += after - before;
            int val = numeric_limits<int>::max();
            if (!heuristics[i]->is_dead_end()) {
                val = heuristics[i]->get_heuristic();
            }
            samp[s].push_back(val);
        }
    }
}
