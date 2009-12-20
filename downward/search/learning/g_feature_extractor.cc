#include "g_feature_extractor.h"

#include <cassert>

GFeatureExtractor::GFeatureExtractor(SearchSpace *space)
    :NodeInfoFeatureExtractor(space) {
    max_g = 100;

}

GFeatureExtractor::~GFeatureExtractor() {
}


int GFeatureExtractor::get_num_features() {
    return 1;
}

int GFeatureExtractor::get_feature_domain_size(int feature)
{
    assert( (feature >= 0) && (feature < get_num_features()) );
    if ((feature >= 0)&& (feature < get_num_features()))
        return (max_g + 1);
    else
        return -1;
}

void GFeatureExtractor::extract_features(const void *obj,
        vector<int>& features)
{
    const State &state = *((const State*) obj);
    int g = search_space->get_node(state).get_g();
    //cout << "KAKA " << g << endl;
    features.push_back( g );
}
