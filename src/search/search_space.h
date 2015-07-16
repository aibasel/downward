#ifndef SEARCH_SPACE_H
#define SEARCH_SPACE_H

#include "global_state.h"
#include "operator_cost.h"
#include "per_state_information.h"
#include "search_node_info.h"

#include <vector>

class GlobalOperator;
class GlobalState;


class SearchNode {
    StateID state_id;
    SearchNodeInfo &info;
    OperatorCost cost_type;
public:
    SearchNode(StateID state_id_, SearchNodeInfo &info_,
               OperatorCost cost_type_);

    StateID get_state_id() const {
        return state_id;
    }
    GlobalState get_state() const;

    bool is_new() const;
    bool is_open() const;
    bool is_closed() const;
    bool is_dead_end() const;

    bool is_h_dirty() const;
    void set_h_dirty();
    void clear_h_dirty();
    int get_g() const;
    int get_real_g() const;
    int get_h() const;

    void open_initial(int h);
    void open(int h, const SearchNode &parent_node,
              const GlobalOperator *parent_op);
    void reopen(const SearchNode &parent_node,
                const GlobalOperator *parent_op);
    void update_parent(const SearchNode &parent_node,
                       const GlobalOperator *parent_op);
    void increase_h(int h);
    void close();
    void mark_as_dead_end();

    void dump() const;
};


class SearchSpace {
    PerStateInformation<SearchNodeInfo> search_node_infos;

    OperatorCost cost_type;
public:
    SearchSpace(OperatorCost cost_type_);
    SearchNode get_node(const GlobalState &state);
    void trace_path(const GlobalState &goal_state,
                    std::vector<const GlobalOperator *> &path) const;

    void dump() const;
    void print_statistics() const;
};

#endif
