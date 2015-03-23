#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H

#include "landmark_factory.h"
#include "landmark_graph.h"

#include "../globals.h"

#include <unordered_map>
#include <vector>


class LandmarkFactoryRpgSasp : public LandmarkFactory {
    std::list<LandmarkNode *> open_landmarks;
    std::vector<std::vector<int> > disjunction_classes;

    void find_forward_orders(const std::vector<std::vector<int> > &lvl_var,
                             LandmarkNode *lmp);
    void add_lm_forward_orders();

    void get_greedy_preconditions_for_lm(const LandmarkNode *lmp,
                                         const GlobalOperator &o, std::unordered_map<int, int> &result) const;
    void compute_shared_preconditions(std::unordered_map<int, int> &shared_pre,
                                      std::vector<std::vector<int> > &lvl_var,
                                      LandmarkNode *bp);
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
                                    const std::unordered_set<int> &exclude);

    void build_disjunction_classes();
public:
    LandmarkFactoryRpgSasp(const Options &opts);
    virtual ~LandmarkFactoryRpgSasp() {}
};

#endif
