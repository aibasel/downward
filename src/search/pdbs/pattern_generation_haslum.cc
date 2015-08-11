#include "pattern_generation_haslum.h"

#include "canonical_pdbs_heuristic.h"
#include "pattern_database.h"

#include "../causal_graph.h"
#include "../countdown_timer.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../rng.h"
#include "../sampling.h"
#include "../task_tools.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

struct HillClimbingTimeout : public exception {};


PatternGenerationHaslum::PatternGenerationHaslum(const Options &opts)
    : task(get_task_from_options(opts)),
      task_proxy(*task),
      pdb_max_size(opts.get<int>("pdb_max_size")),
      collection_max_size(opts.get<int>("collection_max_size")),
      num_samples(opts.get<int>("num_samples")),
      min_improvement(opts.get<int>("min_improvement")),
      max_time(opts.get<double>("max_time")),
      cost_type(OperatorCost(opts.get<int>("cost_type"))),
      successor_generator(task),
      num_rejected(0),
      hill_climbing_timer(0) {
    Timer timer;
    initialize();
    cout << "Pattern generation (Haslum et al.) time: " << timer << endl;
}

PatternGenerationHaslum::~PatternGenerationHaslum() {
}

void PatternGenerationHaslum::generate_candidate_patterns(
    const PatternDatabase *pdb, vector<vector<int> > &candidate_patterns) {
    const CausalGraph &causal_graph = task_proxy.get_causal_graph();
    const vector<int> &pattern = pdb->get_pattern();
    int pdb_size = pdb->get_size();
    for (size_t i = 0; i < pattern.size(); ++i) {
        /* Only consider variables used in preconditions for current
           variable from pattern. It would also make sense to consider
           *goal* variables connected by effect-effect arcs, but we
           don't. This may be worth experimenting with. */
        const vector<int> &rel_vars = causal_graph.get_eff_to_pre(pattern[i]);
        vector<int> relevant_vars;
        // Only use relevant variables which are not already in pattern.
        set_difference(rel_vars.begin(), rel_vars.end(),
                       pattern.begin(), pattern.end(),
                       back_inserter(relevant_vars));
        for (int rel_var_id : relevant_vars) {
            VariableProxy rel_var = task_proxy.get_variables()[rel_var_id];
            int rel_var_size = rel_var.get_domain_size();
            if (is_product_within_limit(pdb_size, rel_var_size, pdb_max_size)) {
                vector<int> new_pattern(pattern);
                new_pattern.push_back(rel_var_id);
                sort(new_pattern.begin(), new_pattern.end());
                candidate_patterns.push_back(new_pattern);
            } else {
                ++num_rejected;
            }
        }
    }
}

size_t PatternGenerationHaslum::generate_pdbs_for_candidates(
    set<vector<int> > &generated_patterns, vector<vector<int> > &new_candidates,
    vector<PatternDatabase *> &candidate_pdbs) const {
    /*
      For the new candidate patterns check whether they already have been
      candidates before and thus already a PDB has been created an inserted into
      candidate_pdbs.
    */
    size_t max_pdb_size = 0;
    for (const vector<int> &new_candidate : new_candidates) {
        if (generated_patterns.count(new_candidate) == 0) {
            candidate_pdbs.push_back(new PatternDatabase(task, new_candidate));
            max_pdb_size = max(max_pdb_size,
                               candidate_pdbs.back()->get_size());
            generated_patterns.insert(new_candidate);
        }
    }
    return max_pdb_size;
}

void PatternGenerationHaslum::sample_states(
    vector<State> &samples, double average_operator_cost) {
    int init_h = current_heuristic->compute_heuristic(
        task_proxy.get_initial_state());

    try {
        samples = sample_states_with_random_walks(
            task_proxy, successor_generator, num_samples, init_h, average_operator_cost,
            [this](const State &state) {return current_heuristic->is_dead_end(state);
            },
            hill_climbing_timer);
    } catch (SamplingTimeout &) {
        throw HillClimbingTimeout();
    }
}

