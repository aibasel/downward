#include "pdb_heuristic.h"
#include "globals.h"
//#include "option_parser.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"
#include "timer.h"
#include "abstract_state_iterator.h"
#include "variable_order_finder.h"

#include <limits>
#include <cstdlib>
#include <cassert>
#include <queue>
#include <vector>
#include <string>

using namespace std;

// AbstractOperator -------------------------------------------------------------------------------

AbstractOperator::AbstractOperator(const Operator &o, const vector<int> &var_to_index) {
    const vector<Prevail> &prevail = o.get_prevail();
    const vector<PrePost> &pre_post = o.get_pre_post();
    for (size_t j = 0; j < prevail.size(); ++j) {
        if (var_to_index[prevail[j].var] != -1) {
            conditions.push_back(make_pair(var_to_index[prevail[j].var], prevail[j].prev));
        }
    }
    for (size_t j = 0; j < pre_post.size(); ++j) {
        if (var_to_index[pre_post[j].var] != -1) {
            if (pre_post[j].pre != -1)
                conditions.push_back(make_pair(var_to_index[pre_post[j].var], pre_post[j].pre));
            effects.push_back(make_pair(var_to_index[pre_post[j].var], pre_post[j].post));
        }
    }
}

AbstractOperator::~AbstractOperator() {
}

void AbstractOperator::dump(const vector<int> &pattern) const {
    cout << "AbstractOperator:" << endl;
    cout << "Conditions:" << endl;
    for (size_t i = 0; i < conditions.size(); ++i) {
        cout << "Variable: " << conditions[i].first << " (True name: " 
        << g_variable_name[pattern[conditions[i].first]] << ") Value: " << conditions[i].second << endl;
    }
    cout << "Effects:" << endl;
    for (size_t i = 0; i < effects.size(); ++i) {
        cout << "Variable: " << effects[i].first << " (True name: " 
        << g_variable_name[pattern[effects[i].first]] << ") Value: " << effects[i].second << endl;
    }
}

// AbstractState ----------------------------------------------------------------------------------

AbstractState::AbstractState(const vector<int> &var_vals) : variable_values(var_vals) {
}

AbstractState::AbstractState(const State &state, const vector<int> &pattern) {
    for (size_t i = 0; i < pattern.size(); ++i) {
        variable_values.push_back(state[pattern[i]]);
    }
}

AbstractState::~AbstractState() {
}

bool AbstractState::is_applicable(const AbstractOperator &op) const {
    const vector<pair<int, int> > &conditions = op.get_conditions();
    for (size_t i = 0; i < conditions.size(); ++i) {
        if (variable_values[conditions[i].first] != conditions[i].second)
            return false;
    }
    return true;
}

void AbstractState::apply_operator(const AbstractOperator &op) {
    assert(is_applicable(op));
    const vector<pair<int, int> > &effects = op.get_effects();
    for (size_t i = 0; i < effects.size(); ++i) {
        int var = effects[i].first;
        int val = effects[i].second;
        variable_values[var] = val;
    }
}

bool AbstractState::is_goal_state(const vector<pair<int, int> > &abstract_goal) const {
    for (size_t i = 0; i < abstract_goal.size(); ++i) {
        if (variable_values[abstract_goal[i].first] != abstract_goal[i].second) {
            return false;
        }
    }
    return true;
}

void AbstractState::dump(const vector<int> &pattern) const {
    cout << "AbstractState: " << endl;
    for (size_t i = 0; i < variable_values.size(); ++i) {
        cout << "Variable: " << pattern[i] << " (True name: " 
        << g_variable_name[pattern[i]] << ") Value: " << variable_values[i] << endl;
    }
}

// PDBHeuristic ---------------------------------------------------------------

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run);
static ScalarEvaluatorPlugin pdb_heuristic_plugin("pdb", create);

/*PDBHeuristic::PDBHeuristic(int max_abstract_states) {
    verify_no_axioms_no_cond_effects();
    Timer timer;
    generate_pattern(max_abstract_states);
    cout << "PDB construction time: " << timer << endl;
}*/

