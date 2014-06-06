#include "pattern_generation_haslum.h"

#include "canonical_pdbs_heuristic.h"
#include "pdb_heuristic.h"

#include "../globals.h"
#include "../legacy_causal_graph.h"
#include "../operator.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../rng.h"
#include "../state.h"
#include "../state_registry.h"
#include "../successor_generator.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

struct HillClimbingTimeout : public exception {};

PatternGenerationHaslum::PatternGenerationHaslum(const Options &opts)
    : pdb_max_size(opts.get<int>("pdb_max_size")),
      collection_max_size(opts.get<int>("collection_max_size")),
      num_samples(opts.get<int>("num_samples")),
      min_improvement(opts.get<int>("min_improvement")),
      max_time(opts.get<int>("max_time")),
      cost_type(OperatorCost(opts.get<int>("cost_type"))),
      num_rejected(0),
      hill_climbing_timer(0) {
    Timer timer;
    initialize();
    cout << "Pattern generation (Haslum et al.) time: " << timer << endl;
}

PatternGenerationHaslum::~PatternGenerationHaslum() {
}

void PatternGenerationHaslum::generate_candidate_patterns(const PDBHeuristic *pdb,
                                                          vector<vector<int> > &candidate_patterns) {
    const vector<int> &pattern = pdb->get_pattern();
    int pdb_size = pdb->get_size();
    for (size_t i = 0; i < pattern.size(); ++i) {
        // causally relevant variables for current variable from pattern
        vector<int> rel_vars = g_legacy_causal_graph->get_predecessors(pattern[i]);
        sort(rel_vars.begin(), rel_vars.end());
        vector<int> relevant_vars;
        // make sure we only use relevant variables which are not already included in pattern
        set_difference(rel_vars.begin(), rel_vars.end(), pattern.begin(), pattern.end(), back_inserter(relevant_vars));
        for (size_t j = 0; j < relevant_vars.size(); ++j) {
            // test against overflow and pdb_max_size
            if (pdb_size <= pdb_max_size / g_variable_domain[relevant_vars[j]]) {
                // pdb_size * g_variable_domain[relevant_vars[j]] <= pdb_max_size
                vector<int> new_pattern(pattern);
                new_pattern.push_back(relevant_vars[j]);
                sort(new_pattern.begin(), new_pattern.end());
                candidate_patterns.push_back(new_pattern);
            } else {
                // [commented out the message because it might be too verbose]
                // cout << "ignoring new pattern as candidate because it is too large" << endl;
                num_rejected += 1;
            }
        }
    }
}

size_t PatternGenerationHaslum::generate_pdbs_for_candidates(set<vector<int> > &generated_patterns,
                                                             vector<vector<int> > &new_candidates,
                                                             vector<PDBHeuristic *> &candidate_pdbs) const {
    // For the new candidate patterns check whether they already have been candidates before and
    // thus already a PDB has been created an inserted into candidate_pdbs.
    size_t max_pdb_size = 0;
    for (size_t i = 0; i < new_candidates.size(); ++i) {
        if (generated_patterns.count(new_candidates[i]) == 0) {
            Options opts;
            opts.set<int>("cost_type", cost_type);
            opts.set<vector<int> >("pattern", new_candidates[i]);
            candidate_pdbs.push_back(new PDBHeuristic(opts, false));
            max_pdb_size = max(max_pdb_size,
                               candidate_pdbs.back()->get_size());
            generated_patterns.insert(new_candidates[i]);
        }
    }
    return max_pdb_size;
}

void PatternGenerationHaslum::sample_states(StateRegistry &sample_registry,
                                            vector<State> &samples,
                                            double average_operator_cost) {
    const State &initial_state = sample_registry.get_initial_state();
    current_heuristic->evaluate(initial_state);
    assert(!current_heuristic->is_dead_end());

    int h = current_heuristic->get_heuristic();
    int n;
    if (h == 0) {
        n = 10;
    } else {
        // Convert heuristic value into an approximate number of actions
        // (does nothing on unit-cost problems).
        // average_operator_cost cannot equal 0, as in this case, all operators
        // must have costs of 0 and in this case the if-clause triggers.
        int solution_steps_estimate = int((h / average_operator_cost) + 0.5);
        n = 4 * solution_steps_estimate;
    }
    double p = 0.5;
    // The expected walk length is np = 2 * estimated number of solution steps.
    // (We multiply by 2 because the heuristic is underestimating.)

    samples.reserve(num_samples);
    for (int i = 0; i < num_samples; ++i) {
        if ((*hill_climbing_timer)() > max_time)
            throw HillClimbingTimeout();

        // calculate length of random walk accoring to a binomial distribution
        int length = 0;
        for (int j = 0; j < n; ++j) {
            double random = g_rng(); // [0..1)
            if (random < p)
                ++length;
        }

        // random walk of length length
        State current_state(initial_state);
        for (int j = 0; j < length; ++j) {
            vector<const Operator *> applicable_ops;
            g_successor_generator->generate_applicable_ops(current_state, applicable_ops);
            // if there are no applicable operators --> do not walk further
            if (applicable_ops.empty()) {
                break;
            } else {
                int random = g_rng.next(applicable_ops.size()); // [0..applicable_os.size())
                assert(applicable_ops[random]->is_applicable(current_state));
                current_state = sample_registry.get_successor_state(current_state, *applicable_ops[random]);
                // if current state is a dead end, then restart with initial state
                current_heuristic->evaluate_dead_end(current_state);
                if (current_heuristic->is_dead_end())
                    current_state = initial_state;
            }
        }
        // last state of the random walk is used as sample
        samples.push_back(current_state);
    }
}

