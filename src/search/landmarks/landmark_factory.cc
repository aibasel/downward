#include "landmark_factory.h"

using namespace std;

LandmarkFactory::LandmarkFactory(LandmarkGraph::Options &options, Exploration *exploration) 
    : lm_graph(new LandmarkGraph(options, exploration)) {
}

LandmarkGraph *LandmarkFactory::compute_lm_graph() {
    lm_graph->read_external_inconsistencies();
    generate_landmarks();
    LandmarkGraph::build_lm_graph(lm_graph);
    return lm_graph;
}