std::pair<int, int> PatternGenerationHaslum::find_best_improving_pdb(
    vector<State> &samples,
    vector<PatternDatabase *> &candidate_pdbs) {
    /*
      TODO: The original implementation by Haslum et al. uses A* to compute
      h values for the sample states only instead of generating all PDBs.
      improvement: best improvement (= highest count) for a pattern so far.
      We require that a pattern must have an improvement of at least one in
      order to be taken into account.
    */
    int improvement = 0;
    int best_pdb_index = -1;

    // Iterate over all candidates and search for the best improving pattern/pdb
    for (size_t i = 0; i < candidate_pdbs.size(); ++i) {
        if (hill_climbing_timer->is_expired())
            throw HillClimbingTimeout();

        PatternDatabase *pdb = candidate_pdbs[i];
        if (!pdb) {
            /* candidate pattern is too large or has already been added to
               the canonical heuristic. */
            continue;
        }
        /*
          If a candidate's size added to the current collection's size exceeds
          the maximum collection size, then delete the pdb and let the pdb's
          entry point to a null reference
        */
        int combined_size = current_heuristic->get_size() + pdb->get_size();
        if (combined_size > collection_max_size) {
            delete pdb;
            candidate_pdbs[i] = nullptr;
            continue;
        }

        /*
          Calculate the "counting approximation" for all sample states: count
          the number of samples for which the current pattern collection
          heuristic would be improved if the new pattern was included into it.
        */
        /*
          TODO: The original implementation by Haslum et al. uses m/t as a
          statistical confidence interval to stop the A*-search (which they use,
          see above) earlier.
        */
        int count = 0;
        vector<vector<PatternDatabase *> > max_additive_subsets;
        current_heuristic->get_max_additive_subsets(
            pdb->get_pattern(), max_additive_subsets);
        for (State &sample : samples) {
            if (is_heuristic_improved(pdb, sample, max_additive_subsets))
                ++count;
        }
        if (count > improvement) {
            improvement = count;
            best_pdb_index = i;
        }
        if (count > 0) {
            cout << "pattern: " << candidate_pdbs[i]->get_pattern()
                 << " - improvement: " << count << endl;
        }
    }

    return make_pair(improvement, best_pdb_index);
}

bool PatternGenerationHaslum::is_heuristic_improved(
    PatternDatabase *pdb, const State &sample,
    const vector<vector<PatternDatabase *> > &max_additive_subsets) {
    // h_pattern: h-value of the new pattern
    int h_pattern = pdb->get_value(sample);

    if (h_pattern == numeric_limits<int>::max()) {
        return true;
    }

    /*
      TODO: we still compute the value of each pdb twice:
      once inside current_heuristic and once as part of max_additive_subsets.
    */
    unordered_map<PatternDatabase *, int> pdb_h_values;
    pdb_h_values.reserve(current_heuristic->get_pattern_databases().size());

    // h_collection: h-value of the current collection heuristic
    int h_collection = current_heuristic->compute_heuristic(sample);
    for (auto &subset : max_additive_subsets) {
        int h_subset = 0;
        for (PatternDatabase *additive_pdb : subset) {
            if (!pdb_h_values.count(additive_pdb))
                pdb_h_values[additive_pdb] = additive_pdb->get_value(sample);
            h_subset += pdb_h_values[additive_pdb];
        }
        if (h_pattern + h_subset > h_collection) {
            /*
              return true if a max additive subset is found for
              which the condition is met
            */
            return true;
        }
    }
    return false;
}

