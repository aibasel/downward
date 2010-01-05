#ifndef PDBSTATESPACESAMPLE_H
#define PDBSTATESPACESAMPLE_H

#include "probe_state_space_sample.h"
#include "../heuristic.h"

class PDBStateSpaceSample: public ProbeStateSpaceSample {
protected:
	double *prob_table;

	int get_random_depth();
public:
	PDBStateSpaceSample(Heuristic *h, int goal_depth, int probes, int size);
	virtual ~PDBStateSpaceSample();

	virtual int collect();
};

#endif
