#include "hm_heuristic.h"


#include <cassert>
#include <limits>
#include <set>

HMHeuristic::HMHeuristic(int _m):m(_m) {
	MAX_VALUE = 100000;
	//MAX_VALUE = numeric_limits<int>::max();
}

HMHeuristic::~HMHeuristic() {
}

void HMHeuristic::initialize() {
    cout << "The implementation of the  h^m heuristic is preliminary" << endl;
    cout << "It is SLOOOOOOOOOOOW" << endl;
    cout << "Please do not use this for comparison" << endl;
	generate_all_tuples();
}

int HMHeuristic::compute_heuristic(const State &state) {
	if (test_goal(state)) {
		return 0;
	}
	else {
		//cout << "Evaluating state: " << endl;
		//state.dump();
		//cout << endl;

	    tuple s_tup;
		state_to_tuple(state, s_tup);

		init_hm_table(s_tup);
		//cout << "After Init" << endl;
		//dump_table();
		//cout << "********************************" << endl;

		update_hm_table();
		//cout << "After Update" << endl;
		//dump_table();
		//cout << "********************************" << endl;

		int h = eval(g_goal);
		//cout << "*** " << h << endl;

		// ******************************
		//init_hm_table(g_goal);
		//update_hm_table();
		//int h1 = eval(s_tup);
		//cout << "*** " << h1 << endl;
		// ******************************

		if (h == MAX_VALUE) {
            return DEAD_END;
        }
		return h;
	}
}


void HMHeuristic::init_hm_table(tuple &t) {
	map<tuple, int>::iterator it;
	for (it = hm_table.begin(); it != hm_table.end(); it++) {
		pair<tuple, int> hm_ent = *it;
		tuple &tup = hm_ent.first;
		int h_val = check_tuple_in_tuple(tup, t);
		hm_table[tup] = h_val;
	}
}


void HMHeuristic::update_hm_table() {
	int round = 0;
	do {
		round++;
		//cout << "Update round " << round << endl;
		//cout << "***********************************" << endl;
		//dump_table();
		//cout << "***********************************" << endl;
		was_updated = false;

		for (int op_id = 0; op_id < g_operators.size(); op_id++) {
			const Operator &op = g_operators[op_id];
			//cout << "Checking operator " << op.get_name() << endl;
			tuple pre;
			get_operator_pre(op, pre);
			int c1 = eval(pre);
			if (c1 < MAX_VALUE) {
				tuple eff;
				vector<tuple> partial_eff;

				get_operator_eff(op, eff);
				generate_all_partial_tuple(eff, partial_eff);
				for (int i = 0; i < partial_eff.size(); i++) {
					update_hm_entry(partial_eff[i], c1 + op.get_cost());

					if (partial_eff[i].size() < m) {
					    //cout << "Calling extend_tuple for ";
					    //print_tuple(partial_eff[i]);
					    //cout << endl;

					    //tuple others;
					    //extend_tuple(partial_eff[i], op, others, 0);

					    extend_tuple(partial_eff[i], op);
					}
				}
			}
		}

	} while (was_updated);
}

void HMHeuristic::extend_tuple(tuple &t, const Operator &op) {
    map<tuple, int>::const_iterator it;
    for (it = hm_table.begin(); it != hm_table.end(); it++) {
        pair<tuple, int> hm_ent = *it;
        tuple &entry = hm_ent.first;
        if ((entry.size() > t.size()) && (check_tuple_in_tuple(t, entry) == 0)) {

            tuple pre;
            get_operator_pre(op, pre);

            tuple others;
            for (int i = 0; i < entry.size(); i++) {
                pair<int,int> fact = entry[i];
                if (find(t.begin(), t.end(), fact) == t.end()) {
                    others.push_back(fact);
                    if (find(pre.begin(), pre.end(), fact) == pre.end()) {
                        pre.push_back(fact);
                    }
                }
            }

            sort(pre.begin(), pre.end());


            set<int> vars;
            bool is_valid = true;
            for (int i = 0; i < pre.size(); i++) {
                if (vars.count(pre[i].first) != 0) {
                    is_valid = false;
                    break;
                }
                vars.insert(pre[i].first);
            }

            if (is_valid) {
                int c2 = eval(pre);
                if (c2 < MAX_VALUE) {
                    //cout << "updating " << c2 + op.get_cost()<< endl;
                    update_hm_entry(entry, c2 + op.get_cost());
                }
            }
        }
    }
}