std::pair<int, int> PatternGenerationHaslum::find_best_improving_pdb(
    vector<State> &samples,
    vector<PDBHeuristic *> &candidate_pdbs) {
    // TODO: The original implementation by Haslum et al. uses astar to compute h values for
    // the sample states only instead of generating all PDBs.
    // improvement: best improvement (= hightest count) for a pattern so far.
    // We require that a pattern must have an improvement of at least one in
    // order to be taken into account.
    int improvement = 0;
    int best_pdb_index = -1;

    // Iterate over all candidates and search for the best improving pattern/pdb
    for (size_t i = 0; i < candidate_pdbs.size(); ++i) {
        if ((*hill_climbing_timer)() > max_time)
            throw HillClimbingTimeout();

        PDBHeuristic *pdb_heuristic = candidate_pdbs[i];
        if (pdb_heuristic == 0) {
            // candidate pattern is too large or has already been added to
            // the canonical heuristic.
            continue;
        }
        // If a candidate's size added to the current collection's size exceeds the maximum
        // collection size, then delete the PDB and let the PDB's entry point to a null reference
        if (current_heuristic->get_size() + pdb_heuristic->get_size() > collection_max_size) {
            delete pdb_heuristic;
            candidate_pdbs[i] = 0;
            continue;
        }

        // Calculate the "counting approximation" for all sample states: count the number of
        // samples for which the current pattern collection heuristic would be improved
        // if the new pattern was included into it.
        // TODO: The original implementation by Haslum et al. uses m/t as a statistical
        // confidence intervall to stop the astar-search (which they use, see above) earlier.
        int count = 0;
        vector<vector<PDBHeuristic *> > max_additive_subsets;
        current_heuristic->get_max_additive_subsets(pdb_heuristic->get_pattern(), max_additive_subsets);
        for (size_t j = 0; j < samples.size(); ++j) {
            if (is_heuristic_improved(pdb_heuristic, samples[j], max_additive_subsets))
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

bool PatternGenerationHaslum::is_heuristic_improved(PDBHeuristic *pdb_heuristic,
                                                    const State &sample,
                                                    const vector<vector<PDBHeuristic *> > &max_additive_subsets) {
    pdb_heuristic->evaluate(sample);
    if (pdb_heuristic->is_dead_end()) {
        return true;
    }
    int h_pattern = pdb_heuristic->get_heuristic(); // h-value of the new pattern

    current_heuristic->evaluate(sample);
    int h_collection = current_heuristic->get_heuristic(); // h-value of the current collection heuristic
    for (size_t k = 0; k < max_additive_subsets.size(); ++k) { // for each max additive subset...
        int h_subset = 0;
        for (size_t l = 0; l < max_additive_subsets[k].size(); ++l) { // ...calculate its h-value
            max_additive_subsets[k][l]->evaluate(sample);
            assert(!max_additive_subsets[k][l]->is_dead_end());
            h_subset += max_additive_subsets[k][l]->get_heuristic();
        }
        if (h_pattern + h_subset > h_collection) {
            // return true if one max additive subest is found for which the condition is met
            return true;
        }
    }
    return false;
}

void PatternGenerationHaslum::hill_climbing(double average_operator_cost,
                                            vector<vector<int> > &initial_candidate_patterns) {
    hill_climbing_timer = new Timer();
    // stores all candidate patterns generated so far in order to avoid duplicates
    set<vector<int> > generated_patterns;
    // new_candidates is the set of new pattern candidates from the last call to generate_candidate_patterns
    vector<vector<int> > &new_candidates = initial_candidate_patterns;
    // all candidate patterns are converted into pdbs once and stored
    vector<PDBHeuristic *> candidate_pdbs;
    int num_iterations = 0;
    size_t max_pdb_size = 0;
    try {
        while (true) {
            num_iterations += 1;
            cout << "current collection size is " << current_heuristic->get_size() << endl;
            // TODO think about how to handle this when state_registries are moved into the search algorithms.
            current_heuristic->evaluate(g_initial_state());
            cout << "current initial h value: ";
            if (current_heuristic->is_dead_end()) {
                cout << "infinite => stopping hill climbing" << endl;
                break;
            } else {
                cout << current_heuristic->get_heuristic() << endl;
            }

            size_t new_max_pdb_size = generate_pdbs_for_candidates(generated_patterns,
                                                                   new_candidates,
                                                                   candidate_pdbs);
            max_pdb_size = max(max_pdb_size, new_max_pdb_size);

            StateRegistry sample_registry;
            vector<State> samples;
            sample_states(sample_registry, samples, average_operator_cost);

            pair<int, int> improvement_and_index =
                find_best_improving_pdb(samples, candidate_pdbs);
            int improvement = improvement_and_index.first;
            int best_pdb_index = improvement_and_index.second;

            if (improvement < min_improvement) {
                cout << "Improvement below threshold. Stop hill climbing." << endl;
                break;
            }

            // add the best pattern to the CanonicalPDBsHeuristic
            assert(best_pdb_index != -1);
            const PDBHeuristic *best_pdb = candidate_pdbs[best_pdb_index];
            const vector<int> &best_pattern = best_pdb->get_pattern();
            cout << "found a better pattern with improvement " << improvement << endl;
            cout << "pattern: " << best_pattern << endl;
            current_heuristic->add_pattern(best_pattern);

            // clear current new_candidates and get successors for next iteration
            new_candidates.clear();
            generate_candidate_patterns(best_pdb, new_candidates);

            // remove from candidate_pdbs the added PDB
            delete candidate_pdbs[best_pdb_index];
            candidate_pdbs[best_pdb_index] = 0;

            cout << "Hill climbing time so far: " << *hill_climbing_timer << endl;
        }
    } catch (HillClimbingTimeout &e) {
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
    hill_climbing_timer = 0;
}

void PatternGenerationHaslum::initialize() {
    // calculate average operator costs
    double average_operator_cost = 0;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        average_operator_cost += get_adjusted_action_cost(g_operators[i], cost_type);
    }
    average_operator_cost /= g_operators.size();
    cout << "Average operator cost: " << average_operator_cost << endl;

    // initial collection: a pdb for each goal variable
    vector<vector<int> > initial_pattern_collection;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        initial_pattern_collection.push_back(vector<int>(1, g_goal[i].first));
    }
    Options opts;
    opts.set<int>("cost_type", cost_type);
    opts.set<vector<vector<int> > >("patterns", initial_pattern_collection);
    current_heuristic = new CanonicalPDBsHeuristic(opts);
    current_heuristic->evaluate(g_initial_state());
    if (current_heuristic->is_dead_end())
        return;

    // initial candidate patterns, computed separately for each pattern from the initial collection
    vector<vector<int> > initial_candidate_patterns;
    for (size_t i = 0; i < current_heuristic->get_pattern_databases().size(); ++i) {
        const PDBHeuristic *current_pdb = current_heuristic->get_pattern_databases()[i];
        generate_candidate_patterns(current_pdb, initial_candidate_patterns);
    }
    // remove duplicates in the candidate list
    sort(initial_candidate_patterns.begin(), initial_candidate_patterns.end());
    initial_candidate_patterns.erase(unique(initial_candidate_patterns.begin(),
                                            initial_candidate_patterns.end()),
                                     initial_candidate_patterns.end());
    cout << "done calculating initial pattern collection and candidate patterns for the search" << endl;

    // call to this method modifies initial_candidate_patterns (contains the new_candidates
    // after each call to generate_candidate_patterns)
    if (max_time > 0)
        hill_climbing(average_operator_cost, initial_candidate_patterns);
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "iPDB",
        "This pattern generation method is an adaption of the algorithm described "
        "in the following paper:\n\n"
        " * Patrik Haslum, Adi Botea, Malte Helmert, Blai Bonet and Sven Koenig.<<BR>>\n"
        " [Domain-Independent Construction of Pattern Database Heuristics for Cost-Optimal "
        "Planning http://www.informatik.uni-freiburg.de/~ki/papers/haslum-etal-aaai07.pdf].<<BR>>\n"
        " In //Proceedings of the 22nd AAAI Conference on Artificial Intelligence (AAAI 2007)//, "
        "pp. 1007-1012. AAAI Press 2007.\n"
        "For implementation notes, see also this paper:\n\n"
        " * Silvan Sievers, Manuela Ortlieb and Malte Helmert.<<BR>>\n"
        " [Efficient Implementation of Pattern Database Heuristics for Classical Planning "
        "http://ai.cs.unibas.ch/papers/sievers-et-al-socs2012.pdf].<<BR>>\n"
        " In //Proceedings of the Fifth Annual Symposium on Combinatorial Search (SoCS 2012)//, "
        "pp. 105-111. AAAI Press 2012.\n");
    parser.document_note(
        "Note",
        "The pattern collection created by the algorithm will always contain "
        "all patterns consisting of a single goal variable, even if this violates "
        "the pdb_max_size or collection_max_size limits.");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");
    parser.document_note("Note",
                         "This pattern generation method uses the canonical pattern collection heuristic.");
    parser.document_note("Implementation Notes",
                         "The following will very briefly describe the algorithm and explain the differences "
                         "between the original implementation from 2007 and the new one in Fast Downward.\n\n"
                         "The aim of the algorithm is to output a pattern collection for which the "
                         "Heuristic#Canonical_PDB yields the best heuristic estimates.\n\n"
                         "The algorithm is basically a local search (hill climbing) which searches the "
                         "\"pattern neighbourhood\" (starting initially with a pattern for each goal variable) "
                         "for improving the pattern collection. This is done exactly as described in the "
                         "section \"pattern construction as search\" in the paper. For evaluating the "
                         "neighbourhood, the \"counting approximation\" as introduced in the paper was "
                         "implemented. An important difference however consists in the fact that this "
                         "implementation computes all pattern databases for each candidate pattern rather than "
                         "using A* search to compute the heuristic values only for the sample states for each "
                         "pattern.\n\nAlso the logic for sampling the search space differs a bit from the "
                         "original implementation. The original implementation uses a random walk of a length "
                         "which is binomially distributed with the mean at the estimated solution depth "
                         "(estimation is done with the current pattern collection heuristic). In the Fast "
                         "Downward implementation, also a random walk is used, where the length is the "
                         "estimation of the number of solution steps, which is calculated by dividing the "
                         "current heuristic estimate for the initial state by the average operator costs of "
                         "the planning task (calculated only once and not updated during sampling!) to take "
                         "non-unit cost problems into account. This yields a random walk of an expected lenght "
                         "of np = 2 * estimated number of solution steps. If the random walk gets stuck, it is "
                         "being restarted from the initial state, exactly as described in the original paper.\n\n"
                         "The section \"avoiding redundant evaluations\" describes how the search neighbourhood "
                         "of patterns can be restricted to variables that are somewhat relevant to the variables "
                         "already included in the pattern by analyzing causal graphs. This is also implemented "
                         "in Fast Downward. The second approach described in the paper (statistical confidence "
                         "interval) is not applicable to this implementation, as it doesn't use A* search but "
                         "constructs the entire pattern databases for all candidate patterns anyway.\n"
                         "The search is ended if there is no more improvement (or the improvement is smaller "
                         "than the minimal improvement which can be set as an option), how ever there is no "
                         "limit of iterations of the local search. This is similar to the techniques used in "
                         "the original implementation as described in the paper.", true);

    parser.add_option<int>("pdb_max_size",
                           "maximal number of states per pattern database ",
                           "2000000");
    parser.add_option<int>("collection_max_size",
                           "maximal number of states in the pattern collection",
                           "20000000");
    parser.add_option<int>("num_samples",
                           "number of samples (random states) on which to evaluate each candidate pattern collection",
                           "1000");
    parser.add_option<int>("min_improvement",
                           "minimum number of samples on which a candidate pattern collection must improve on the "
                           "current one to be considered as the next pattern collection ", "10");
    parser.add_option<int>("max_time",
                           "maximum time in seconds for improving the initial pattern "
                           "collection via hill climbing. If set to 0, no hill climbing is performed at all.",
                           "infinity");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    if (opts.get<int>("pdb_max_size") < 1)
        parser.error("size per pdb must be at least 1");
    if (opts.get<int>("collection_max_size") < 1)
        parser.error("total pdb collection size must be at least 1");
    if (opts.get<int>("min_improvement") < 1)
        parser.error("minimum improvement must be at least 1");
    if (opts.get<int>("min_improvement") > opts.get<int>("num_samples"))
        parser.error("minimum improvement must not be higher than number of samples");

    if (parser.dry_run())
        return 0;

    PatternGenerationHaslum pgh(opts);
    return pgh.get_pattern_collection_heuristic();
}

static Plugin<Heuristic> _plugin("ipdb", _parse);
