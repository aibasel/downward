#include "pdb_heuristic.h"

#include "globals.h"
//#include "option_parser.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"
#include "timer.h"
#include <cassert>
#include <cstdlib>
#include <algorithm>
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
    for (size_t i = 0; i < size; ++i) {
        num_states *= g_variable_domain[pattern[i]];
        
        variable_to_index[pattern[i]] = i;
        
        int p = 1;
        for (int j = 0; j < i; ++j) {
            p *= g_variable_domain[pattern[j]];
        }
        n_i.push_back(p);
    }
    
    vector<AbstractOperator> operators;
    //operators.reserve(g_operators.size());
    for (size_t i = 0; i < g_operators.size(); ++i) {
        AbstractOperator ao(g_operators[i], pattern);
        if (!ao.get_effects().empty())
            operators.push_back(ao);
    }
    //cout << "number of operators:" << g_operators.size() << " number of abstract operators:"
    //<< operators.size() << endl;
    
    vector<pair<int, int> > abstracted_goal;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        for (size_t j = 0; j < size; ++j) {
            if (g_goal[i].first == pattern[j]) {
                abstracted_goal.push_back(g_goal[i]);
                break;
            }
        }
    }
    
    vector<vector<Edge > > back_edges;
    back_edges.resize(num_states);
    distances.reserve(num_states);
    priority_queue<pair<int, size_t>, vector<pair<int, size_t> >, greater<pair<int, size_t> > > pq;
    // first entry: priority, second entry: index for an abstract state
    for (size_t i = 0; i < num_states; ++i) {
        AbstractState abstract_state = inv_hash_index(i);
        if (abstract_state.is_goal_state(abstracted_goal, variable_to_index)) {
            pq.push(make_pair(0, i));
            distances.push_back(0);
        }
        else {
            pq.push(make_pair(numeric_limits<int>::max(), i));
            distances.push_back(numeric_limits<int>::max());
        }
        for (size_t j = 0; j < operators.size(); ++j) {
            if (abstract_state.is_applicable(operators[j], variable_to_index)) {
                AbstractState next_state = abstract_state; //TODO: this didn't change anything compared to the old
                // AbstractOperator::apply_to_state-method where a new AbstractState was returned in the end ;-)
                next_state.apply_operator(operators[j], variable_to_index);
                size_t state_index = hash_index(next_state);
                assert(i != state_index);
                back_edges[state_index].push_back(Edge(g_operators[j].get_cost(), i));
            }
        }
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

AbstractState PDBAbstraction::inv_hash_index(int index) const {
    vector<int> var_vals;
    var_vals.resize(size);
    for (int n = 1; n < size; ++n) {
        int d = index % n_i[n];
        var_vals[variable_to_index[pattern[n - 1]]] = (int) d / n_i[n - 1];
        index -= d;
    }
    var_vals[variable_to_index[pattern[size - 1]]] = (int) index / n_i[size - 1];
    return AbstractState(var_vals);
}

int PDBAbstraction::get_heuristic_value(const State &state) const {
    return distances[hash_index(AbstractState(state, pattern))];
}

void PDBAbstraction::dump() const {
    for (size_t i = 0; i < num_states; ++i) {
        AbstractState abs_state = inv_hash_index(i);
        abs_state.dump();
        cout << "h-value: " << distances[i] << endl;
    }
}

// CanonicalHeuristic -----------------------------------------------------------------------------

CanonicalHeuristic::CanonicalHeuristic(vector<vector<int> > pat_coll) {
    pattern_collection = pat_coll;
    build_cgraph();
    cout << "built cgraph." << endl;
    vector<int> vertices_1(pat_coll.size());
    vector<int> vertices_2(pat_coll.size());
    for (size_t i = 0; i < pat_coll.size(); ++i) {
        vertices_1[i] = i;
        vertices_2[i] = i;
    }
    expand(vertices_1, vertices_2);
    cout << "expanded." << endl;
    dump();
}

CanonicalHeuristic::~CanonicalHeuristic() {
}

bool CanonicalHeuristic::are_additive(int pattern1, int pattern2) const {
    vector<int> p1 = pattern_collection[pattern1];
    vector<int> p2 = pattern_collection[pattern2];

    for (size_t i = 0; i < p1.size(); i++) {
        for (size_t j = 0; j < p2.size(); j++) {
            // now check which operators affect pattern1
            for (size_t k = 0; k < g_operators.size(); k++) {
                bool p1_affected = false;
                const Operator &o = g_operators[k];
                const vector<PrePost> effects = o.get_pre_post();
                for (size_t l = 0; l < effects.size(); l++) {
                    // every effect affect one specific variable
                    int var = effects[l].var;
                    // we have to check if this variable is in our pattern
                    for (size_t m = 0; m < p1.size(); m++) {
                        if (var == p1[m]) {
                            // this operator affects pattern 1
                            p1_affected = true;
                            //cout << "operator:" << endl;
                            //o.dump();
                            //cout << "affects pattern:" << endl;
                            //for (size_t n = 0; n < p1.size(); n++) {
                                //cout << "variable: " << p1[n] << " (real name: " << g_variable_name[p1[n]] << ")" << endl;
                            //}
                            break;
                        }
                    }
                    if (p1_affected) {
                        break;
                    }
                }
                if (p1_affected) {
                    // check if the same operator also affects pattern 2
                    for (size_t l = 0; l < effects.size(); l++) {
                        int var = effects[l].var;
                        for (size_t m = 0; m < p2.size(); m++) {
                            if (var == p2[m]) {
                                // this operator affects also pattern 2
                                //cout << "and also affects pattern:" << endl;
                                //for (size_t n = 0; n < p2.size(); n++) {
                                    //cout << "variable: " << p2[n] << " (real name: " << g_variable_name[p2[n]] << ")" << endl;
                                //}
                                //cout << endl;
                                return false;
                            }
                        }
                    }
                    //cout << "but doesn't affect pattern:" << endl;
                    //for (size_t n = 0; n < p2.size(); n++) {
                        //cout << "variable: " << p2[n] << " (real name: " << g_variable_name[p2[n]] << ")" << endl;
                    //}
                    //cout << endl;
                }
            }
        }
    }
    return true;
}

void CanonicalHeuristic::build_cgraph() { 
    // initialize compatibility graph
    cgraph = vector<vector<int> >(pattern_collection.size());

    for (size_t i = 0; i < pattern_collection.size(); i++) {
        for (size_t j = i+1; j < pattern_collection.size(); j++) {
            if (are_additive(i,j)) {
                // if the two patterns are additive there is an edge in the compatibility graph
                cout << "pattern (index) " << i << " additive with " << "pattern (index) " << j << endl;
                cgraph[i].push_back(j);
                cgraph[j].push_back(i);
            }
        }
    }
}

int CanonicalHeuristic::get_maxi_vertex(const vector<int> &subg, const vector<int> &cand) const {
    // assert that subg and cand are sorted
    // how is complexity of subg.sort() and cand.sort() if they are sorted yet?
    int max = -1;
    int vertex = -1;
    
    vector<int>::const_iterator it = subg.begin();
    while (*it > -1 && it != subg.end()) {
        vector<int> intersection(subg.size(), -1); // intersection [ -1, -1, ..., -1 ]
        // for vertex u in subg get u's adjacent vertices --> cgraph[subg[i]];
        vector<int>::iterator it2 = set_intersection(cand.begin(), cand.end(),
            cgraph[*it].begin(), cgraph[*it].end(), intersection.begin());
        if (int(it2 - intersection.begin()) > max) {
            max = int(it2 - intersection.begin());
            vertex = *it;
        }
        it++;
    }
    return vertex;
}

void CanonicalHeuristic::expand(vector<int> &subg, vector<int> &cand) {
    if (subg[0] == -1) {
        //cout << "clique" << endl;
        max_cliques.push_back(q_clique);
    }
    else {
        int u = get_maxi_vertex(subg, cand);
        
        vector<int> ext_u(cand.size(), -1); // ext_u = cand - gamma(u)
        set_difference(cand.begin(), cand.end(), cgraph[u].begin(), cgraph[u].end(), ext_u.begin());
        
        vector<int>::iterator it_ext = ext_u.begin();
        vector<int>::iterator it_cand = cand.begin();
        
        while (*it_ext > -1 && it_ext != ext_u.end()) { // while cand - gamma(u) is not empty
            
            int q = *it_ext; // q is a vertex in cand - gamma(u)
            //cout << q << ",";
            q_clique.push_back(q);
            
            // subg_q = subg n gamma(q)
            vector<int> subg_q(subg.size(), -1);
            set_intersection(subg.begin(), subg.end(), cgraph[q].begin(), cgraph[q].end(), subg_q.begin());
            
            // cand_q = cand n gamma(q)
            vector<int> cand_q(cand.size(), -1);
            set_intersection(it_cand, cand.end(), cgraph[q].begin(), cgraph[q].end(), cand_q.begin());
            
            expand(subg_q, cand_q);
            
            // remove q from cand --> cand = cand - q
            it_cand++;
            it_ext++;
            
            //cout << "back"  << endl;
            q_clique.pop_back();
        }
    }
}

int CanonicalHeuristic::get_heuristic_value(const State &state) const {
    // h^C(state) = max_{D \in max_cliques(C)} \sum_{P \in D} h^P(state)
    int max_val = 0;
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        vector<int> clique = max_cliques[i];
        int h_val = 0;
        for (size_t j = 0; j < clique.size(); ++j) {
            PDBAbstraction pdb = pattern_databases.find(clique[j])->second;
            h_val += pdb.get_heuristic_value(state);
        }
        if (h_val > max_val) {
            max_val = h_val;
        }
    }
    return max_val;
}