void PatternGenerationHaslum::hill_climbing(
    double average_operator_cost,
    vector<vector<int> > &initial_candidate_patterns) {
    hill_climbing_timer = new CountdownTimer(max_time);
    // Candidate patterns generated so far (used to avoid duplicates).
    set<vector<int> > generated_patterns;
    /* Set of new pattern candidates from the last call to
       generate_candidate_patterns */
    vector<vector<int> > &new_candidates = initial_candidate_patterns;
    // All candidate patterns are converted into pdbs once and stored
    vector<PatternDatabase *> candidate_pdbs;
    int num_iterations = 0;
    size_t max_pdb_size = 0;
    State initial_state = task_proxy.get_initial_state();
    try {
        while (true) {
            ++num_iterations;
            cout << "current collection size is "
                 << current_heuristic->get_size() << endl;
            cout << "current initial h value: ";
            if (current_heuristic->is_dead_end(initial_state)) {
                cout << "infinite => stopping hill climbing" << endl;
                break;
            } else {
                cout << current_heuristic->compute_heuristic(initial_state)
                     << endl;
            }

            size_t new_max_pdb_size = generate_pdbs_for_candidates(
                generated_patterns, new_candidates, candidate_pdbs);
            max_pdb_size = max(max_pdb_size, new_max_pdb_size);

            vector<State> samples;
            sample_states(samples, average_operator_cost);

            pair<int, int> improvement_and_index =
                find_best_improving_pdb(samples, candidate_pdbs);
            int improvement = improvement_and_index.first;
            int best_pdb_index = improvement_and_index.second;

            if (improvement < min_improvement) {
                cout << "Improvement below threshold. Stop hill climbing."
                     << endl;
                break;
            }

            // Add the best pattern to the CanonicalPDBsHeuristic.
            assert(best_pdb_index != -1);
            const PatternDatabase *best_pdb = candidate_pdbs[best_pdb_index];
            const vector<int> &best_pattern = best_pdb->get_pattern();
            cout << "found a better pattern with improvement " << improvement
                 << endl;
            cout << "pattern: " << best_pattern << endl;
            current_heuristic->add_pattern(best_pattern);

            /* Clear current new_candidates and get successors for next
               iteration. */
            new_candidates.clear();
            generate_candidate_patterns(best_pdb, new_candidates);

            // remove from candidate_pdbs the added PDB
            delete candidate_pdbs[best_pdb_index];
            candidate_pdbs[best_pdb_index] = nullptr;

            cout << "Hill climbing time so far: " << *hill_climbing_timer
                 << endl;
        }
    } catch (HillClimbingTimeout &) {
        cout << "Time limit reached. Abort hill climbing." << endl;
    }

    // Note that using dominance pruning during hill climbing could lead to
    // fewer discovered patterns and pattern collections.
    // A dominated pattern (collection) might no longer be dominated
    // after more patterns are added.
    current_heuristic->dominance_pruning();
    cout << "iPDB: iterations = " << num_iterations << endl;
    cout << "iPDB: num_patterns = "
         << current_heuristic->get_pattern_databases().size() << endl;
    cout << "iPDB: size = " << current_heuristic->get_size() << endl;
    cout << "iPDB: generated = " << generated_patterns.size() << endl;
    cout << "iPDB: rejected = " << num_rejected << endl;
    cout << "iPDB: max_pdb_size = " << max_pdb_size << endl;
    cout << "iPDB: hill climbing time: " << *hill_climbing_timer << endl;

    // delete all created PDB-pointer
    for (size_t i = 0; i < candidate_pdbs.size(); ++i) {
        delete candidate_pdbs[i];
    }

    delete hill_climbing_timer;
    hill_climbing_timer = nullptr;
}

