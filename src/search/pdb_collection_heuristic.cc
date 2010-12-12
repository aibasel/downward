#include "pdb_collection_heuristic.h"
#include "globals.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"

#include <cstdlib>
#include <limits>

using namespace std;

static ScalarEvaluatorPlugin canonical_heuristic_plugin("pdbs", PDBCollectionHeuristic::create);

PDBCollectionHeuristic::PDBCollectionHeuristic() {
    pattern_collection = 0;
}

PDBCollectionHeuristic::~PDBCollectionHeuristic() {
    delete pattern_collection;
}

void PDBCollectionHeuristic::initialize() {
    cout << "Initializing pattern database heuristic..." << endl;

    // Canonical heuristic function tests
    //1. one pattern logistics00 6-2
    /*int patt_1[6] = {3, 4, 5, 6, 7, 8};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    vector<vector<int> > pattern_collection(1);
    pattern_collection[0] = pattern_1;*/

    //2. two patterns logistics00 6-2
    /*int patt_1[3] = {3, 4, 5};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[3] = {6, 7, 8};
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    vector<vector<int> > patt_coll(2);
    patt_coll[0] = pattern_1;
    patt_coll[1] = pattern_2;*/

    //3. three patterns logistics00 6-2
    /*int patt_1[2] = {3, 4};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[2] = {5, 6};
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    int patt_3[2] = {7, 8};
    vector<int> pattern_3(patt_3, patt_3 + sizeof(patt_3) / sizeof(int));
    vector<vector<int> > patt_coll(3);
    patt_coll[0] = pattern_1;
    patt_coll[1] = pattern_2;
    patt_coll[2] = pattern_3;*/

    //1. one pattern driverlog 6
    /*int patt_1[7] = {4, 5, 7, 9, 10, 11, 12};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    vector<vector<int> > pattern_collection(1);
    pattern_collection[0] = pattern_1;*/

    //2. two patterns driverlog 6
    /*int patt_1[3] = {4, 5, 7};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[4] = {9, 10, 11, 12};
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    vector<vector<int> > pattern_collection(2);
    pattern_collection[0] = pattern_1;
    pattern_collection[1] = pattern_2;*/

    //3. three patterns driverlog 6
    /*int patt_1[2] = {4, 5};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[2] = {7, 9};
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    int patt_3[3] = {10, 11, 12};
    vector<int> pattern_3(patt_3, patt_3 + sizeof(patt_3) / sizeof(int));
    vector<vector<int> > pattern_collection(3);
    pattern_collection[0] = pattern_1;
    pattern_collection[1] = pattern_2;
    pattern_collection[2] = pattern_3;*/

    //1. one pattern blocks 7-2
    /*int patt_1[6] = {9, 10, 11, 12, 13, 14};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    vector<vector<int> > pattern_collection(1);
    pattern_collection[0] = pattern_1;*/

    //2. two patterns blocks 7-2
    /*int patt_1[3] = {9, 10, 11};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[3] = {12, 13, 14};
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    vector<vector<int> > pattern_collection(2);
    pattern_collection[0] = pattern_1;
    pattern_collection[1] = pattern_2;*/

    //3. three patterns blocks 7-2
    int patt_1[2] = {9, 10};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[2] = {11, 12};
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    int patt_3[2] = {13, 14};
    vector<int> pattern_3(patt_3, patt_3 + sizeof(patt_3) / sizeof(int));
    vector<vector<int> > patt_coll(3);
    patt_coll[0] = pattern_1;
    patt_coll[1] = pattern_2;
    patt_coll[2] = pattern_3;

    // additional two patterns for logistics00 9-1
    /*int patt_1[4] = {4, 5, 6, 7};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[5] = {8, 9, 10, 11, 12};
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    vector<vector<int> > pattern_collection(2);
    pattern_collection[0] = pattern_1;
    pattern_collection[1] = pattern_2;*/

    // additional five patterns for logistics00 9-1
    /*int patt_1[2] = {4, 5};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[2] = {6, 7};
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    int patt_3[2] = {8, 9};
    vector<int> pattern_3(patt_3, patt_3 + sizeof(patt_3) / sizeof(int));
    int patt_4[2] = {10, 11};
    vector<int> pattern_4(patt_4, patt_4 + sizeof(patt_4) / sizeof(int));
    int patt_5[1] = {12};
    vector<int> pattern_5(patt_5, patt_5 + sizeof(patt_5) / sizeof(int));
    vector<vector<int> > pattern_collection(5);
    pattern_collection[0] = pattern_1;
    pattern_collection[1] = pattern_2;
    pattern_collection[2] = pattern_3;
    pattern_collection[3] = pattern_4;
    pattern_collection[4] = pattern_5;*/

    pattern_collection = new PatternCollection(patt_coll);
}

int PDBCollectionHeuristic::compute_heuristic(const State &state) {
    int h = pattern_collection->get_heuristic_value(state);
        if (h == numeric_limits<int>::max())
            return -1;
    return h;
}

ScalarEvaluator *PDBCollectionHeuristic::create(const vector<string> &config, int start, int &end, bool dry_run) {
    //TODO: check what we have to do here!
    OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    else
        return new PDBCollectionHeuristic;
}
