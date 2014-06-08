#ifndef HM_HEURISTIC_H
#define HM_HEURISTIC_H

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

#include "heuristic.h"
#include "globals.h"
#include "state.h"
#include "operator.h"

using namespace std;

typedef vector<pair<int, int> > tuple;

class Options;
/**
 * The h^m heuristic.
 * This is a preliminary implementation, and it is very slow.
 * Please do not use this for comparison.
 */
class HMHeuristic : public Heuristic {
public:
    HMHeuristic(const Options &opts);
    virtual ~HMHeuristic();
    virtual bool dead_ends_are_reliable() const;
protected:
    virtual int compute_heuristic(const State &state);
    virtual void initialize();

    // parameters
    int m;

    // h^m table
    map<tuple, int> hm_table;
    bool was_updated;

    void init_hm_table(tuple &t);
    void update_hm_table();
    int eval(tuple &t);
    int update_hm_entry(tuple &t, int val);
    void extend_tuple(tuple &t, const Operator &op);

    // some helper methods
    int check_tuple_in_tuple(const tuple &tup, const tuple &big_tuple);
    void state_to_tuple(const State &state, tuple &t) {
        for (int i = 0; i < g_variable_domain.size(); i++)
            t.push_back(make_pair(i, state[i]));
    }

    int get_operator_pre_value(const Operator &op, int var) {
        for (int i = 0; i < op.get_prevail().size(); i++) {
            if (op.get_prevail()[i].var == var)
                return op.get_prevail()[i].prev;
        }


        for (int i = 0; i < op.get_pre_post().size(); i++)
            if (op.get_pre_post()[i].var == var)
                return op.get_pre_post()[i].pre;

        return -1;
    }

    void get_operator_pre(const Operator &op, tuple &t) {
        for (int i = 0; i < op.get_prevail().size(); i++)
            t.push_back(make_pair(op.get_prevail()[i].var, op.get_prevail()[i].prev));

        for (int i = 0; i < op.get_pre_post().size(); i++)
            if (op.get_pre_post()[i].pre >= 0)
                t.push_back(make_pair(op.get_pre_post()[i].var, op.get_pre_post()[i].pre));

        sort(t.begin(), t.end());
    }

    void get_operator_eff(const Operator &op, tuple &t) {
        for (int i = 0; i < op.get_pre_post().size(); i++)
            t.push_back(make_pair(op.get_pre_post()[i].var, op.get_pre_post()[i].post));

        sort(t.begin(), t.end());
    }


    bool is_pre_of(const Operator &op, int var) {
        for (int j = 0; j < op.get_prevail().size(); j++) {
            if (op.get_prevail()[j].var == var) {
                return true;
            }
        }
        for (int j = 0; j < op.get_pre_post().size(); j++) {
            if (op.get_pre_post()[j].var == var) {
                return true;
            }
        }
        return false;
    }

    bool is_effect_of(const Operator &op, int var) {
        for (int j = 0; j < op.get_pre_post().size(); j++) {
            if (op.get_pre_post()[j].var == var) {
                return true;
            }
        }
        return false;
    }

    bool contradict_effect_of(const Operator &op, int var, int val) {
        for (int j = 0; j < op.get_pre_post().size(); j++) {
            if ((op.get_pre_post()[j].var == var) && (op.get_pre_post()[j].post != val)) {
                return true;
            }
        }
        return false;
    }


    void generate_all_tuples() {
        tuple t;
        generate_all_tuples(0, m, t);
    }
    void generate_all_tuples(int var, int sz, tuple &base);

    void generate_all_partial_tuple(tuple &base_tuple, vector<tuple> &res) {
        tuple t;
        generate_all_partial_tuple(base_tuple, t, 0, m, res);
    }
    void generate_all_partial_tuple(tuple &base_tuple, tuple &t, int index, int sz, vector<tuple> &res);

    void dump_table() const {
        map<tuple, int>::const_iterator it;
        for (it = hm_table.begin(); it != hm_table.end(); it++) {
            pair<tuple, int> hm_ent = *it;
            cout << "h[";
            print_tuple(hm_ent.first);
            cout << "] = " << hm_ent.second << endl;
        }
    }
    void print_tuple(tuple &tup) const {
        cout << tup[0].first << "=" << tup[0].second;
        for (int i = 1; i < tup.size(); i++)
            cout << "," << tup[i].first << "=" << tup[i].second;
    }
};

#endif