void CanonicalHeuristic::dump() const {
    // print compatibility graph
    cout << "Compatibility graph" << endl;
    for (size_t i = 0; i < cgraph.size(); i++) {
        cout << i << " adjacent to [ ";
        for (size_t j = 0; j < cgraph[i].size(); j++) {
            cout << cgraph[i][j] << " ";
        }
        cout << "]" << endl;
    }
    // print maximal cliques
    assert(max_cliques.size() > 0);
    cout << max_cliques.size() << " maximal clique(s)" << endl;
    cout << "Maximal cliques are { ";
    for (size_t i = 0; i < max_cliques.size(); i++) {
        cout << "[ ";
        for (size_t j = 0; j < max_cliques[i].size(); j++) {
            cout << max_cliques[i][j] << " ";
        }
        cout << "] ";
    }
    cout << "}" << endl;
}

// PDBHeuristic -----------------------------------------------------------------------------------

static ScalarEvaluatorPlugin pdb_heuristic_plugin(
    "pdb", PDBHeuristic::create);

PDBHeuristic::PDBHeuristic() {
}

PDBHeuristic::~PDBHeuristic() {
    delete pdb_abstraction;
    //delete canonical_heuristic;
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

void PDBHeuristic::initialize() {
    cout << "Initializing pattern database heuristic..." << endl;
    verify_no_axioms_no_cond_effects();
    
    // pdbabstraction function tests
    // 1. blocks-7-2 test-pattern
    //int patt[] = {9, 10, 11, 12, 13, 14};
    
    // 2. driverlog-6 test-pattern
    //int patt[] = {4, 5, 7, 9, 10, 11, 12};
    
    // 3. logistics00-6-2 test-pattern
    //int patt[] = {3, 4, 5, 6, 7, 8};
    
    // 4. blocks-9-0
    //int patt[] = {0};
    
    // 5. logistics00-5-1 test-pattern
    int patt[] = {0, 1, 2, 3, 4, 5, 6, 7};
    
    vector<int> pattern(patt, patt + sizeof(patt) / sizeof(int));
    Timer timer;
    timer();
    pdb_abstraction = new PDBAbstraction(pattern);
    timer.stop();
    cout << "PDB construction time: " << timer << endl;
    
    
    // Canonical heuristic function tests
    //1. one pattern
    /*int patt_1[6] = {3, 4, 5, 6, 7, 8};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    vector<vector<int> > pattern_collection(1);
    pattern_collection[0] = pattern_1;*/

    //2. two patterns
    /*int patt_1[3] = {3, 4, 5};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[3] = {6, 7, 8};
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    vector<vector<int> > pattern_collection(2);
    pattern_collection[0] = pattern_1;
    pattern_collection[1] = pattern_2;*/

    //3. three patterns
    /*int patt_1[2] = {3, 4};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[2] = {5, 6};
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    int patt_3[2] = {7, 8};
    vector<int> pattern_3(patt_3, patt_3 + sizeof(patt_3) / sizeof(int));
    vector<vector<int> > pattern_collection(3);
    pattern_collection[0] = pattern_1;
    pattern_collection[1] = pattern_2;
    pattern_collection[2] = pattern_3;*/

    //canonical_heuristic = new CanonicalHeuristic(pattern_collection);

    // build all pdbs
    /*Timer timer;
    timer();
    for (int i = 0; i < pattern_collection.size(); ++i) {
        PDBAbstraction pdb = PDBAbstraction(pattern_collection[i]); // TODO delete it anywhere
        canonical_heuristic->pattern_databases.insert(pair<int, PDBAbstraction>(i, pdb));
    }
    timer.stop();
    cout << pattern_collection.size() << " pdbs constructed." << endl;
    cout << "Construction time for all pdbs: " << timer << endl;*/

    //cout << "Done initializing." << endl;
}

int PDBHeuristic::compute_heuristic(const State &state) {
    int h = pdb_abstraction->get_heuristic_value(state);
    if (h == numeric_limits<int>::max())
        return -1;
    return h;
    //return canonical_heuristic->get_heuristic_value(state);
}

ScalarEvaluator *PDBHeuristic::create(const vector<string> &config, int start, int &end, bool dry_run) {
    //TODO: check what we have to do here!
    OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    else
        return new PDBHeuristic;
}
