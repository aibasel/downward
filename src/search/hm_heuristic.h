#ifndef HM_HEURISTIC_H
#define HM_HEURISTIC_H

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "global_state.h"
#include "global_operator.h"
#include "globals.h"
#include "heuristic.h"


typedef std::vector<std::pair<int, int> > Tuple;

class Options;

/*
  Haslum's h^m heuristic family ("critical path heuristics").

  This is a very slow implementation and should not be used for
  speed benchmarks.
*/

class HMHeuristic : public Heuristic {
    // parameters
    int m;

    // h^m table
    std::map<Tuple, int> hm_table;
    bool was_updated;

    // auxiliary methods
    void init_hm_table(Tuple &t);
    void update_hm_table();
    int eval(Tuple &t) const;
    int update_hm_entry(Tuple &t, int val);
    void extend_tuple(Tuple &t, const GlobalOperator &op);

    int check_tuple_in_tuple(const Tuple &tup, const Tuple &big_tuple) const;
    void state_to_tuple(const GlobalState &state, Tuple &t) const;

    int get_operator_pre_value(const GlobalOperator &op, int var) const;
    void get_operator_pre(const GlobalOperator &op, Tuple &t) const;
    void get_operator_eff(const GlobalOperator &op, Tuple &t) const;
    bool is_pre_of(const GlobalOperator &op, int var) const;
    bool is_effect_of(const GlobalOperator &op, int var) const;
    bool contradict_effect_of(const GlobalOperator &op, int var, int val) const;

    void generate_all_tuples();
    void generate_all_tuples_aux(int var, int sz, Tuple &base);

    void generate_all_partial_tuples(Tuple &base_tuple,
                                     std::vector<Tuple> &res) const;
    void generate_all_partial_tuples_aux(Tuple &base_tuple, Tuple &t, int index,
                                         int sz, std::vector<Tuple> &res) const;

    void dump_table() const;
    void dump_tuple(Tuple &tup) const;

protected:
    virtual int compute_heuristic(const GlobalState &state);
    virtual void initialize();

public:
    HMHeuristic(const Options &opts);
    virtual ~HMHeuristic();
    virtual bool dead_ends_are_reliable() const;
};

#endif
