#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H

#include "landmark_factory.h"
#include "landmark_graph.h"
#include <ext/hash_set>
#include "../globals.h"

class LandmarkFactoryRpgSasp : public LandmarkFactory {
    list<LandmarkNode *> open_landmarks;

    void find_forward_orders(const std::vector<std::vector<int> > &lvl_var,
                             LandmarkNode *lmp);
    void add_lm_forward_orders();

    void get_greedy_preconditions_for_lm(const LandmarkNode *lmp,
                                         const Operator &o, __gnu_cxx::hash_map<int, int> &result) const;
    void compute_shared_preconditions(__gnu_cxx::hash_map<int, int> &shared_pre, std::vector<
                                          std::vector<int> > &lvl_var, LandmarkNode *bp);
    void compute_disjunctive_preconditions(
        std::vector<std::set<std::pair<int, int> > > &disjunctive_pre,
        std::vector<std::vector<int> > &lvl_var, LandmarkNode *bp);

    int min_cost_for_landmark(LandmarkNode *bp, std::vector<std::vector<int> > &lvl_var);
    void generate_landmarks();
    void found_simple_lm_and_order(const std::pair<int, int> a, LandmarkNode &b,
                                   edge_type t);
    void found_disj_lm_and_order(const std::set<std::pair<int, int> > a, LandmarkNode &b,
                                 edge_type t);
    void approximate_lookahead_orders(const std::vector<std::vector<int> > &lvl_var,
                                      LandmarkNode *lmp);
    static bool domain_connectivity(const std::pair<int, int> &landmark,
                                    const __gnu_cxx::hash_set<int> &exclude);
public:
    LandmarkFactoryRpgSasp(const Options &opts);
    virtual ~LandmarkFactoryRpgSasp() {}
};

#endif
