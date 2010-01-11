#ifndef PDBSTATESPACESAMPLE_H_
#define PDBSTATESPACESAMPLE_H_

#include "probe_state_space_sample.h"
#include "../heuristic.h"

class PDBStateSpaceSample: public ProbeStateSpaceSample {
protected:
	double *prob_table;

	int get_random_depth();
public:
	PDBStateSpaceSample(int goal_depth, int probes, int size);
	virtual ~PDBStateSpaceSample();

	virtual int collect();
};

#endif /* PDBSTATESPACESAMPLE_H_ */
