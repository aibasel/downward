#include "causal_graph.h"

#include "operator.h"
#include "utilities.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <tr1/unordered_set>

using namespace std;
using namespace std::tr1;


/*
  An IntRelationBuilder constructs an IntRelation by adding one pair
  to the relation at a time. Duplicates are automatically handled
  (i.e., it is OK to add the same pair twice), and the pairs need not
  be added in any specific sorted order.

  Define the following parameters:
  - K: range of the IntRelation (i.e., allowed values {0, ..., K - 1})
  - M: number of pairs added to the relation (including duplicates)
  - N: number of unique pairs in the final relation
  - D: maximal number of unique elements (x, y) in the relation for given x

  Then we get:
  - O(K + N) memory usage during construction and for final IntRelation
  - O(K + M + N log D) construction time
*/

class IntRelationBuilder {
    typedef unordered_set<int> IntSet;
    vector<IntSet> int_sets;

    int get_range() const;
public:
    explicit IntRelationBuilder(int range);
    ~IntRelationBuilder();

    void add_pair(int u, int v);
    void compute_relation(IntRelation &result) const;
};


IntRelationBuilder::IntRelationBuilder(int range)
    : int_sets(range) {
}


IntRelationBuilder::~IntRelationBuilder() {
}


int IntRelationBuilder::get_range() const {
    return int_sets.size();
}


void IntRelationBuilder::add_pair(int u, int v) {
    assert(u >= 0 && u < get_range());
    assert(v >= 0 && v < get_range());
    int_sets[u].insert(v);
}


void IntRelationBuilder::compute_relation(IntRelation &result) const {
    int range = get_range();
    result.clear();
    result.resize(range);
    for (size_t i = 0; i < range; ++i) {
        result[i].assign(int_sets[i].begin(), int_sets[i].end());
        sort(result[i].begin(), result[i].end());
    }
}


struct CausalGraphBuilder {
    IntRelationBuilder pre_eff_builder;
    IntRelationBuilder eff_pre_builder;
    IntRelationBuilder eff_eff_builder;

    IntRelationBuilder succ_builder;
    IntRelationBuilder pred_builder;

    explicit CausalGraphBuilder(int var_count)
        : pre_eff_builder(var_count),
          eff_pre_builder(var_count),
          eff_eff_builder(var_count),
          succ_builder(var_count),
          pred_builder(var_count) {
    }

    ~CausalGraphBuilder() {
    }

    void handle_pre_eff_arc(int u, int v) {
        assert(u != v);
        pre_eff_builder.add_pair(u, v);
        succ_builder.add_pair(u, v);
        eff_pre_builder.add_pair(v, u);
        pred_builder.add_pair(v, u);
    }

    void handle_eff_eff_edge(int u, int v) {
        assert(u != v);
        eff_eff_builder.add_pair(u, v);
        eff_eff_builder.add_pair(v, u);
        succ_builder.add_pair(u, v);
        succ_builder.add_pair(v, u);
        pred_builder.add_pair(u, v);
        pred_builder.add_pair(v, u);
    }

    void handle_operator(const Operator &op) {
        const vector<Prevail> &prevail = op.get_prevail();
        const vector<PrePost> &pre_post = op.get_pre_post();

        // Handle pre->eff links from prevail conditions.
        for (size_t i = 0; i < prevail.size(); ++i) {
            int pre_var = prevail[i].var;
            for (size_t j = 0; j < pre_post.size(); ++j) {
                int eff_var = pre_post[j].var;
                if (pre_var != eff_var)
                    handle_pre_eff_arc(pre_var, eff_var);
            }
        }

        // Handle pre->eff links from preconditions inside PrePost.
        for (size_t i = 0; i < pre_post.size(); ++i) {
            if (pre_post[i].pre == -1)
                continue;
            int pre_var = pre_post[i].var;
            for (size_t j = 0; j < pre_post.size(); ++j) {
                int eff_var = pre_post[j].var;
                if (pre_var != eff_var)
                    handle_pre_eff_arc(pre_var, eff_var);
            }
        }

        // Handle eff->eff links.
        for (size_t i = 0; i < pre_post.size(); ++i) {
            int eff1_var = pre_post[i].var;
            for (size_t j = i + 1; j < pre_post.size(); ++j) {
                int eff2_var = pre_post[j].var;
                if (eff1_var != eff2_var)
                    handle_eff_eff_edge(eff1_var, eff2_var);
            }
        }
    }
};


CausalGraph::CausalGraph() {
    CausalGraphBuilder cg_builder(g_variable_domain.size());

    for (size_t i = 0; i < g_operators.size(); ++i)
        cg_builder.handle_operator(g_operators[i]);

    for (size_t i = 0; i < g_axioms.size(); ++i)
        cg_builder.handle_operator(g_axioms[i]);

    cg_builder.pre_eff_builder.compute_relation(pre_to_eff);
    cg_builder.eff_pre_builder.compute_relation(eff_to_pre);
    cg_builder.eff_eff_builder.compute_relation(eff_to_eff);

    cg_builder.pred_builder.compute_relation(predecessors);
    cg_builder.succ_builder.compute_relation(successors);

    // dump();
}


CausalGraph::~CausalGraph() {
}


void CausalGraph::dump() const {
    cout << "Causal graph: " << endl;
    for (size_t var = 0; var < g_variable_domain.size(); ++var)
        cout << "#" << var << " [" << g_variable_name[var] << "]:" << endl
             << "    pre->eff arcs: " << pre_to_eff[var] << endl
             << "    eff->pre arcs: " << eff_to_pre[var] << endl
             << "    eff->eff arcs: " << eff_to_eff[var] << endl
             << "    successors: " << successors[var] << endl
             << "    predecessors: " << predecessors[var] << endl;
}
