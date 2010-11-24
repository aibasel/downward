#include "pdb_heuristic.h"

#include "globals.h"
//#include "option_parser.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <queue>

using namespace std;

// AbstractOperator -------------------------------------------------------------------------------

AbstractOperator::AbstractOperator(const Operator &o, const vector<int> &pattern) {
    const vector<Prevail> &prevail = o.get_prevail();
    const vector<PrePost> &pre_post = o.get_pre_post();
    for (size_t i = 0; i < pattern.size(); i++) {
        int var = pattern[i];
        bool prev = false;
        for (size_t j = 0; j < prevail.size(); j++) {
            if (var == prevail[j].var) {
                conditions.push_back(make_pair(var, prevail[j].prev));
                prev = true;
                break;
            }
        }
        if (prev)
            continue;
        for (size_t j = 0; j < pre_post.size(); j++) {
            if (var == pre_post[j].var) {
                conditions.push_back(make_pair(var, pre_post[j].pre));
                effects.push_back(make_pair(var, pre_post[j].post));
            }
        }
    }
    /*pre_effect = vector<PreEffect>();
    for (size_t i = 0; i < pre_post.size(); i++) {
        int var = pre_post[i].var;
        for (size_t j = 0; j < pattern.size(); j++) {
            if (var == pattern[j]) {
                int pre = pre_post[i].pre;
                int post = pre_post[i].post;
                pre_effect.push_back(make_pair(var, make_pair(pre, post)));
                break;
            }
        }
    }*/
}

/*bool AbstractOperator::is_applicable(const AbstractState &abstract_state) const {
    if (pre_effect.size() == 0)
        return false;
    for (size_t i = 0; i < pre_effect.size(); i++) {
        int var = pre_effect[i].first;
        int pre = pre_effect[i].second.first;
        assert(var >= 0 && var < g_variable_name.size());
        assert(pre == -1 || (pre >= 0 && pre < g_variable_domain[var]));
        if (!(pre == -1 || abstract_state.get_variable_values()[var] == pre))
            return false;
    }
    return true;
}

AbstractState AbstractOperator::apply_operator(const AbstractState &abstract_state) const {
    assert(is_applicable(abstract_state));
    map<int, int> variable_values = abstract_state.get_variable_values();
    for (size_t i = 0; i < pre_effect.size(); i++) {
        int var = pre_effect[i].first;
        int post = pre_effect[i].second.second;
        variable_values[var] = post;
    }
    return AbstractState(variable_values);
}*/

void AbstractOperator::dump() const {
    cout << "AbstractOperator:" << endl;
    cout << "Conditions:" << endl;
    for (size_t i = 0; i < conditions.size(); i++) {
        cout << "Variable: " << conditions[i].first << " (True name: " 
        << g_variable_name[conditions[i].first] << ") Value: " << conditions[i].second << endl;
    }
    cout << "Effects:" << endl;
    for (size_t i = 0; i < effects.size(); i++) {
        cout << "Variable: " << effects[i].first << " (True name: " 
        << g_variable_name[effects[i].first] << ") Value: " << effects[i].second << endl;
    }
}

// AbstractState ----------------------------------------------------------------------------------

AbstractState::AbstractState(map<int, int> var_vals) {
    variable_values = var_vals;
}

AbstractState::AbstractState(const State &state, const vector<int> &pattern) {
    for (size_t i = 0; i < pattern.size(); i++) {
        int var = pattern[i];
        int val = state[var];
        variable_values[var] = val;
    }
}

bool AbstractState::is_applicable(const AbstractOperator &op) const {
    const vector<pair<int, int> > &conditions = op.get_conditions();
    if (conditions.size() == 0)
        return false;
    for (size_t i = 0; i < conditions.size(); ++i) {
        int var = conditions[i].first;
        int val = conditions[i].second;
        assert(var >= 0 && var < g_variable_name.size());
        assert(val == -1 || (val >= 0 && val < g_variable_domain[var]));
        if (val != -1 && val != variable_values.find(var)->second)
            return false;
    }
    return true;
}

void AbstractState::apply_operator(const AbstractOperator &op) {
    assert(is_applicable(op));
    const vector<pair<int, int> > &effects = op.get_effects();
    for (size_t i = 0; i < effects.size(); i++) {
        int var = effects[i].first;
        int val = effects[i].second;
        variable_values[var] = val;
    }
}

map<int, int> AbstractState::get_variable_values() const {
    return variable_values;
}

bool AbstractState::is_goal_state(const vector<pair<int, int> > &abstract_goal) const {
    for (size_t i = 0; i < abstract_goal.size(); i++) {
        int var = abstract_goal[i].first;
        int val = abstract_goal[i].second;
        if (val != variable_values.find(var)->second) {
            return false;
        }
    }
    return true;
}

void AbstractState::dump() const {
    cout << "AbstractState: " << endl;
    map<int, int> copy_map(variable_values);
    for (map<int, int>::iterator it = copy_map.begin(); it != copy_map.end(); it++) {
        cout << "Variable: " <<  it->first << " (True name: " 
        << g_variable_name[it->first] << ") Value: " << it->second << endl;
    }
}

