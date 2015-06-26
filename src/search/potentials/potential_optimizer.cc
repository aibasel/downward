#include "potential_optimizer.h"

#include "potential_heuristic.h"

#include "../countdown_timer.h"
#include "../global_operator.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../rng.h"
#include "../sampling.h"
#include "../state_registry.h"
#include "../successor_generator.h"
#include "../timer.h"

#include <cmath>
#include <numeric>
#include <unordered_map>

using namespace std;


namespace potentials {

PotentialOptimizer::PotentialOptimizer(const Options &options)
    : lp_solver(LPSolverType(options.get_enum("lpsolver"))),
      optimization_function(OptimizationFunction(options.get_enum("optimization_function"))),
      num_samples(options.get<int>("num_samples")),
      max_potential(options.get<double>("max_potential")),
      max_sampling_time(options.get<double>("max_sampling_time")),
      max_filtering_time(options.get<double>("max_filtering_time")),
      debug(options.get<bool>("debug")),
      num_cols(-1) {
    verify_no_axioms_no_conditional_effects();
    Timer initialization_timer;
    fact_potentials.resize(g_variable_domain.size());
    int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        fact_potentials[var].resize(g_variable_domain[var]);
    }
    construct_lp();
    if (optimization_function == INITIAL_STATE) {
        optimize_for_state(g_initial_state());
    } else if (optimization_function == ALL_STATES) {
        optimize_for_all_states();
    } else if (optimization_function == SAMPLES) {
        StateRegistry sample_registry;
        vector<GlobalState> samples;
        optimize_for_state(g_initial_state());
        if (has_optimal_solution()) {
            Options opts;
            opts.set<int>("cost_type", 0);
            shared_ptr<Heuristic> sampling_heuristic = get_heuristic();
            samples = sample_states_with_random_walks(
                sample_registry, num_samples, *sampling_heuristic);
        }
        if (max_potential == numeric_limits<double>::infinity()) {
            filter_dead_ends(samples);
        }
        optimize_for_samples(samples);
    }
    cout << "Initialization of PotentialOptimizer: " << initialization_timer << endl;
}

bool PotentialOptimizer::has_optimal_solution() const {
    return lp_solver.has_optimal_solution();
}

bool PotentialOptimizer::optimize_for_state(const GlobalState &state) {
    return optimize_for_samples({state});
}

bool PotentialOptimizer::optimize_for_all_states() {
    int num_vars = g_variable_domain.size();
    vector<double> coefficients(num_cols, 0.0);
    for (int var = 0; var < num_vars; ++var) {
        for (int value = 0; value < g_variable_domain[var]; ++value) {
            coefficients[lp_var_ids[var][value]] = 1.0 / g_variable_domain[var];
        }
    }
    set_objective(coefficients);
    bool feasible = solve_lp();
    if (!feasible)
        ABORT("Potentials must be bounded in the LP for all states.");
    return feasible;
}

bool PotentialOptimizer::optimize_for_samples(const vector<GlobalState> &samples) {
    vector<double> coefficients(num_cols, 0.0);
    int num_vars = g_variable_domain.size();
    for (const GlobalState &state : samples) {
        for (int var = 0; var < num_vars; ++var) {
            int value = state[var];
            coefficients[lp_var_ids[var][value]] += 1.0;
        }
    }
    set_objective(coefficients);
    return solve_lp();
}

void PotentialOptimizer::filter_dead_ends(vector<GlobalState> &samples) {
    CountdownTimer filtering_timer(max_filtering_time);
    vector<GlobalState> non_dead_end_samples;
    for (const GlobalState &sample : samples) {
        if (filtering_timer.is_expired()) {
            cout << "Ran out of time filtering dead ends." << endl;
            break;
        }
        if (optimize_for_state(sample))
            non_dead_end_samples.push_back(sample);
    }
    cout << "Time for filtering dead ends: " << filtering_timer << endl;
    cout << "Non dead-end samples: " << non_dead_end_samples.size() << endl;
    swap(samples, non_dead_end_samples);
}

void PotentialOptimizer::set_objective(const vector<double> &coefficients) {
    assert(static_cast<int>(coefficients.size()) == num_cols);
    for (int i = 0; i < num_cols; ++i) {
        lp_solver.set_objective_coefficient(i, coefficients[i]);
    }
}

