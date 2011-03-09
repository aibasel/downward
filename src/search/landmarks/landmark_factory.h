#ifndef LANDMARKS_LANDMARK_FACTORY
#define LANDMARKS_LANDMARK_FACTORY

#include "landmark_graph.h"
#include "exploration.h"

class LandmarkFactory {
public:
    LandmarkFactory(LandmarkGraph::Options &options, Exploration *exploration);
    virtual ~LandmarkFactory() {};
    // TODO: compute_lm_graph *must* be called to avoid memory leeks!
    // returns a landmarkgraph created by a factory class.
    // take care to delete the pointer when you don't need it anymore!
    // (method is principally anyways called by every inheriting class)
    LandmarkGraph *compute_lm_graph();
protected:
    LandmarkGraph *lm_graph;
    
    // protected not private for LandmarkFactoryRpgSasp
    struct Pddl_proposition {
        string predicate;
        vector<string> arguments;
        string to_string() const {
            string output = predicate;
            for (unsigned int i = 0; i < arguments.size(); i++) {
                output += " ";
                output += arguments[i];
            }
            return output;
        }
    };
    hash_map<pair<int, int>, Pddl_proposition, hash_int_pair> pddl_propositions;
    map<string, int> pddl_proposition_indices; //TODO: make this a hash_map

    virtual void generate_landmarks() = 0;
    void read_external_inconsistencies();
    void discard_noncausal_landmarks();
    void discard_disjunctive_landmarks();
    void discard_conjunctive_landmarks();
    void discard_all_orderings();
    void generate();
    //bool is_dead_end() const {return dead_end_found; }
    //void count_shared_costs();
    inline bool inconsistent(const pair<int, int> &a, const pair<int, int> &b) const {
        //if (a.first == b.first && a.second == b.second)
        if (a == b)
            return false;
        if (a.first != b.first || a.second != b.second)
            if (a.first == b.first && a.second != b.second)
                return true;
            if (/*external_inconsistencies_read &&*/
                inconsistent_facts[a.first][a.second].find(b) != inconsistent_facts[a.first][a.second].end())
                return true;
            return false;
    }
    inline bool relaxed_task_solvable(bool level_out,
                                      const LandmarkNode *exclude,
                                      bool compute_lvl_op = false) const {
        vector<vector<int> > lvl_var;
        vector<hash_map<pair<int, int>, int, hash_int_pair> > lvl_op;
        return relaxed_task_solvable(lvl_var, lvl_op, level_out, exclude, compute_lvl_op);
    }
    void edge_add(LandmarkNode &from, LandmarkNode &to, edge_type type);
    void compute_predecessor_information(LandmarkNode *bp,
                                         vector<vector<int> > &lvl_var,
                                         vector<hash_map<pair<int, int>, int, hash_int_pair> > &lvl_op);

    // protected not private for LandmarkFactoryRpgSearch
    bool achieves_non_conditional(const Operator &o, const LandmarkNode *lmp) const;
    bool is_landmark_precondition(const Operator &o, const LandmarkNode *lmp) const;

private:
    vector<vector<set<pair<int, int> > > > inconsistent_facts;

    bool interferes(const LandmarkNode *, const LandmarkNode *) const;
    bool effect_always_happens(const vector<PrePost> &prepost,
                               set<pair<int, int> > &eff) const;
    void approximate_reasonable_orders(bool obedient_orders);
    void mk_acyclic_graph();
    int loop_acyclic_graph(LandmarkNode &lmn,
                           hash_set<LandmarkNode *, hash_pointer> &acyclic_node_set);
    bool remove_first_weakest_cycle_edge(LandmarkNode *cur,
                                         list<pair<LandmarkNode *, edge_type> > &path,
                                         list<pair<LandmarkNode *, edge_type> >::iterator it);
    int calculate_lms_cost() const;
    void collect_ancestors(hash_set<LandmarkNode *, hash_pointer> &result, LandmarkNode &node,
                           bool use_reasonable);
    bool relaxed_task_solvable(vector<vector<int> > &lvl_var,
                               vector<hash_map<pair<int, int>, int, hash_int_pair> > &lvl_op,
                               bool level_out,
                               const LandmarkNode *exclude,
                               bool compute_lvl_op = false) const;
    /*bool relaxed_task_solvable_without_operator(vector<vector<int> > &lvl_var,
    vector<hash_map<pair<int, int>, int, hash_int_pair> > &lvl_op,
                                        bool level_out,
                                        const Operator *exclude,
                                        bool compute_lvl_op = false) const;*/
    bool is_causal_landmark(const LandmarkNode &landmark) const;
    //void reset_landmarks_count() {landmarks_count = nodes.size(); }
    void calc_achievers();
};

#endif