// PDBAbstraction ---------------------------------------------------------------------------------

PDBAbstraction::PDBAbstraction(vector<int> pat) {
    pattern = pat;
    num_states = 1;
    for (size_t i = 0; i < pattern.size(); i++) {
        num_states *= g_variable_domain[pattern[i]];
    }
    distances = vector<int>(num_states, QUITE_A_LOT);
    create_pdb();
}

void PDBAbstraction::create_pdb() {
    n_i = vector<int>(pattern.size());
    for (size_t i = 0; i < pattern.size(); i++) {
        int p = 1;
        for (int j = 0; j < i; j++) {
            p *= g_variable_domain[pattern[j]];
        }
        n_i[i] = p;
    }
    
    vector<AbstractOperator> operators;
    for (size_t i = 0; i < g_operators.size(); i++) {
        operators.push_back(AbstractOperator(g_operators[i], pattern));
    }
    
    vector<pair<int, int> > abstracted_goal;
    for (size_t i = 0; i < g_goal.size(); i++) {
        for (size_t j = 0; j < pattern.size(); j++) {
            if (g_goal[i].first == pattern[j]) {
                abstracted_goal.push_back(make_pair(g_goal[i].first, g_goal[i].second));
                break;
            }
        }
    }
    
    std::vector<std::vector<Edge > > back_edges(num_states);
    priority_queue<Node, std::vector<Node>, compare> pq;
    for (size_t i = 0; i < num_states; i++) {
        AbstractState abstract_state = inv_hash_index(i);
        if (abstract_state.is_goal_state(abstracted_goal)) {
            pq.push(make_pair(i, 0));
        }
        else {
            pq.push(make_pair(i, QUITE_A_LOT));
        }
        for (size_t j = 0; j < operators.size(); j++){
            if (abstract_state.is_applicable(operators[j])) {
                AbstractState next_state = abstract_state; //TODO: this didn't change anything compared to the old
                // AbstractOperator::apply_to_state-method where a new AbstractState was returned in the end ;-)
                next_state.apply_operator(operators[j]);
                int state_index = hash_index(next_state);
                back_edges[state_index].push_back(Edge(g_operators[j].get_cost(), i));
            }
        }
    }
    while (!pq.empty()) {
        Node node = pq.top();
        pq.pop();
        int state_index = node.first;
        int distance = node.second;
        if (distance > distances[state_index])
            continue;
        distances[state_index] = distance;
        vector<Edge> edges = back_edges[state_index];
        for (size_t i = 0; i < edges.size(); i++) {
            int predecessor = edges[i].target;
            int cost = edges[i].cost;
            int alternative = distances[state_index] + cost;
            if (alternative < distances[predecessor]) {
                distances[predecessor] = alternative;
                pq.push(make_pair(predecessor, alternative));
            }
        }
    }
}

int PDBAbstraction::hash_index(const AbstractState &abstract_state) const {
    int index = 0;
    for (int i = 0;i < pattern.size();i++) {
        index += n_i[i]*abstract_state.get_variable_values()[pattern[i]];
    }
    return index;
}

AbstractState PDBAbstraction::inv_hash_index(int index) const {
    map<int, int> var_vals;
    for (int n = 1;n < pattern.size();n++) {
        int d = index%n_i[n];
        var_vals[pattern[n-1]] = (int) d/n_i[n-1];
        index -= d;
    }
    var_vals[pattern[pattern.size()-1]] = (int) index/n_i[pattern.size()-1];
    return AbstractState(var_vals);
}

int PDBAbstraction::get_heuristic_value(const State &state) const {
    AbstractState abstract_state(state, pattern);
    int index = hash_index(abstract_state);
    return distances[index];
}

void PDBAbstraction::dump() const {
    for (size_t i = 0; i < num_states; i++) {
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
    for (size_t i = 0; i < pat_coll.size(); i++) {
        vertices_1[i] = i;
        vertices_2[i] = i;
    }
    expand(vertices_1, vertices_2);
    cout << "expanded." << endl;
    dump();
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
                            cout << "operator:" << endl;
                            o.dump();
                            cout << "affects pattern:" << endl;
                            for (size_t n = 0; n < p1.size(); n++) {
                                cout << "variable: " << p1[n] << " (real name: " << g_variable_name[p1[n]] << ")" << endl;
                            }
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
                                cout << "and also affects pattern:" << endl;
                                for (size_t n = 0; n < p2.size(); n++) {
                                    cout << "variable: " << p2[n] << " (real name: " << g_variable_name[p2[n]] << ")" << endl;
                                }
                                cout << endl;
                                return false;
                            }
                        }
                    }
                    cout << "but doesn't affect pattern:" << endl;
                    for (size_t n = 0; n < p2.size(); n++) {
                        cout << "variable: " << p2[n] << " (real name: " << g_variable_name[p2[n]] << ")" << endl;
                    }
                    cout << endl;
                }
            }
        }
    }

    return true;
}

