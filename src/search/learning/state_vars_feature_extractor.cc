#include <cassert>

#include "state_vars_feature_extractor.h"
#include "../global_state.h"
#include "../globals.h"

StateVarFeatureExtractor::StateVarFeatureExtractor() {
}

StateVarFeatureExtractor::~StateVarFeatureExtractor() {
}

int StateVarFeatureExtractor::get_num_features() {
    return g_variable_domain.size();
}

int StateVarFeatureExtractor::get_feature_domain_size(int feature) {
    assert((feature >= 0) && (feature < get_num_features()));
    return g_variable_domain[feature];
}

void StateVarFeatureExtractor::extract_features(const void *obj,
                                                vector<int> &features) {
    const GlobalState &state = *((const GlobalState *)obj);
    //cout << "kaka " << get_num_features() << endl;
    for (int i = 0; i < get_num_features(); ++i) {
        features.push_back(state[i]);
        //cout << i << " : " << (int) state[i] << "   " << features[i] << endl;
    }
}
