#include "pdb_abstraction.h"
#include "abstract_state_iterator.h"
#include "operator.h"
#include "globals.h"

#include <cassert>
#include <queue>
#include <limits>

using namespace std;

// AbstractOperator -------------------------------------------------------------------------------

AbstractOperator::AbstractOperator(const Operator &o, const vector<int> &pattern) {
    const vector<Prevail> &prevail = o.get_prevail();
    const vector<PrePost> &pre_post = o.get_pre_post();
    for (size_t i = 0; i < pattern.size(); ++i) {
        int var = pattern[i];
        bool prev = false;
        for (size_t j = 0; j < prevail.size(); ++j) {
            if (var == prevail[j].var) {
                conditions.push_back(make_pair(var, prevail[j].prev));
                prev = true;
                break;
            }
        }
        if (prev)
            continue;
        for (size_t j = 0; j < pre_post.size(); ++j) {
            if (var == pre_post[j].var) {
                conditions.push_back(make_pair(var, pre_post[j].pre));
                effects.push_back(make_pair(var, pre_post[j].post));
            }
        }
    }
}

AbstractOperator::~AbstractOperator() {
}

void AbstractOperator::dump() const {
    cout << "AbstractOperator:" << endl;
    cout << "Conditions:" << endl;
    for (size_t i = 0; i < conditions.size(); ++i) {
        cout << "Variable: " << conditions[i].first << " (True name: " 
        << g_variable_name[conditions[i].first] << ") Value: " << conditions[i].second << endl;
    }
    cout << "Effects:" << endl;
    for (size_t i = 0; i < effects.size(); ++i) {
        cout << "Variable: " << effects[i].first << " (True name: " 
        << g_variable_name[effects[i].first] << ") Value: " << effects[i].second << endl;
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

bool AbstractState::is_applicable(const AbstractOperator &op, const vector<int> &var_to_index) const {
    const vector<pair<int, int> > &conditions = op.get_conditions();
    for (size_t i = 0; i < conditions.size(); ++i) {
        int var = conditions[i].first;
        int val = conditions[i].second;
        assert(var >= 0 && var < g_variable_name.size());
        assert(val == -1 || (val >= 0 && val < g_variable_domain[var]));
        if (val != -1 && val != variable_values[var_to_index[var]])
            return false;
    }
    return true;
}

void AbstractState::apply_operator(const AbstractOperator &op, const vector<int> &var_to_index) {
    assert(is_applicable(op, var_to_index));
    const vector<pair<int, int> > &effects = op.get_effects();
    for (size_t i = 0; i < effects.size(); ++i) {
        int var = effects[i].first;
        int val = effects[i].second;
        variable_values[var_to_index[var]] = val;
    }
}

bool AbstractState::is_goal_state(const vector<pair<int, int> > &abstract_goal, const vector<int> &var_to_index) const {
    for (size_t i = 0; i < abstract_goal.size(); ++i) {
        int var = abstract_goal[i].first;
        int val = abstract_goal[i].second;
        if (val != variable_values[var_to_index[var]]) {
            return false;
        }
    }
    return true;
}

void AbstractState::dump() const {
    cout << "AbstractState: " << endl;
    //TODO: if want to display Variable, need pattern to match back the index to the actual variable
    for (size_t i = 0; i < variable_values.size(); ++i) {
        cout << "Index:" << i << "Value:" << variable_values[i] << endl; 
    }
    /*for (map<int, int>::iterator it = copy_map.begin(); it != copy_map.end(); it++) {
        cout << "Variable: " <<  it->first << " (True name: " 
        << g_variable_name[it->first] << ") Value: " << it->second << endl;
    }*/
}

// PDBAbstraction ---------------------------------------------------------------------------------

PDBAbstraction::PDBAbstraction(const vector<int> &pat) : pattern(pat), size(pat.size()) {
    create_pdb();
}

PDBAbstraction::~PDBAbstraction() {
}

void PDBAbstraction::create_pdb() {
    n_i.reserve(size);
    variable_to_index.resize(g_variable_name.size());
    num_states = 1;
    int p = 1;
    for (size_t i = 0; i < size; ++i) {
        num_states *= g_variable_domain[pattern[i]];
        
        variable_to_index[pattern[i]] = i;
        
        n_i.push_back(p);
        p *= g_variable_domain[pattern[i]];
    }
    
    vector<AbstractOperator> operators;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        AbstractOperator ao(g_operators[i], pattern);
        if (!ao.get_effects().empty())
            operators.push_back(ao);
    }
    
    vector<pair<int, int> > abstracted_goal;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        for (size_t j = 0; j < size; ++j) {
            if (g_goal[i].first == pattern[j]) {
                abstracted_goal.push_back(g_goal[i]);
                break;
            }
        }
    }
    
    vector<vector<Edge> > back_edges;
    back_edges.resize(num_states);
    distances.reserve(num_states);
    // first entry: priority, second entry: index for an abstract state
    priority_queue<pair<int, size_t>, vector<pair<int, size_t> >, greater<pair<int, size_t> > > pq;
    
    vector<int> ranges;
    for (size_t i = 0; i < size; ++i) {
        ranges.push_back(g_variable_domain[pattern[i]]);
    }
    AbstractStateIterator it(ranges);
    while (!it.is_at_end()) {
        AbstractState abstract_state(it.get_current());
        int counter = it.get_counter();
        assert(hash_index(abstract_state) == counter);
        
        if (abstract_state.is_goal_state(abstracted_goal, variable_to_index)) {
            pq.push(make_pair(0, counter));
            distances.push_back(0);
        }
        else {
            distances.push_back(numeric_limits<int>::max());
        }
        for (size_t j = 0; j < operators.size(); ++j) {
            if (abstract_state.is_applicable(operators[j], variable_to_index)) {
                AbstractState next_state = abstract_state; //TODO: this didn't change anything compared to the old
                // AbstractOperator::apply_to_state-method where a new AbstractState was returned in the end ;-)
                next_state.apply_operator(operators[j], variable_to_index);
                size_t state_index = hash_index(next_state);
                assert(counter != state_index);
                back_edges[state_index].push_back(Edge(g_operators[j].get_cost(), counter));
            }
        }
        it.next();
    }

    while (!pq.empty()) {
        pair<int, int> node = pq.top();
        pq.pop();
        int distance = node.first;
        size_t state_index = node.second;
        if (distance > distances[state_index])
            continue;
        //distances[state_index] = distance;
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

size_t PDBAbstraction::hash_index(const AbstractState &abstract_state) const {
    size_t index = 0;
    for (int i = 0; i < size; ++i) {
        index += n_i[i] * abstract_state[variable_to_index[pattern[i]]];
    }
    return index;
}

/*AbstractState PDBAbstraction::inv_hash_index(int index) const {
    vector<int> var_vals;
    var_vals.resize(size);
    for (int n = 1; n < size; ++n) {
        int d = index % n_i[n];
        var_vals[variable_to_index[pattern[n - 1]]] = d / n_i[n - 1];
        index -= d;
    }
    var_vals[variable_to_index[pattern[size - 1]]] = index / n_i[size - 1];
    return AbstractState(var_vals);
}*/

int PDBAbstraction::get_heuristic_value(const State &state) const {
    return distances[hash_index(AbstractState(state, pattern))];
}

void PDBAbstraction::dump() const {
    for (size_t i = 0; i < num_states; ++i) {
        //AbstractState abs_state = inv_hash_index(i);
        //abs_state.dump();
        cout << "h-value: " << distances[i] << endl;
    }
}
