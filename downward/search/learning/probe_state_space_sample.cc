#include "probe_state_space_sample.h"
#include "../successor_generator.h"
#include "../heuristic.h"
#include <climits>

ProbeStateSpaceSample::ProbeStateSpaceSample(Heuristic *h, int goal_depth, int probes = 10, int size = 100):
	heuristic(h), goal_depth_estimate(goal_depth), max_num_probes(probes), min_training_set_size(size)
{
	expanded = 0;
	generated = 0;
}

ProbeStateSpaceSample::~ProbeStateSpaceSample() {
}

int ProbeStateSpaceSample::collect() {
	cout << "Probe state space sample" << endl;
	int num_probes = 0;
	while ((sample.size() < min_training_set_size) && (num_probes < max_num_probes)) {
		num_probes++;
		//cout << "Probe: " << num_probes << " - " << sample.size() << endl;

		send_probe(goal_depth_estimate);
	}

	branching_factor = (double) generated / (double) expanded;

	return sample.size();
}

void ProbeStateSpaceSample::send_probe(int depth_limit) {
	vector<const Operator *> applicable_ops;
	vector<int> h_s;

	State s = *g_initial_state;
	if (sample.get_node(s).is_new()) {
		heuristic->evaluate(*g_initial_state);
		sample.get_node(s).open_initial(heuristic->get_heuristic());
	}
	for (int i = 0; (i < depth_limit) && (sample.size() < min_training_set_size); i++) {
		SearchNode node = sample.get_node(s);
		expanded++;
		applicable_ops.clear();
		g_successor_generator->generate_applicable_ops(s, applicable_ops);

		if (applicable_ops.size() == 0) {
			break;
		}
		generated = generated + applicable_ops.size();
		h_s.resize(applicable_ops.size());

		for (int op_num = 0; op_num < applicable_ops.size(); op_num++) {
			// generate and add to training set all successors
			const Operator *op = applicable_ops[op_num];
			State succ(s, *op);
			SearchNode succ_node = sample.get_node(succ);

			heuristic->reach_state(s, *op, succ);
			heuristic->evaluate(succ);
			if (heuristic->is_dead_end()) {
				h_s[op_num] = INT_MAX;
			}
			else {
				h_s[op_num] = heuristic->get_heuristic();
			}
			//cout << op->get_name() << " - " << h_s[op_num] << endl;

			succ_node.reopen(node, op);
			succ_node.update_h(h_s[op_num]);
		}

		// choose operator at random
		int op_num = choose_operator(applicable_ops.size(), h_s);

		const Operator *op = applicable_ops[op_num];

		//cout << op->get_name() << endl;

		State succ(s, *op);
		SearchNode succ_node = sample.get_node(succ);

		heuristic->reach_state(s, *op, succ);
		if (succ_node.is_goal()) {
			break;
		}

		s = sample.get_node(succ).get_state();
	}
}
