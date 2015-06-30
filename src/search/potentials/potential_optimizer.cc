#include "potential_optimizer.h"

#include "potential_heuristic.h"

#include "../global_operator.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../sampling.h"
#include "../state_registry.h"
#include "../task_tools.h"

#include <cmath>
#include <numeric>

using namespace std;


namespace potentials {
enum class OptFunc {
    INITIAL_STATE,
    ALL_STATES,
    SAMPLES
};

PotentialOptimizer::PotentialOptimizer(const Options &opts)
    : task(get_task_from_options(opts)),
      task_proxy(*task),
      lp_solver(LPSolverType(opts.get_enum("lpsolver"))),
      max_potential(opts.get<double>("max_potential")),
      num_lp_vars(0) {
    verify_no_axioms(task_proxy);
    verify_no_conditional_effects(task_proxy);
    initialize();
    construct_lp();
}

void PotentialOptimizer::initialize() {
    VariablesProxy vars = task_proxy.get_variables();
    lp_var_ids.resize(vars.size());
    fact_potentials.resize(vars.size());
    for (VariableProxy var : vars) {
        // Add LP variable for "unknown" value.
        lp_var_ids[var.get_id()].resize(var.get_domain_size() + 1);
        for (int val = 0; val < var.get_domain_size() + 1; ++val) {
            lp_var_ids[var.get_id()][val] = num_lp_vars++;
        }
        fact_potentials[var.get_id()].resize(var.get_domain_size());
    }
}

bool PotentialOptimizer::has_optimal_solution() const {
    return lp_solver.has_optimal_solution();
}

bool PotentialOptimizer::optimize_for_state(const GlobalState &state) {
    return optimize_for_samples({state}
                                );
}

int PotentialOptimizer::get_lp_var_id(const FactProxy &fact) const {
    return lp_var_ids[fact.get_variable().get_id()][fact.get_value()];
}

bool PotentialOptimizer::optimize_for_all_states() {
    vector<double> coefficients(num_lp_vars, 0.0);
    for (FactProxy fact : task_proxy.get_variables().get_facts()) {
        coefficients[get_lp_var_id(fact)] = 1.0 / fact.get_variable().get_domain_size();
    }
    set_lp_objective(coefficients);
    bool optimal = solve_lp();
    if (!optimal) {
        if (potentials_are_bounded())
            ABORT("LP unbounded even though potentials are bounded.");
        else
            ABORT("Potentials must be bounded for all-states LP.");
    }
    return optimal;
}

bool PotentialOptimizer::optimize_for_samples(const vector<GlobalState> &samples) {
    vector<double> coefficients(num_lp_vars, 0.0);
    for (const GlobalState &global_state : samples) {
        State state = task_proxy.convert_global_state(global_state);
        for (FactProxy fact : state) {
            coefficients[get_lp_var_id(fact)] += 1.0;
        }
    }
    set_lp_objective(coefficients);
    return solve_lp();
}

void PotentialOptimizer::set_lp_objective(const vector<double> &coefficients) {
    assert(static_cast<int>(coefficients.size()) == num_lp_vars);
    for (int i = 0; i < num_lp_vars; ++i) {
        lp_solver.set_objective_coefficient(i, coefficients[i]);
    }
}

bool PotentialOptimizer::potentials_are_bounded() const {
    return max_potential != numeric_limits<double>::infinity();
}

void PotentialOptimizer::construct_lp() {
    double upper_bound = (potentials_are_bounded() ? max_potential :
                          lp_solver.get_infinity());

    vector<LPVariable> lp_variables;
    for (int lp_var_id = 0; lp_var_id < num_lp_vars; ++lp_var_id) {
        // Use dummy coefficient for now. Adapt coefficient later.
        lp_variables.emplace_back(-lp_solver.get_infinity(), upper_bound, 1.0);
    }

    vector<LPConstraint> lp_constraints;
    for (OperatorProxy op : task_proxy.get_operators()) {
        // Create constraint:
        // Sum_{V in vars(eff(o))} (P_{V=pre(o)[V]} - P_{V=eff(o)[V]}) <= cost(o)
        unordered_map<int, int> var_to_precondition;
        for (FactProxy pre : op.get_preconditions()) {
            var_to_precondition[pre.get_variable().get_id()] = pre.get_value();
        }
        LPConstraint constraint(-lp_solver.get_infinity(), op.get_cost());
        vector<pair<int, int> > coefficients;
        for (EffectProxy effect : op.get_effects()) {
            int var_id = effect.get_fact().get_variable().get_id();
            int pre = -1;
            auto it = var_to_precondition.find(var_id);
            if (it == var_to_precondition.end()) {
                int undef_val = effect.get_fact().get_variable().get_domain_size();
                pre = undef_val;
            } else {
                pre = it->second;
            }
            int post = effect.get_fact().get_value();
            int pre_lp = lp_var_ids[var_id][pre];
            int post_lp = lp_var_ids[var_id][post];
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
    vector<int> goal(task_proxy.get_variables().size(), -1);
    for (FactProxy fact : task_proxy.get_goals()) {
        goal[fact.get_variable().get_id()] = fact.get_value();
    }
    for (VariableProxy var : task_proxy.get_variables()) {
        int undef_val = var.get_domain_size();
        if (goal[var.get_id()] == -1)
            goal[var.get_id()] = undef_val;
    }

    for (VariableProxy var : task_proxy.get_variables()) {
        // Create constraint (using variable bounds): P_{V=goal[V]} = 0
        // TODO: Do not set lower bound?
        int var_id = var.get_id();
        LPVariable &lp_var = lp_variables[lp_var_ids[var_id][goal[var_id]]];
        lp_var.lower_bound = 0;
        lp_var.upper_bound = 0;

        int undef_val = var.get_domain_size();
        int undef_val_lp = lp_var_ids[var_id][undef_val];
        for (int val = 0; val < var.get_domain_size(); ++val) {
            int val_lp = lp_var_ids[var_id][val];
            // Create constraint: P_{V=v} <= P_{V=u}
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
    for (FactProxy fact : task_proxy.get_variables().get_facts()) {
        fact_potentials[fact.get_variable().get_id()][fact.get_value()] =
            solution[get_lp_var_id(fact)];
    }
}

shared_ptr<Heuristic> PotentialOptimizer::get_heuristic() const {
    assert(has_optimal_solution());
    Options opts;
    opts.set<int>("cost_type", 0);
    return make_shared<PotentialHeuristic>(opts, fact_potentials);
}


void filter_dead_ends(PotentialOptimizer &optimizer, vector<GlobalState> &samples) {
    assert(!optimizer.potentials_are_bounded());
    vector<GlobalState> non_dead_end_samples;
    for (const GlobalState &sample : samples) {
        if (optimizer.optimize_for_state(sample))
            non_dead_end_samples.push_back(sample);
    }
    swap(samples, non_dead_end_samples);
}

void optimize_for_samples(PotentialOptimizer &optimizer, int num_samples) {
    StateRegistry sample_registry;
    vector<GlobalState> samples;
    optimizer.optimize_for_state(g_initial_state());
    if (optimizer.has_optimal_solution()) {
        shared_ptr<Heuristic> sampling_heuristic = optimizer.get_heuristic();
        samples = sample_states_with_random_walks(
            sample_registry, num_samples, *sampling_heuristic);
    }
    if (!optimizer.potentials_are_bounded()) {
        filter_dead_ends(optimizer, samples);
    }
    optimizer.optimize_for_samples(samples);
}

shared_ptr<Heuristic> create_potential_heuristic(const Options &opts) {
    PotentialOptimizer optimizer(opts);
    OptFunc opt_func(OptFunc(opts.get_enum("opt_func")));
    if (opt_func == OptFunc::INITIAL_STATE) {
        optimizer.optimize_for_state(g_initial_state());
    } else if (opt_func == OptFunc::ALL_STATES) {
        optimizer.optimize_for_all_states();
    } else if (opt_func == OptFunc::SAMPLES) {
        optimize_for_samples(optimizer, opts.get<int>("num_samples"));
    } else {
        ABORT("Unkown optimization function");
    }
    return optimizer.get_heuristic();
}

void add_common_potentials_options_to_parser(OptionParser &parser) {
    parser.add_option<int>(
        "num_samples",
        "Number of states to sample",
        "1000");
    parser.add_option<double>(
        "max_potential",
        "Bound potentials by this number",
        "infinity");
}

static Heuristic *_parse(OptionParser &parser) {
    vector<string> opt_funcs;
    vector<string> opt_funcs_doc;
    opt_funcs.push_back("INITIAL_STATE");
    opt_funcs_doc.push_back(
        "optimize heuristic for initial state");
    opt_funcs.push_back("ALL_STATES");
    opt_funcs_doc.push_back(
        "optimize heuristic for all states");
    opt_funcs.push_back("SAMPLES");
    opt_funcs_doc.push_back(
        "optimize heuristic for a set of sample states");
    parser.add_enum_option(
        "opt_func",
        opt_funcs,
        "Optimization function",
        "SAMPLES",
        opt_funcs_doc);
    add_lp_solver_option_to_parser(parser);
    add_common_potentials_options_to_parser(parser);
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    // TODO: Fix memory leak once issue526 is merged.
    shared_ptr<Heuristic> *heuristic = new shared_ptr<Heuristic>(
        create_potential_heuristic(opts));
    return heuristic->get();
}

static Plugin<Heuristic> _plugin("potentials", _parse);
}
