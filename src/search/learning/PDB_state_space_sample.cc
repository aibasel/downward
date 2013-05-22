#include "PDB_state_space_sample.h"

#include "../globals.h"
#include "../rng.h"

#include <cmath>
using namespace std;


static double fac(int n) {
    double t = 1.0;
    for (int i = n; i > 1; i--)
        t *= i;
    return t;
}

static double bin(int n, double p, int r) {
    return fac(n) / (fac(n - r) * fac(r)) * pow(p, r) * pow(1 - p, n - r);
}

PDBStateSpaceSample::PDBStateSpaceSample(int goal_depth, int probes = 10, int size = 100)
    : ProbeStateSpaceSample(goal_depth, probes, size) {
    int table_size = (2 * goal_depth) + 1;
    double p = 0.5;

    prob_table = new double[table_size];

    prob_table[0] = bin(goal_depth / p, p, 0);
    for (int i = 1; i < (table_size - 1); i++) {
        prob_table[i] = prob_table[i - 1] + bin(goal_depth / p, p, i);
    }
    prob_table[table_size - 1] = 1.0;

    uniform_sampling = true;
    add_every_state = false;
}

PDBStateSpaceSample::~PDBStateSpaceSample() {
    delete prob_table;
}

int PDBStateSpaceSample::get_random_depth() {
    double p = g_rng();
    int d = 0;
    while (prob_table[d] < p) {
        d++;
    }
    return d;
}

int PDBStateSpaceSample::collect() {
    cout << "PDB-style state space sample" << endl;
    int num_probes = 0;
    while ((samp.size() < min_training_set_size) && (num_probes < max_num_probes)) {
        sample_t temp_sample;
        temporary_samp = &temp_sample;

        num_probes++;

        int depth = get_random_depth();
        for (int j = 0; j < heuristics.size(); j++) {
            heuristics[j]->reset();
        }
        send_probe(depth);
        //cout << "Probe: " << num_probes << " - " << samp.size() << endl;
    }

    branching_factor = (double)generated / (double)expanded;

    return samp.size();
}
