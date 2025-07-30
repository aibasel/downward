#include "pho_constraints.h"

#include "../lp/lp_solver.h"
#include "../plugins/plugin.h"
#include "../pdbs/pattern_database.h"
#include "../pdbs/pattern_generator.h"
#include "../pdbs/utils.h"
#include "../utils/markup.h"

#include <cassert>
#include <limits>
#include <memory>
#include <vector>

using namespace std;

namespace operator_counting {
PhOConstraints::PhOConstraints(
    const shared_ptr<pdbs::PatternCollectionGenerator> &patterns)
    : pattern_generator(patterns) {
}

void PhOConstraints::initialize_constraints(
    const shared_ptr<AbstractTask> &task, lp::LinearProgram &lp) {
    assert(pattern_generator);
    pdbs::PatternCollectionInformation pattern_collection_info =
        pattern_generator->generate(task);
    /*
      TODO issue590: Currently initialize_constraints should only be called
      once. When we separate constraint generators from constraints, we can
      create pattern_generator locally and no longer need to explicitly reset
      it.
    */
    pdbs = pattern_collection_info.get_pdbs();
    pattern_generator = nullptr;
    TaskProxy task_proxy(*task);
    named_vector::NamedVector<lp::LPConstraint> &constraints = lp.get_constraints();
    constraint_offset = constraints.size();
    for (const shared_ptr<pdbs::PatternDatabase> &pdb : *pdbs) {
        constraints.emplace_back(0, lp.get_infinity());
        lp::LPConstraint &constraint = constraints.back();
        for (OperatorProxy op : task_proxy.get_operators()) {
            if (pdbs::is_operator_relevant(pdb->get_pattern(), op)) {
                constraint.insert(op.get_id(), op.get_cost());
            }
        }
    }
}

bool PhOConstraints::update_constraints(const State &state,
                                        lp::LPSolver &lp_solver) {
    state.unpack();
    for (size_t i = 0; i < pdbs->size(); ++i) {
        int constraint_id = constraint_offset + i;
        shared_ptr<pdbs::PatternDatabase> pdb = (*pdbs)[i];
        int h = pdb->get_value(state.get_unpacked_values());
        if (h == numeric_limits<int>::max()) {
            return true;
        }
        lp_solver.set_constraint_lower_bound(constraint_id, h);
    }
    return false;
}

class PhOConstraintsFeature
    : public plugins::TypedFeature<ConstraintGenerator, PhOConstraints> {
public:
    PhOConstraintsFeature() : TypedFeature("pho_constraints") {
        document_title("Posthoc optimization constraints");
        document_synopsis(
            "The generator will compute a PDB for each pattern and add the"
            " constraint h(s) <= sum_{o in relevant(h)} Count_o. For details,"
            " see" + utils::format_conference_reference(
                {"Florian Pommerening", "Gabriele Roeger", "Malte Helmert"},
                "Getting the Most Out of Pattern Databases for Classical Planning",
                "http://ijcai.org/papers13/Papers/IJCAI13-347.pdf",
                "Proceedings of the Twenty-Third International Joint"
                " Conference on Artificial Intelligence (IJCAI 2013)",
                "2357-2364",
                "AAAI Press",
                "2013"));

        add_option<shared_ptr<pdbs::PatternCollectionGenerator>>(
            "patterns",
            "pattern generation method",
            "systematic(2)");
    }

    virtual shared_ptr<PhOConstraints>
    create_component(const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<PhOConstraints>(
            opts.get<shared_ptr<pdbs::PatternCollectionGenerator>>(
                "patterns"));
    }
};

static plugins::FeaturePlugin<PhOConstraintsFeature> _plugin;
}
