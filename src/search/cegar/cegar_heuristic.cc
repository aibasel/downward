#include "./cegar_heuristic.h"

#include "./../globals.h"
#include "./../operator.h"
#include "./../option_parser.h"
#include "./../plugin.h"
#include "./../state.h"

#include <limits>
#include <utility>
using namespace std;

namespace cegar_heuristic {

int get_eff(Operator op, int var) {
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        PrePost pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.post;
    }
    return -2;
}

int get_pre(Operator op, int var) {
    for (int i = 0; i < op.get_prevail().size(); ++i) {
        Prevail prevail = op.get_prevail()[i];
        if (prevail.var == var)
            // if op.pre[v] not in s1_vals:
            return prevail.prev;
    }
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        PrePost pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            // if op.pre[v] not in s1_vals:
            return pre_post.pre;
    }
    return -2;
}

AbstractState AbstractState::regress(Operator op) {
    AbstractState abs_state = AbstractState();

    for (int v = 0; v < g_variable_domain.size(); ++v) {
        set<int> s1_vals;
        // s2_vals = s2[v]
        set<int> s2_vals = values[v];
        // if v occurs in op.eff:
        int eff = get_eff(op, v);
        if (eff != -2) {
            // if op.eff[v] not in s2_vals:
            if (s2_vals.count(eff) == 0) {
                // return regression_empty
                // TODO
                return abs_state;
            }
            // s1_vals = v.all_values_in_domain
            for (int i = 0; i < g_variable_domain[v]; ++i)
                s1_vals.insert(i);
        } else {
            // s1_vals = s2_vals
            s1_vals = s2_vals;
        }
        // if v occurs in op.pre:
        int pre = get_pre(op, v);
        if (pre != -2) {
            // if op.pre[v] not in s1_vals:
            if (s1_vals.count(pre) == 0) {
                // return regression_empty
                // TODO
                return abs_state;
            }
            // s1_vals = [op.pre[v]]
            s1_vals.clear();
            s1_vals.insert(pre);
        }
        abs_state.values[v] = s1_vals;
    }
    return abs_state;
}

CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts) {
    min_operator_cost = numeric_limits<int>::max();
    for (int i = 0; i < g_operators.size(); ++i)
        min_operator_cost = min(min_operator_cost,
                                get_adjusted_cost(g_operators[i]));
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
}

int CegarHeuristic::compute_heuristic(const State &state) {
    if (test_goal(state))
        return 0;
    else
        return min_operator_cost;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar", _parse);

}
