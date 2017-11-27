#include "lama_ff_synergy.h"

#include "exploration.h"
#include "landmark_count_heuristic.h"
#include "landmark_factory_rpg_sasp.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../lp/lp_solver.h"

#include "../utils/system.h"

using namespace std;

namespace landmarks {
/*
  Implementation notes:

  There are four heuristic objects involved at the moment:
  - two externally visible ones (seen by the search), using the (local)
    classes LamaMasterHeuristic and FFSlaveHeuristic
  - two internal ones that appear within the LamaFFSynergy object only,
    using the "normal" landmark classes LandmarkCountHeuristic and
    Exploration.

  It should be quite possible to simplify this in the future. In
  principle, there is no need to have more than two heuristic objects.

  The "master" heuristic is the one that triggers the actual heuristic
  computation by asking the Synergy object to compute and store the
  heuristic values. The "slave" heuristic makes sure that the
  heuristic values stored in the synergy are up to date by asking for
  the master heuristic's value (which will trigger a computation if
  necessary).
*/

class LamaMasterHeuristic : public Heuristic {
    LamaFFSynergy *synergy;

protected:
    virtual int compute_heuristic(const GlobalState & /*state*/) override {
        ABORT("This method should never be called.");
    }

public:
    explicit LamaMasterHeuristic(LamaFFSynergy *synergy)
        : Heuristic(Heuristic::default_options()),
          synergy(synergy) {
    }

    virtual ~LamaMasterHeuristic() override = default;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override {
        synergy->compute_heuristics(eval_context);
        return synergy->lama_result;
    }

    virtual void notify_initial_state(const GlobalState &initial_state) override {
        synergy->lama_notify_initial_state(initial_state);
    }

    virtual bool notify_state_transition(
        const GlobalState &parent_state, OperatorID op_id,
        const GlobalState &state) override {
        if (synergy->lama_notify_state_transition(parent_state, op_id, state)) {
            if (cache_h_values) {
                heuristic_cache[state].dirty = true;
            }
            return true;
        }
        return false;
    }
};

class FFSlaveHeuristic : public Heuristic {
    LamaFFSynergy *synergy;
    LamaMasterHeuristic *master;

protected:
    virtual int compute_heuristic(const GlobalState & /*state*/) override {
        ABORT("This method should never be called.");
    }

public:
    explicit FFSlaveHeuristic(LamaFFSynergy *synergy,
                              LamaMasterHeuristic *master)
        : Heuristic(Heuristic::default_options()),
          synergy(synergy),
          master(master) {
    }

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override {
        /*
           Asking for the master's heuristic value triggers both
           heuristic computations if they have not been computed yet.
           If they have been computed yet, then both heuristic values
           are already cached, and this is just a quick lookup. In
           either case, the result is subsequently available in the
           synergy object.
        */
        eval_context.get_heuristic_value_or_infinity(master);
        return synergy->ff_result;
    }

    virtual ~FFSlaveHeuristic() override = default;
};


LamaFFSynergy::LamaFFSynergy(const Options &opts)
    : lama_master_heuristic(new LamaMasterHeuristic(this)),
      ff_slave_heuristic(new FFSlaveHeuristic(
                             this, lama_master_heuristic.get())),
      lama_heuristic(new LandmarkCountHeuristic(opts)) {
    cout << "Initializing LAMA-FF synergy object" << endl;
}

void LamaFFSynergy::compute_heuristics(EvaluationContext &eval_context) {
    /*
      When this method is called, we know that eval_context contains
      results for neither of the two synergy heuristics because the
      method isn't called when a heuristic results is already present,
      and the two results are always added to the evaluation context
      together.
    */

    lama_heuristic->exploration.set_recompute_heuristic();
    lama_result = lama_heuristic->compute_result(eval_context);
    ff_result = lama_heuristic->exploration.compute_result(eval_context);
}

void LamaFFSynergy::lama_notify_initial_state(const GlobalState &initial_state) {
    lama_heuristic->notify_initial_state(initial_state);
}

bool LamaFFSynergy::lama_notify_state_transition(
    const GlobalState &parent_state, OperatorID op_id,
    const GlobalState &state) {
    return lama_heuristic->notify_state_transition(parent_state, op_id, state);
}

Heuristic *LamaFFSynergy::get_lama_heuristic_proxy() const {
    return lama_master_heuristic.get();
}

Heuristic *LamaFFSynergy::get_ff_heuristic_proxy() const {
    return ff_slave_heuristic.get();
}

static Synergy *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "LAMA-FF synergy",
        "If the FF heuristic should be used "
        "(for its estimates or its preferred operators) "
        "and we want to use preferred operators of the "
        "landmark count heuristic, we can exploit synergy effects by "
        "using the LAMA-FF synergy. "
        "This synergy can only be used via Predefinition "
        "(see OptionSyntax#Predefinitions), for example:\n"
        "\"hlm,hff=lm_ff_syn(...)\"");
    parser.add_option<LandmarkFactory *>("lm_factory");
    parser.add_option<bool>("admissible", "get admissible estimate", "false");
    parser.add_option<bool>("optimal", "optimal cost sharing", "false");
    parser.add_option<bool>("alm", "use action landmarks", "true");
    lp::add_lp_solver_option_to_parser(parser);
    Heuristic::add_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    /*
      It does not make sense to use the synergy without preferred
      operators, so they are always enabled. (A landmark heuristic
      without preferred operators does not need to perform a relaxed
      exploration, hence no need for a synergy.)
    */
    opts.set("pref", true);

    LamaFFSynergy *lama_ff_synergy = new LamaFFSynergy(opts);
    Synergy *syn = new Synergy;
    syn->heuristics.push_back(lama_ff_synergy->get_lama_heuristic_proxy());
    syn->heuristics.push_back(lama_ff_synergy->get_ff_heuristic_proxy());
    return syn;
}


static PluginTypePlugin<Synergy> _type_plugin(
    "Synergy",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");

static Plugin<Synergy> _plugin("lm_ff_syn", _parse);
}
