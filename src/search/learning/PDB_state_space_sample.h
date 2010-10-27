#ifndef LEARNING_PDB_STATE_SPACE_SAMPLE_H
#define LEARNING_PDB_STATE_SPACE_SAMPLE_H

#include "probe_state_space_sample.h"
#include "../heuristic.h"

class PDBStateSpaceSample : public ProbeStateSpaceSample {
protected:
    double *prob_table;

    int get_random_depth();
public:
    PDBStateSpaceSample(int goal_depth, int probes, int size);
    virtual ~PDBStateSpaceSample();

    virtual int collect();
};

#endif