// generate all tuples which extend t with variables not affected by op
void HMHeuristic::extend_tuple(tuple &t, const Operator &op, tuple &others, int var) {
    if (others.size() < m) {
        for (int i = var; i < g_variable_domain.size(); i++) {
            // skip variables that are affected by op
            if (!is_effect_of(op, i) && !is_pre_of(op, i)) {
                for (int val = 0; val < g_variable_domain[i]; val++) {
                    tuple tup(others);
                    tup.push_back(make_pair(i, val));

                    //cout << "Others: ";
                    //print_tuple(tup);
                    //cout << endl;

                    tuple pre;
                    get_operator_pre(op, pre);
                    pre.insert(pre.end(), tup.begin(), tup.end());
                    sort(pre.begin(), pre.end());

                    //cout << "Evaluating ";
                    //print_tuple(tup);
                    //cout << endl;
                    int c2 = eval(pre);

                    if (c2 < MAX_VALUE) {
                        tuple ent(t);
                        ent.insert(ent.end(), tup.begin(), tup.end());
                        sort(ent.begin(), ent.end());

                        //cout << "Updating ";
                        //print_tuple(ent);
                        //cout << endl;

                        vector<tuple> entries;
                        generate_all_partial_tuple(ent, entries);

                        for (int j = 0; j < entries.size(); j++)
                            update_hm_entry(entries[j], c2 + op.get_cost());
                    }

                    extend_tuple(t, op, tup, i+1);
                }
            }
        }
        if (var < g_variable_domain.size()) {
            extend_tuple(t, op, others, var+1);
        }
    }
}

int HMHeuristic::eval(tuple &t) {
	vector<tuple> partial;
	generate_all_partial_tuple(t, partial);
	int max = 0;
	for (int i = 0; i < partial.size(); i++) {
		assert(hm_table.count(partial[i]) == 1);

		int h = hm_table[partial[i]];
		if (h > max) {
			max = h;
		}
	}
	return max;
}

int HMHeuristic::update_hm_entry(tuple &t, int val) {
	assert(hm_table.count(t) == 1);
	if (hm_table[t] > val) {
	    //cout << "Entry updated " << val << endl;
		hm_table[t] = val;
		was_updated = true;
	}
	return val;
}

void HMHeuristic::generate_all_tuples(int var, int sz, tuple &base) {
	if (sz == 1) {
		for (int i = var; i < g_variable_domain.size(); i++) {
			for (int j = 0; j < g_variable_domain[i]; j++) {
				tuple tup(base);
				tup.push_back(make_pair(i,j));
				hm_table[tup] = 0;
			}
		}
	}
	else {
		for (int i = var; i < g_variable_domain.size(); i++) {
			for (int j = 0; j < g_variable_domain[i]; j++) {
				tuple tup(base);
				tup.push_back(make_pair(i,j));
				hm_table[tup] = 0;
				generate_all_tuples(i+1, sz-1, tup);
			}
		}
	}
}

void HMHeuristic::generate_all_partial_tuple(tuple &base_tuple, tuple &t,
		int index, int sz, vector<tuple>& res) {
	if (sz == 1) {
		for (int i = index; i < base_tuple.size(); i++) {
			tuple tup(t);
			tup.push_back(base_tuple[i]);
			res.push_back(tup);
		}
	}
	else {
		for (int i = index; i < base_tuple.size(); i++) {
			tuple tup(t);
			tup.push_back(base_tuple[i]);
			res.push_back(tup);
			generate_all_partial_tuple(base_tuple, tup, i+1, sz - 1, res);
		}
	}
}


int HMHeuristic::check_tuple_in_tuple(const tuple &tup, const tuple& big_tuple) {
	for (int i = 0; i < tup.size(); i++) {
		bool found = false;
		for (int j = 0; j < big_tuple.size(); j++) {
			if (tup[i] == big_tuple[j]) {
				found = true;
				break;
			}
		}
		if (!found) {
			return MAX_VALUE;
		}

	}
	return 0;
}
