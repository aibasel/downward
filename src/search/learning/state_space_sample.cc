#include "state_space_sample.h"

#include "../globals.h"
#include "../rng.h"


StateSpaceSample::StateSpaceSample() {
    uniform_sampling = false;
}

StateSpaceSample::~StateSpaceSample() {
}

int StateSpaceSample::choose_operator(vector<int> &h_s) {
    int ret = 0;
    if (uniform_sampling) {
        ret = g_rng(h_s.size());
    } else {
        double sum_inv = 0;
        for (size_t i = 0; i < h_s.size(); ++i) {
            sum_inv = sum_inv + (1.0 / (double)h_s[i]);
        }
        double val = g_rng() * sum_inv;
        ret = -1;
        while (val >= 0) {
            ++ret;
            val = val - (1.0 / (double)h_s[ret]);
        }
    }
    return ret;
}