void PotentialOptimizer::construct_lp() {
    double upper_bound = (max_potential == numeric_limits<double>::infinity() ?
                          lp_solver.get_infinity() : max_potential);

    vector<LPVariable> lp_variables;
    int num_lp_vars = 0;
    vector<vector<int> > lp_vars;
    lp_var_ids.resize(g_variable_domain.size());
    const int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        lp_var_ids[var].resize(g_variable_domain[var] + 1);
        for (int val = 0; val < g_variable_domain[var] + 1; ++val) {
            // Use dummy coefficient for now. Set the real coefficients later.
            lp_variables.emplace_back(-lp_solver.get_infinity(), upper_bound, 1.0);
            lp_var_ids[var][val] = num_lp_vars++;
        }
    }
    num_cols = num_lp_vars;

    vector<LPConstraint> lp_constraints;
    lp_constraints.reserve(g_operators.size());
    for (size_t i = 0; i < g_operators.size(); ++i) {
        // Create Constraint:
        // Sum_{V in Vars(eff(o))} (P_{V=pre(o)[V]} - P_{V=eff(o)[V]}) <= cost(o)
        const GlobalOperator &op = g_operators[i];
        const vector<GlobalCondition> &preconditions = op.get_preconditions();
        unordered_map<int, int> var_to_precondition;
        for (size_t j = 0; j < preconditions.size(); ++j) {
            var_to_precondition[preconditions[j].var] = preconditions[j].val;
        }
        const vector<GlobalEffect> &effects = op.get_effects();
        LPConstraint constraint(-lp_solver.get_infinity(), op.get_cost());
        vector<pair<int, int> > coefficients;
        for (size_t j = 0; j < effects.size(); ++j) {
            int var = effects[j].var;
            int pre = -1;
            auto it = var_to_precondition.find(var);
            if (it == var_to_precondition.end()) {
                pre = g_variable_domain[var];
            } else {
                pre = it->second;
            }
            int post = effects[j].val;
            int pre_lp = lp_var_ids[var][pre];
            int post_lp = lp_var_ids[var][post];
            assert(pre_lp != post_lp);
            coefficients.emplace_back(pre_lp, 1);
            coefficients.emplace_back(post_lp, -1);
        }
        sort(coefficients.begin(), coefficients.end());
        for (const auto &coeff : coefficients)
            constraint.insert(coeff.first, coeff.second);
        lp_constraints.push_back(constraint);
    }

    /* Create full goal state. Use dummy value |dom(V)| for variables V
       undefined in the goal. */
    vector<int> goal(g_variable_domain.size(), -1);
    for (size_t i = 0; i < g_goal.size(); ++i) {
        goal[g_goal[i].first] = g_goal[i].second;
    }
    for (int var = 0; var < num_vars; ++var) {
        int undef_val = g_variable_domain[var];
        if (goal[var] == -1)
            goal[var] = undef_val;
    }

    for (int var = 0; var < num_vars; ++var) {
        // Create Constraint (we use variable bounds to express this constraint):
        // P_{V=goal[V]} = 0
        // TODO: Do not set lower bound?
        LPVariable &lp_var = lp_variables[lp_var_ids[var][goal[var]]];
        lp_var.lower_bound = 0;
        lp_var.upper_bound = 0;

        int undef_val = g_variable_domain[var];
        int undef_val_lp = lp_var_ids[var][undef_val];
        for (int val = 0; val < g_variable_domain[var]; ++val) {
            int val_lp = lp_var_ids[var][val];
            // Create Constraint:
            // P_{V=v} <= P_{V=u}
            // Note that we could eliminate variables P_{V=u} if V is undefined in the goal.
            LPConstraint constraint(-lp_solver.get_infinity(), 0);
            constraint.insert(val_lp, 1);
            constraint.insert(undef_val_lp, -1);
            lp_constraints.push_back(constraint);
        }
    }
    lp_solver.load_problem(LPObjectiveSense::MAXIMIZE, lp_variables, lp_constraints);
}

bool PotentialOptimizer::solve_lp() {
    lp_solver.solve();
    if (lp_solver.has_optimal_solution()) {
        extract_lp_solution();
    }
    return lp_solver.has_optimal_solution();
}

void PotentialOptimizer::extract_lp_solution() {
    assert(lp_solver.has_optimal_solution());
    const vector<double> solution = lp_solver.extract_solution();
    int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        for (int val = 0; val < g_variable_domain[var]; ++val) {
            fact_potentials[var][val] = solution[lp_var_ids[var][val]];
        }
    }
}

shared_ptr<Heuristic> PotentialOptimizer::get_heuristic() const {
    assert(has_optimal_solution());
    Options opts;
    opts.set<int>("cost_type", 0);
    return make_shared<PotentialHeuristic>(opts, fact_potentials);
}

void add_common_potential_options_to_parser(OptionParser &parser) {
    vector<string> optimization_function;
    vector<string> optimization_function_doc;
    optimization_function.push_back("INITIAL_STATE");
    optimization_function_doc.push_back(
        "optimize heuristic for initial state");
    optimization_function.push_back("ALL_STATES");
    optimization_function_doc.push_back(
        "optimize heuristic for all states");
    optimization_function.push_back("SAMPLES");
    optimization_function_doc.push_back(
        "optimize heuristic for a set of sample states");
    parser.add_enum_option("optimization_function",
                           optimization_function,
                           "Optimization function.",
                           "SAMPLES",
                           optimization_function_doc);
    parser.add_option<int>(
        "num_samples",
        "Number of states to sample if optimization_function=samples",
        "1000");
    parser.add_option<double>(
        "max_potential",
        "Bound potentials by this number",
        "infinity");
    parser.add_option<double>(
        "max_sampling_time",
        "time limit in seconds for sampling states",
        "infinity");
    parser.add_option<double>(
        "max_filtering_time",
        "time limit in seconds for filtering dead ends",
        "infinity");
    parser.add_option<bool>(
        "debug",
        "Print debug messages",
        "false");
}

static Heuristic *_parse(OptionParser &parser) {
    add_lp_solver_option_to_parser(parser);
    add_common_potential_options_to_parser(parser);
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    PotentialOptimizer optimizer(opts);
    // TODO: Fix.
    shared_ptr<Heuristic> *heuristic = new shared_ptr<Heuristic>(optimizer.get_heuristic());
    return heuristic->get();
}

static Plugin<Heuristic> _plugin("potential", _parse);
}
