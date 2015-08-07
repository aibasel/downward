#include "hm_heuristic.h"

#include "option_parser.h"
#include "plugin.h"

#include <cassert>
#include <limits>
#include <set>

using namespace std;


HMHeuristic::HMHeuristic(const Options &opts)
    : Heuristic(opts),
      m(opts.get<int>("m")) {
}


HMHeuristic::~HMHeuristic() {
}


bool HMHeuristic::dead_ends_are_reliable() const {
    return !has_axioms() && !has_conditional_effects();
}


void HMHeuristic::initialize() {
    cout << "Using h^" << m << "." << endl;
    cout << "The implementation of the h^m heuristic is preliminary." << endl
         << "It is SLOOOOOOOOOOOW." << endl
         << "Please do not use this for comparison!" << endl;
    generate_all_tuples();
}


int HMHeuristic::compute_heuristic(const GlobalState &state) {
    if (test_goal(state)) {
        return 0;
    } else {
        Tuple s_tup;
        state_to_tuple(state, s_tup);

        init_hm_table(s_tup);
        update_hm_table();
        //dump_table();

        int h = eval(g_goal);

        if (h == numeric_limits<int>::max())
            return DEAD_END;
        return h;
    }
}


void HMHeuristic::init_hm_table(Tuple &t) {
    map<Tuple, int>::iterator it;
    for (it = hm_table.begin(); it != hm_table.end(); ++it) {
        pair<Tuple, int> hm_ent = *it;
        Tuple &tup = hm_ent.first;
        int h_val = check_tuple_in_tuple(tup, t);
        hm_table[tup] = h_val;
    }
}


void HMHeuristic::update_hm_table() {
    int round = 0;
    do {
        ++round;
        was_updated = false;

        for (size_t op_id = 0; op_id < g_operators.size(); ++op_id) {
            const GlobalOperator &op = g_operators[op_id];
            Tuple pre;
            get_operator_pre(op, pre);

            int c1 = eval(pre);
            if (c1 != numeric_limits<int>::max()) {
                Tuple eff;
                vector<Tuple> partial_eff;
                get_operator_eff(op, eff);
                generate_all_partial_tuples(eff, partial_eff);
                for (size_t i = 0; i < partial_eff.size(); ++i) {
                    update_hm_entry(partial_eff[i], c1 + get_adjusted_cost(op));

                    int eff_size = partial_eff[i].size();
                    if (eff_size < m) {
                        extend_tuple(partial_eff[i], op);
                    }
                }
            }
        }
    } while (was_updated);
}


