#include "lama_ff_synergy.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "landmarks_graph_rpg_sasp.h"

static SynergyPlugin lama_ff_synergy_plugin(
    "lm_ff_syn", LamaFFSynergy::create_heuristics);


LamaFFSynergy::HeuristicProxy::HeuristicProxy(LamaFFSynergy *synergy_)
    : Heuristic(g_default_heuristic_options) {
    synergy = synergy_;
    is_first_proxy = false;
}

void LamaFFSynergy::HeuristicProxy::initialize() {
    if (!synergy->initialized) {
        synergy->initialize();
        is_first_proxy = true;
    }
}

LamaFFSynergy::LamaFFSynergy(const HeuristicOptions &options,
                             LandmarksGraph &lm_graph,
                             bool lm_pref_, bool lm_admissible_, bool lm_optimal_,
                             bool use_action_landmarks_)
    : lama_heuristic_proxy(this), ff_heuristic_proxy(this),
      lm_pref(lm_pref_), lm_admissible(lm_admissible_),
      lm_optimal(lm_optimal_),
      use_action_landmarks(use_action_landmarks_) {
    cout << "Initializing LAMA-FF Synergy Object" << endl;
    lama_heuristic = new LandmarkCountHeuristic(options, lm_graph,
                                                lm_pref, lm_admissible, lm_optimal,
                                                use_action_landmarks_);
    //lama_heuristic->initialize(); // must be called here explicitly
    exploration = lama_heuristic->get_exploration();
    initialized = false;
}

void LamaFFSynergy::get_lama_preferred_operators(std::vector<const Operator *> &result) {
    result.insert(result.end(),
                  lama_preferred_operators.begin(),
                  lama_preferred_operators.end());
}

void LamaFFSynergy::get_ff_preferred_operators(std::vector<const Operator *> &result) {
    result.insert(result.end(),
                  ff_preferred_operators.begin(),
                  ff_preferred_operators.end());
}

void LamaFFSynergy::compute_heuristics(const State &state) {
    /* Compute heuristics and pref. ops. and store results;
       actual work is delegated to the heuristics. */

    exploration->set_recompute_heuristic();
    lama_preferred_operators.clear();
    ff_preferred_operators.clear();

    lama_heuristic->evaluate(state);
    if (!lama_heuristic->is_dead_end()) {
        lama_heuristic_value = lama_heuristic->get_heuristic();
        lama_heuristic->get_preferred_operators(lama_preferred_operators);
    } else {
        lama_heuristic_value = -1;
    }

    exploration->evaluate(state);
    if (!exploration->is_dead_end()) {
        ff_heuristic_value = exploration->get_heuristic();
        exploration->get_preferred_operators(ff_preferred_operators);
    } else {
        ff_heuristic_value = -1;
    }
}

bool LamaFFSynergy::lama_reach_state(const State &parent_state,
                                     const Operator &op, const State &state) {
    return lama_heuristic->reach_state(parent_state, op, state);
}

void
LamaFFSynergy::create_heuristics(const std::vector<string> &config,
                                 int start, int &end,
                                 vector<Heuristic *> &heuristics) {
    bool lm_admissible_ = false;
    bool lm_optimal_ = false;
    bool use_action_landmarks_ = true;
    HeuristicOptions common_options;

    if (config[start + 1] != "(")
        throw ParseError(start + 1);

    LandmarksGraph *lm_graph = OptionParser::instance()->parse_lm_graph(
        config, start + 2, end, false);
    ++end;

    if (lm_graph == 0)
        throw ParseError(start);

    if (config[end] != ")") {
        end++;
        NamedOptionParser option_parser;

        common_options.add_option_to_parser(option_parser);

        option_parser.add_bool_option("admissible",
                                      &lm_admissible_,
                                      "get admissible estimate");
        option_parser.add_bool_option("optimal",
                                      &lm_optimal_,
                                      "optimal cost sharing");
        option_parser.add_bool_option("alm",
                                      &use_action_landmarks_,
                                      "use action landmarks");
        option_parser.parse_options(config, end, end, false);
        ++end;
    }
    if (config[end] != ")")
        throw ParseError(end);

    bool lm_pref_ = true; // this will always be the case because it
                          // does not make sense to use the synergy without
                          // using lm preferred operators
    LamaFFSynergy *lama_ff_synergy = new LamaFFSynergy(
        common_options, *lm_graph,
        lm_pref_, lm_admissible_,
        lm_optimal_, use_action_landmarks_);

    heuristics.push_back(lama_ff_synergy->get_lama_heuristic_proxy());
    heuristics.push_back(lama_ff_synergy->get_ff_heuristic_proxy());
    return;
}
