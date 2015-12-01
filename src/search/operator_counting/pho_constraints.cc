#include "pho_constraints.h"

#include "../lp_solver.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../pdbs/pattern_database.h"
#include "../pdbs/pattern_generator.h"

#include <cassert>
#include <limits>
#include <vector>


namespace OperatorCounting {
PhOConstraints::PhOConstraints(const Options &opts)
    : pattern_generator(
          opts.get<shared_ptr<PatternCollectionGenerator>>("patterns")) {
}

void PhOConstraints::initialize_constraints(
    const std::shared_ptr<AbstractTask> task, vector<LPConstraint> &constraints,
    double infinity) {
    assert(pattern_generator);
    PatternCollectionInformation patterns = pattern_generator->generate(task);
    /*
      TODO issue590: Currently initialize_constraints should only be called
      once. When we separate constraint generators from constraints, we can
      create pattern_generator locally and no longer need to explicitly reset
      it.
    */
    pattern_generator = nullptr;
    pdbs = patterns.get_pdbs();
    TaskProxy task_proxy(*task);
    constraint_offset = constraints.size();
    for (const auto &pdb : *pdbs) {
        constraints.emplace_back(0, infinity);
        LPConstraint &constraint = constraints.back();
        for (OperatorProxy op : task_proxy.get_operators()) {
            if (pdb->is_operator_relevant(op)) {
                constraint.insert(op.get_id(), op.get_cost());
            }
        }
    }
}

bool PhOConstraints::update_constraints(const State &state,
                                        LPSolver &lp_solver) {
    for (size_t i = 0; i < pdbs->size(); ++i) {
        int constraint_id = constraint_offset + i;
        shared_ptr<PatternDatabase> pdb = (*pdbs)[i];
        int h = pdb->get_value(state);
        if (h == numeric_limits<int>::max()) {
            return true;
        }
        lp_solver.set_constraint_lower_bound(constraint_id, h);
    }
    return false;
}

static shared_ptr<ConstraintGenerator> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Posthoc optimization constraints",
        "The generator will compute a PDB for each pattern and add the "
        "constraint h(s) <= sum_{o in relevant(h)} Count_o. For details, see\n"
        " * Florian Pommerening, Gabriele Roeger and Malte Helmert.<<BR>>\n"
        " [Getting the Most Out of Pattern Databases for Classical Planning "
        "http://ijcai.org/papers13/Papers/IJCAI13-347.pdf].<<BR>>\n "
        "In //Proceedings of the Twenty-Third International Joint "
        "Conference on Artificial Intelligence (IJCAI 2013)//, "
        "pp. 2357-2364. 2013.\n\n\n");

    parser.add_option<shared_ptr<PatternCollectionGenerator>>(
        "patterns",
        "pattern generation method",
        "systematic(2)");

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PhOConstraints>(opts);
}

static PluginShared<ConstraintGenerator> _plugin("pho_constraints", _parse);
}
