#include "landmark_factory.h"

using namespace std;

LandmarkFactory::LandmarkFactory(LandmarkGraph::Options &options, Exploration *exploration) 
    : lm_graph(new LandmarkGraph(options, exploration)) {
}

LandmarkGraph *LandmarkFactory::compute_lm_graph() {
    lm_graph->read_external_inconsistencies();
    generate_landmarks();
    
    // the following replaces the old "build_lm_graph"
    lm_graph->generate();
    if (lm_graph->number_of_landmarks() == 0)
        cout << "Warning! No landmarks found. Task unsolvable?" << endl;
    cout << "Discovered " << lm_graph->number_of_landmarks()
    << " landmarks, of which " << lm_graph->number_of_disj_landmarks()
    << " are disjunctive and "
    << lm_graph->number_of_conj_landmarks() << " are conjunctive \n"
    << lm_graph->number_of_edges() << " edges\n";
    return lm_graph;
}