void PatternGenerationHaslum::initialize() {
    double average_operator_cost = get_average_operator_cost(task_proxy);
    cout << "Average operator cost: " << average_operator_cost << endl;

    // Generate initial collection: a pdb for each goal variable.
    vector<vector<int> > initial_pattern_collection;
    for (FactProxy goal : task_proxy.get_goals()) {
        int goal_var_id = goal.get_variable().get_id();
        initial_pattern_collection.emplace_back(1, goal_var_id);
    }
    Options opts;
    opts.set<shared_ptr<AbstractTask> >("transform", task);
    // Since we pass a task transformation, cost_type won't be used.
    opts.set<int>("cost_type", NORMAL);
    opts.set<vector<vector<int> > >("patterns", initial_pattern_collection);
    current_heuristic = new CanonicalPDBsHeuristic(opts);

    State initial_state = task_proxy.get_initial_state();
    if (current_heuristic->is_dead_end(initial_state))
        return;

    /* Generate initial candidate patterns (based on each pattern from
       the initial collection). */
    vector<vector<int> > initial_candidate_patterns;
    for (const PatternDatabase *current_pdb :
         current_heuristic->get_pattern_databases()) {
        generate_candidate_patterns(current_pdb, initial_candidate_patterns);
    }
    // Remove duplicates in the candidate list.
    sort(initial_candidate_patterns.begin(), initial_candidate_patterns.end());
    initial_candidate_patterns.erase(unique(initial_candidate_patterns.begin(),
                                            initial_candidate_patterns.end()),
                                     initial_candidate_patterns.end());
    cout << "done calculating initial pattern collection and "
         << "candidate patterns for the search" << endl;

    if (max_time > 0)
        /* A call to the following method modifies initial_candidate_patterns
           (contains the new_candidates after each call to
           generate_candidate_patterns) */
        hill_climbing(average_operator_cost, initial_candidate_patterns);
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "iPDB",
        "This pattern generation method is an adaption of the algorithm "
        "described in the following paper:\n\n"
        " * Patrik Haslum, Adi Botea, Malte Helmert, Blai Bonet and "
        "Sven Koenig.<<BR>>\n"
        " [Domain-Independent Construction of Pattern Database Heuristics for "
        "Cost-Optimal Planning http://www.informatik.uni-freiburg.de/~ki"
        "/papers/haslum-etal-aaai07.pdf].<<BR>>\n "
        "In //Proceedings of the 22nd AAAI Conference on Artificial "
        "Intelligence (AAAI 2007)//, pp. 1007-1012. AAAI Press 2007.\n"
        "For implementation notes, see also this paper:\n\n"
        " * Silvan Sievers, Manuela Ortlieb and Malte Helmert.<<BR>>\n"
        " [Efficient Implementation of Pattern Database Heuristics for "
        "Classical Planning "
        "http://ai.cs.unibas.ch/papers/sievers-et-al-socs2012.pdf].<<BR>>\n"
        " In //Proceedings of the Fifth Annual Symposium on Combinatorial "
        "Search (SoCS 2012)//, "
        "pp. 105-111. AAAI Press 2012.\n");
    parser.document_note(
        "Note",
        "The pattern collection created by the algorithm will always contain "
        "all patterns consisting of a single goal variable, even if this "
        "violates the pdb_max_size or collection_max_size limits.");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");
    parser.document_note(
        "Note",
        "This pattern generation method uses the canonical pattern collection "
        "heuristic.");
    parser.document_note(
        "Implementation Notes",
        "The following will very briefly describe the algorithm and explain "
        "the differences between the original implementation from 2007 and the "
        "new one in Fast Downward.\n\n"
        "The aim of the algorithm is to output a pattern collection for which "
        "the Heuristic#Canonical_PDB yields the best heuristic estimates.\n\n"
        "The algorithm is basically a local search (hill climbing) which "
        "searches the \"pattern neighbourhood\" (starting initially with a "
        "pattern for each goal variable) for improving the pattern collection. "
        "This is done exactly as described in the section \"pattern "
        "construction as search\" in the paper. For evaluating the "
        "neighbourhood, the \"counting approximation\" as introduced in the "
        "paper was implemented. An important difference however consists in "
        "the fact that this implementation computes all pattern databases for "
        "each candidate pattern rather than using A* search to compute the "
        "heuristic values only for the sample states for each pattern.\n\n"
        "Also the logic for sampling the search space differs a bit from the "
        "original implementation. The original implementation uses a random "
        "walk of a length which is binomially distributed with the mean at the "
        "estimated solution depth (estimation is done with the current pattern "
        "collection heuristic). In the Fast Downward implementation, also a "
        "random walk is used, where the length is the estimation of the number "
        "of solution steps, which is calculated by dividing the current "
        "heuristic estimate for the initial state by the average operator "
        "costs of the planning task (calculated only once and not updated "
        "during sampling!) to take non-unit cost problems into account. This "
        "yields a random walk of an expected lenght of np = 2 * estimated "
        "number of solution steps. If the random walk gets stuck, it is being "
        "restarted from the initial state, exactly as described in the "
        "original paper.\n\n"
        "The section \"avoiding redundant evaluations\" describes how the "
        "search neighbourhood of patterns can be restricted to variables that "
        "are somewhat relevant to the variables already included in the "
        "pattern by analyzing causal graphs. This is also implemented in Fast "
        "Downward, but we only consider precondition-to-effect arcs of the "
        "causal graph, ignoring effect-to-effect arcs. The second approach "
        "described in the paper (statistical confidence interval) is not "
        "applicable to this implementation, as it doesn't use A* search but "
        "constructs the entire pattern databases for all candidate patterns "
        "anyway.\n"
        "The search is ended if there is no more improvement (or the "
        "improvement is smaller than the minimal improvement which can be set "
        "as an option), however there is no limit of iterations of the local "
        "search. This is similar to the techniques used in the original "
        "implementation as described in the paper.",
        true);

    parser.add_option<int>(
        "pdb_max_size",
        "maximal number of states per pattern database ",
        "2000000",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "collection_max_size",
        "maximal number of states in the pattern collection",
        "20000000",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "num_samples",
        "number of samples (random states) on which to evaluate each "
        "candidate pattern collection",
        "1000",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "min_improvement",
        "minimum number of samples on which a candidate pattern "
        "collection must improve on the current one to be considered "
        "as the next pattern collection ",
        "10",
        Bounds("1", "infinity"));
    parser.add_option<double>(
        "max_time",
        "maximum time in seconds for improving the initial pattern "
        "collection via hill climbing. If set to 0, no hill climbing "
        "is performed at all.",
        "infinity",
        Bounds("0.0", "infinity"));

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    if (opts.get<int>("min_improvement") > opts.get<int>("num_samples"))
        parser.error("minimum improvement must not be higher than number of "
                     "samples");

    if (parser.dry_run())
        return 0;

    PatternGenerationHaslum pgh(opts);
    return pgh.get_pattern_collection_heuristic();
}

static Plugin<Heuristic> _plugin("ipdb", _parse);
