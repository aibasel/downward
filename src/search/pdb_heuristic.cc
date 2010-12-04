#include "pdb_heuristic.h"
#include "globals.h"
//#include "option_parser.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"
#include "timer.h"

#include <limits>
#include <cstdlib>

static ScalarEvaluatorPlugin pdb_heuristic_plugin("pdb", PDBHeuristic::create);

PDBHeuristic::PDBHeuristic() {
}

PDBHeuristic::~PDBHeuristic() {
    delete pdb_abstraction;
}

void PDBHeuristic::verify_no_axioms_no_cond_effects() const {
    if (!g_axioms.empty()) {
        cerr << "Heuristic does not support axioms!" << endl << "Terminating." << endl;
        exit(1);
    }
    for (int i = 0; i < g_operators.size(); ++i) {
        const vector<PrePost> &pre_post = g_operators[i].get_pre_post();
        for (int j = 0; j < pre_post.size(); ++j) {
            const vector<Prevail> &cond = pre_post[j].cond;
            if (cond.empty())
                continue;
            // Accept conditions that are redundant, but nothing else.
            // In a better world, these would never be included in the
            // input in the first place.
            int var = pre_post[j].var;
            int pre = pre_post[j].pre;
            int post = pre_post[j].post;
            if (pre == -1 && cond.size() == 1 &&
                cond[0].var == var && cond[0].prev != post &&
                g_variable_domain[var] == 2)
                continue;
            
            cerr << "Heuristic does not support conditional effects "
            << "(operator " << g_operators[i].get_name() << ")"
            << endl << "Terminating." << endl;
            exit(1);
        }
    }
}

void PDBHeuristic::initialize() {
    cout << "Initializing pattern database heuristic..." << endl;
    verify_no_axioms_no_cond_effects();
    
    // function tests
    // 1. blocks-7-2 test-pattern
    //int patt[] = {9, 10, 11, 12, 13, 14};
    
    // 2. driverlog-6 test-pattern
    //int patt[] = {4, 5, 7, 9, 10, 11, 12};
    
    // 3. logistics00-6-2 test-pattern
    //int patt[] = {3, 4, 5, 6, 7, 8};
    
    // 4. blocks-9-0 test-pattern
    //int patt[] = {0};
    
    // 5. logistics00-5-1 test-pattern
    int patt[] = {0, 1, 2, 3, 4, 5, 6, 7};
    
    vector<int> pattern(patt, patt + sizeof(patt) / sizeof(int));
    Timer timer;
    timer();
    pdb_abstraction = new PDBAbstraction(pattern);
    timer.stop();
    cout << "PDB construction time: " << timer << endl;
    //pdb_abstraction->dump();

    cout << "Done initializing." << endl;
}

int PDBHeuristic::compute_heuristic(const State &state) {
    int h = pdb_abstraction->get_heuristic_value(state);
    if (h == numeric_limits<int>::max())
        return -1;
    return h;
}

ScalarEvaluator *PDBHeuristic::create(const vector<string> &config, int start, int &end, bool dry_run) {
    //TODO: check what we have to do here!
    OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    else
        return new PDBHeuristic;
}