void HMHeuristic::extend_tuple(Tuple &t, const GlobalOperator &op) {
    map<Tuple, int>::const_iterator it;
    for (it = hm_table.begin(); it != hm_table.end(); ++it) {
        pair<Tuple, int> hm_ent = *it;
        Tuple &entry = hm_ent.first;
        bool contradict = false;
        for (size_t i = 0; i < entry.size(); ++i) {
            if (contradict_effect_of(op, entry[i].first, entry[i].second)) {
                contradict = true;
                break;
            }
        }
        if (!contradict && (entry.size() > t.size()) && (check_tuple_in_tuple(t, entry) == 0)) {
            Tuple pre;
            get_operator_pre(op, pre);

            Tuple others;
            for (size_t i = 0; i < entry.size(); ++i) {
                pair<int, int> fact = entry[i];
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
            for (size_t i = 0; i < pre.size(); ++i) {
                if (vars.count(pre[i].first) != 0) {
                    is_valid = false;
                    break;
                }
                vars.insert(pre[i].first);
            }

            if (is_valid) {
                int c2 = eval(pre);
                if (c2 != numeric_limits<int>::max()) {
                    update_hm_entry(entry, c2 + get_adjusted_cost(op));
                }
            }
        }
    }
}


int HMHeuristic::eval(Tuple &t) const {
    vector<Tuple> partial;
    generate_all_partial_tuples(t, partial);
    int max = 0;
    for (size_t i = 0; i < partial.size(); ++i) {
        assert(hm_table.count(partial[i]) == 1);

        int h = hm_table.find(partial[i])->second; // C++11: use "at"
        if (h > max) {
            max = h;
        }
    }
    return max;
}


int HMHeuristic::update_hm_entry(Tuple &t, int val) {
    assert(hm_table.count(t) == 1);
    if (hm_table[t] > val) {
        hm_table[t] = val;
        was_updated = true;
    }
    return val;
}


int HMHeuristic::check_tuple_in_tuple(
    const Tuple &tup, const Tuple &big_tuple) const {
    for (size_t i = 0; i < tup.size(); ++i) {
        bool found = false;
        for (size_t j = 0; j < big_tuple.size(); ++j) {
            if (tup[i] == big_tuple[j]) {
                found = true;
                break;
            }
        }
        if (!found) {
            return numeric_limits<int>::max();
        }
    }
    return 0;
}


void HMHeuristic::state_to_tuple(const GlobalState &state, Tuple &t) const {
    for (size_t i = 0; i < g_variable_domain.size(); ++i)
        t.push_back(make_pair(i, state[i]));
}


int HMHeuristic::get_operator_pre_value(
    const GlobalOperator &op, int var) const {
    for (size_t i = 0; i < op.get_preconditions().size(); ++i) {
        if (op.get_preconditions()[i].var == var)
            return op.get_preconditions()[i].val;
    }
    return -1;
}


void HMHeuristic::get_operator_pre(const GlobalOperator &op, Tuple &t) const {
    for (size_t i = 0; i < op.get_preconditions().size(); ++i)
        t.push_back(make_pair(op.get_preconditions()[i].var,
                              op.get_preconditions()[i].val));
    sort(t.begin(), t.end());
}


void HMHeuristic::get_operator_eff(const GlobalOperator &op, Tuple &t) const {
    for (size_t i = 0; i < op.get_effects().size(); ++i)
        t.push_back(make_pair(op.get_effects()[i].var,
                              op.get_effects()[i].val));
    sort(t.begin(), t.end());
}


bool HMHeuristic::is_pre_of(const GlobalOperator &op, int var) const {
    // TODO if preconditions will be always sorted we should use a log-n
    // search instead
    for (size_t j = 0; j < op.get_preconditions().size(); ++j) {
        if (op.get_preconditions()[j].var == var) {
            return true;
        }
    }
    return false;
}


bool HMHeuristic::is_effect_of(const GlobalOperator &op, int var) const {
    for (size_t j = 0; j < op.get_effects().size(); ++j) {
        if (op.get_effects()[j].var == var) {
            return true;
        }
    }
    return false;
}


bool HMHeuristic::contradict_effect_of(
    const GlobalOperator &op, int var, int val) const {
    for (size_t j = 0; j < op.get_effects().size(); ++j) {
        if (op.get_effects()[j].var == var && op.get_effects()[j].val != val) {
            return true;
        }
    }
    return false;
}


void HMHeuristic::generate_all_tuples() {
    Tuple t;
    generate_all_tuples_aux(0, m, t);
}


void HMHeuristic::generate_all_tuples_aux(int var, int sz, Tuple &base) {
    int num_variables = g_variable_domain.size();
    for (int i = var; i < num_variables; ++i) {
        for (int j = 0; j < g_variable_domain[i]; ++j) {
            Tuple tup(base);
            tup.push_back(make_pair(i, j));
            hm_table[tup] = 0;
            if (sz > 1) {
                generate_all_tuples_aux(i + 1, sz - 1, tup);
            }
        }
    }
}


void HMHeuristic::generate_all_partial_tuples(
    Tuple &base_tuple, vector<Tuple> &res) const {
    Tuple t;
    generate_all_partial_tuples_aux(base_tuple, t, 0, m, res);
}


void HMHeuristic::generate_all_partial_tuples_aux(
    Tuple &base_tuple, Tuple &t, int index, int sz, vector<Tuple> &res) const {
    if (sz == 1) {
        for (size_t i = index; i < base_tuple.size(); ++i) {
            Tuple tup(t);
            tup.push_back(base_tuple[i]);
            res.push_back(tup);
        }
    } else {
        for (size_t i = index; i < base_tuple.size(); ++i) {
            Tuple tup(t);
            tup.push_back(base_tuple[i]);
            res.push_back(tup);
            generate_all_partial_tuples_aux(base_tuple, tup, i + 1, sz - 1, res);
        }
    }
}


void HMHeuristic::dump_table() const {
    map<Tuple, int>::const_iterator it;
    for (it = hm_table.begin(); it != hm_table.end(); ++it) {
        pair<Tuple, int> hm_ent = *it;
        cout << "h[";
        dump_tuple(hm_ent.first);
        cout << "] = " << hm_ent.second << endl;
    }
}


void HMHeuristic::dump_tuple(Tuple &tup) const {
    cout << tup[0].first << "=" << tup[0].second;
    for (size_t i = 1; i < tup.size(); ++i)
        cout << "," << tup[i].first << "=" << tup[i].second;
}


static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("h^m heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "ignored");
    parser.document_language_support("axioms", "ignored");
    parser.document_property("admissible",
                             "yes for tasks without conditional "
                             "effects or axioms");
    parser.document_property("consistent",
                             "yes for tasks without conditional "
                             "effects or axioms");

    parser.document_property("safe",
                             "yes for tasks without conditional "
                             "effects or axioms");
    parser.document_property("preferred operators", "no");

    parser.add_option<int>("m", "subset size", "2", Bounds("1", "infinity"));
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new HMHeuristic(opts);
}


static Plugin<Heuristic> _plugin("hm", _parse);
