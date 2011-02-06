#include "landmark_factory.h"

using namespace std;

LandmarkFactory::LandmarkFactory(LandmarkGraph::Options &options, Exploration *exploration) 
    : lm_graph(new LandmarkGraph(options, exploration)) {
    // do something;
}