void CanonicalHeuristic::build_cgraph() {
    /*
    vector<int> p0;
    vector<int> p1;
    vector<int> p2;
    vector<int> p3;
    vector<int> p4;
    vector<int> p5;
    vector<int> p6;
    vector<int> p7;
    vector<int> p8;
    int edges[] = {1,8,0,2,8,1,3,7,8,2,4,5,6,7,3,5,3,4,6,7,3,5,7,2,3,5,6,0,1,2};
    p0.assign(edges, edges+2);
    cgraph.push_back(p0);
    p1.assign(edges+2, edges+5);
    cgraph.push_back(p1);
    p2.assign(edges+5, edges+9);
    cgraph.push_back(p2);
    p3.assign(edges+9, edges+14);
    cgraph.push_back(p3);
    p4.assign(edges+14, edges+16);
    cgraph.push_back(p4);
    p5.assign(edges+16, edges+20);
    cgraph.push_back(p5);
    p6.assign(edges+20, edges+23);
    cgraph.push_back(p6);
    p7.assign(edges+23, edges+27);
    cgraph.push_back(p7);
    p8.assign(edges+27, edges+30);
    cgraph.push_back(p8);
   */
    
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
    for (size_t i = 0; i < max_cliques.size(); i++) {
        vector<int> clique = max_cliques[i];
        int h_val = 0;
        for (size_t j = 0; j < clique.size(); j++) {
            vector<int> pattern = pattern_collection[clique[j]];
            PDBAbstraction *pdb_abstraction = new PDBAbstraction(pattern);
            h_val += pdb_abstraction->get_heuristic_value(state);
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
        cout << "[ ";
        for (size_t j = 0; j < cgraph[i].size(); j++) {
            cout << cgraph[i][j] << " ";
        }
        cout << "]" << endl;
    }
    // print maximal cliques
    cout << "Maximal cliques" << endl;
    if (max_cliques.size() == 0) {
        cout << "no max cliques";
    }
    else {
        cout << "max cliques are ";
    }
    for (size_t i = 0; i < max_cliques.size(); i++) {
        cout << "[ ";
        for (size_t j = 0; j < max_cliques[i].size(); j++) {
            cout << max_cliques[i][j] << " ";
        }
        cout << "] ";
    }
    cout << endl;
}

// PDBHeuristic -----------------------------------------------------------------------------------

static ScalarEvaluatorPlugin pdb_heuristic_plugin(
    "pdb", PDBHeuristic::create);

PDBHeuristic::PDBHeuristic() {
}

PDBHeuristic::~PDBHeuristic() {
}

void PDBHeuristic::verify_no_axioms_no_cond_effects() const {
    if (!g_axioms.empty()) {
        cerr << "Heuristic does not support axioms!" << endl << "Terminating." << endl;
        exit(1);
    }
    for (int i = 0; i < g_operators.size(); i++) {
        const vector<PrePost> &pre_post = g_operators[i].get_pre_post();
        for (int j = 0; j < pre_post.size(); j++) {
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
    cout << "Initializing pattern database heuristic..." << endl;// << endl;
    verify_no_axioms_no_cond_effects();
    int patt[2] = {10, 11};
    vector<int> pattern(patt, patt + sizeof(patt) / sizeof(int));
    //cout << "Try creating a new PDBAbstraction" << endl << endl;
    pdb_abstraction = new PDBAbstraction(pattern);
    //pdb_abstraction->dump();
    // probBlocks-6-2.pddl
    /*int patt_1[2] = {10, 11}; // on(b, c), on(e, f)
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[2] = {9, 10}; // on(f, a), on(b, c)
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    int patt_3[2] = {8, 12}; // on(c, d), on(a, b)
    vector<int> pattern_3(patt_3, patt_3 + sizeof(patt_3) / sizeof(int));
    vector<vector<int> > pattern_collection(3);
    pattern_collection[0] = pattern_1;
    pattern_collection[1] = pattern_2;
    pattern_collection[2] = pattern_3;*/
    
    /*int patt_1[1] = {2};
    vector<int> pattern_1(patt_1, patt_1 + sizeof(patt_1) / sizeof(int));
    int patt_2[1] = {3};
    vector<int> pattern_2(patt_2, patt_2 + sizeof(patt_2) / sizeof(int));
    vector<vector<int> > pattern_collection(2);
    pattern_collection[0] = pattern_1;
    pattern_collection[1] = pattern_2;
    canonical_heuristic = new CanonicalHeuristic(pattern_collection);*/
    cout << "Done initializing." << endl;
}

int PDBHeuristic::compute_heuristic(const State &state) {
    return pdb_abstraction->get_heuristic_value(state);
    //return canonical_heuristic->get_heuristic_value(state);
}

ScalarEvaluator *PDBHeuristic::create(const std::vector<string> &config, int start, int &end, bool dry_run) {
    //TODO: check what we have to do here!
    OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    else
        return new PDBHeuristic;
}
