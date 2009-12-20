#include "state_space_sample.h"

StateSpaceSample::StateSpaceSample() {
	uniform_sampling = false;
}

StateSpaceSample::~StateSpaceSample() {
	sample.resize(0);
}

int StateSpaceSample::choose_operator(int num_ops, vector<int> &h_s) {
	int ret = 0;
	if (uniform_sampling) {
		ret = rand() % num_ops;
	}
	else {
		double sum_inv = 0;
		for (int i = 0; i < num_ops; i++) {
			sum_inv = sum_inv + (1.0 / (double) h_s[i]);
		}
		double val = drand48() * sum_inv;
		ret = -1;
		while (val >= 0) {
			ret++;
			val = val - (1.0 / (double) h_s[ret]);
		}
	}
	return ret;
}
