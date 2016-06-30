#ifndef HEURISTICS_HM_HEURISTIC_H
#define HEURISTICS_HM_HEURISTIC_H

#include "../heuristic.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace options {
class Options;
}

namespace hm_heuristic {
/*
  Haslum's h^m heuristic family ("critical path heuristics").

  This is a very slow implementation and should not be used for
  speed benchmarks.
*/
using Facts = std::vector<FactPair>;

class HMHeuristic : public Heuristic {
    // parameters
    int m;
    bool has_cond_effects;

    // h^m table
    std::map<Facts, int> hm_table;
    bool was_updated;

    // auxiliary methods
    void init_hm_table(const Facts &t);
    void update_hm_table();
    int eval(const Facts &t) const;
    int update_hm_entry(const Facts &t, int val);
    void extend_tuple(const Facts &t, const OperatorProxy &op);

    int check_tuple_in_tuple(const Facts &tup, const Facts &big_tuple) const;
    void state_to_tuple(const State &state, Facts &t) const;

    int get_operator_pre_value(const OperatorProxy &op, int var) const;
    void get_operator_pre(const OperatorProxy &op, Facts &t) const;
    void get_operator_eff(const OperatorProxy &op, Facts &t) const;
    bool contradict_effect_of(const OperatorProxy &op, int var, int val) const;

    void generate_all_tuples();
    void generate_all_tuples_aux(int var, int sz, const Facts &base);

    void generate_all_partial_tuples(const Facts &base_tuple,
                                     std::vector<Facts> &res) const;
    void generate_all_partial_tuples_aux(const Facts &base_tuple, const Facts &t, int index,
                                         int sz, std::vector<Facts> &res) const;

    void dump_table() const;
    void dump_tuple(const Facts &tup) const;

protected:
    virtual int compute_heuristic(const GlobalState &global_state);
    virtual void initialize();

public:
    HMHeuristic(const options::Options &opts);
    virtual ~HMHeuristic();
    virtual bool dead_ends_are_reliable() const;
};
}

#endif