PDBHeuristic::PDBHeuristic(const vector<int> &pattern) {
    verify_no_axioms_no_cond_effects();
    Timer timer;
    set_pattern(pattern);
    cout << "PDB construction time: " << timer << endl;
}

PDBHeuristic::~PDBHeuristic() {
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

void PDBHeuristic::set_pattern(const vector<int> &pat) {
    pattern = pat;
    n_i.reserve(pattern.size());
    variable_to_index.resize(g_variable_name.size(), -1);
    num_states = 1;
    // int p = 1; ; same as num_states; incrementally computed!
    for (size_t i = 0; i < pattern.size(); ++i) {
        n_i.push_back(num_states);
        variable_to_index[pattern[i]] = i;
        num_states *= g_variable_domain[pattern[i]];
        //p *= g_variable_domain[pattern[i]];
    }
    create_pdb();
}

/*void PDBHeuristic::generate_pattern(int max_abstract_states) {
#define DEBUG false
#if DEBUG
    cout << "dummy cout " << max_abstract_states << endl; //so that ... compiler is happy!
    // function tests
    // 1. blocks-7-2 test-pattern
    int patt[] = {9, 10, 11, 12, 13, 14};
    //int patt[] = {13, 14};
    
    // 2. driverlog-6 test-pattern
    //int patt[] = {4, 5, 7, 9, 10, 11, 12};
    
    // 3. logistics00-6-2 test-pattern
    //int patt[] = {3, 4, 5, 6, 7, 8};
    
    // 4. blocks-9-0 test-pattern
    //int patt[] = {0};
    
    // 5. logistics00-5-1 test-pattern
    //int patt[] = {0, 1, 2, 3, 4, 5, 6, 7};
    
    pattern = vector<int>(patt, patt + sizeof(patt) / sizeof(int));
    // TODO: without the = vector<int>, it does not work!
    n_i.reserve(pattern.size());
    variable_to_index.resize(g_variable_name.size(), -1);
    num_states = 1;
    // int p = 1; ; same as num_states; incrementally computed!
    for (size_t i = 0; i < pattern.size(); ++i) {
        n_i.push_back(num_states);
        variable_to_index[pattern[i]] = i;
        num_states *= g_variable_domain[pattern[i]];
        //p *= g_variable_domain[pattern[i]];
    }
    create_pdb();
#else
    VariableOrderFinder vof(MERGE_LINEAR_GOAL_CG_LEVEL);
    variable_to_index.resize(g_variable_name.size(), -1);
    int var = vof.next();
    num_states = g_variable_domain[var];
    size_t index = 0;
    int p = 1;
    while (num_states <= max_abstract_states) {
        //cout << "Number of abstract states = " << num_states << endl;
        //cout << "Including variable: " << var << " (True name:" << g_variable_name[var] << ")" << endl;
        pattern.push_back(var);
        variable_to_index[var] = index;
        n_i.push_back(p);
        p *= g_variable_domain[var];
        
        var = vof.next();
        ++index;
        num_states *= g_variable_domain[var];
    }
    num_states /= g_variable_domain[var];
    create_pdb();
#endif
}*/

void PDBHeuristic::create_pdb() {
    //assert(!pattern.empty());
    //assert(!n_i.empty());
    //assert(num_states != 0);
    
    vector<AbstractOperator> operators;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        AbstractOperator ao(g_operators[i], variable_to_index);
        if (!ao.get_effects().empty())
            operators.push_back(ao);
    }
    
    vector<pair<int, int> > abstracted_goal;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        if (variable_to_index[g_goal[i].first] != -1) {
            abstracted_goal.push_back(make_pair(variable_to_index[g_goal[i].first], g_goal[i].second));
        }
    }
    
    vector<vector<Edge> > back_edges;
    back_edges.resize(num_states);
    distances.reserve(num_states);
    // first entry: priority, second entry: index for an abstract state
    priority_queue<pair<int, size_t>, vector<pair<int, size_t> >, greater<pair<int, size_t> > > pq;
    
    vector<int> ranges;
    for (size_t i = 0; i < pattern.size(); ++i) {
        ranges.push_back(g_variable_domain[pattern[i]]);
    }
    for (AbstractStateIterator it(ranges); !it.is_at_end(); it.next()) {
        AbstractState abstract_state(it.get_current());
        int counter = it.get_counter();
        assert(hash_index(abstract_state) == counter);
        
        if (abstract_state.is_goal_state(abstracted_goal)) {
            pq.push(make_pair(0, counter));
            distances.push_back(0);
        }
        else {
            distances.push_back(numeric_limits<int>::max());
        }
        //abstract_state.dump(pattern);
        //cout << "successor states:" << endl;
        for (size_t j = 0; j < operators.size(); ++j) {
            if (abstract_state.is_applicable(operators[j])) {
                //cout << "applying operator:" << endl;
                //operators[j].dump(pattern);
                AbstractState next_state = abstract_state;
                next_state.apply_operator(operators[j]);
                //next_state.dump(pattern);
                size_t state_index = hash_index(next_state);
                if (state_index == counter) {
                    //cout << "operator didn't lead to a new state, ignoring!" << endl;
                    continue;
                } else {
                    assert(counter != state_index);
                    back_edges[state_index].push_back(Edge(g_operators[j].get_cost(), counter));
                }
            }
        }
        //cout << endl;
    }
    
    while (!pq.empty()) {
        pair<int, int> node = pq.top();
        pq.pop();
        int distance = node.first;
        size_t state_index = node.second;
        if (distance > distances[state_index])
            continue;
        const vector<Edge> &edges = back_edges[state_index];
        for (size_t i = 0; i < edges.size(); ++i) {
            size_t predecessor = edges[i].target;
            int cost = edges[i].cost;
            int alternative_cost = distances[state_index] + cost;
            if (alternative_cost < distances[predecessor]) {
                distances[predecessor] = alternative_cost;
                pq.push(make_pair(alternative_cost, predecessor));
            }
        }
    }
}

size_t PDBHeuristic::hash_index(const AbstractState &abstract_state) const {
    size_t index = 0;
    for (int i = 0; i < pattern.size(); ++i) {
        index += n_i[i] * abstract_state[variable_to_index[pattern[i]]];
    }
    return index;
}

/*AbstractState PDBAbstraction::inv_hash_index(int index) const {
vector<int> var_vals;
var_vals.resize(size);
for (int n = 1; n < pattern.size(); ++n) {
    int d = index % n_i[n];
    var_vals[variable_to_index[pattern[n - 1]]] = d / n_i[n - 1];
    index -= d;
    }
    var_vals[variable_to_index[pattern[size - 1]]] = index / n_i[size - 1];
    return AbstractState(var_vals);
    }*/

void PDBHeuristic::initialize() {
    cout << "Initializing pattern database heuristic..." << endl;

    cout << "Didn't do anything. Done initializing." << endl;
}

int PDBHeuristic::compute_heuristic(const State &state) {
    int h = distances[hash_index(AbstractState(state, pattern))];;
    if (h == numeric_limits<int>::max())
        return -1;
    return h;
}

const vector<int> &PDBHeuristic::get_pattern() {
    return pattern;
}

void PDBHeuristic::dump() const {
    for (size_t i = 0; i < num_states; ++i) {
        //AbstractState abs_state = inv_hash_index(i);
        //abs_state.dump();
        cout << "h-value: " << distances[i] << endl;
    }
}

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run) {
    int max_states = -1;
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_int_option(
                "max_states",
                &max_states,
                "maximum abstraction size");
            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }
    if (max_states == -1) {
        max_states = 1000000;
    }
    if (max_states < 1) {
        cerr << "error: abstraction size must be at least 1" << endl;
        exit(2);
    }
    //OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    
    vector<int> pattern;
    VariableOrderFinder vof(MERGE_LINEAR_GOAL_CG_LEVEL);
    int var = vof.next();
    int num_states = g_variable_domain[var];
    while (num_states <= max_states) {
        //cout << "Number of abstract states = " << num_states << endl;
        //cout << "Including variable: " << var << " (True name:" << g_variable_name[var] << ")" << endl;
        pattern.push_back(var);
        var = vof.next();
        num_states *= g_variable_domain[var];
    }
    
    return new PDBHeuristic(pattern);
}
