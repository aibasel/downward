#ifndef SEARCH_SPACE_H
#define SEARCH_SPACE_H

#include "global_state.h"
#include "operator_cost.h"
#include "per_state_information.h"
#include "search_node_info.h"

#include <vector>

class GlobalState;
class OperatorProxy;
class TaskProxy;


class SearchNode {
    const StateRegistry &state_registry;
    StateID state_id;
    SearchNodeInfo &info;
public:
    SearchNode(const StateRegistry &state_registry,
               StateID state_id,
               SearchNodeInfo &info);

    StateID get_state_id() const {
        return state_id;
    }
    GlobalState get_state() const;

    bool is_new() const;
    bool is_open() const;
    bool is_closed() const;
    bool is_dead_end() const;

    int get_g() const;
    int get_real_g() const;

    void open_initial();
    void open(const SearchNode &parent_node,
              const OperatorProxy &parent_op,
              int adjusted_cost);
    void reopen(const SearchNode &parent_node,
                const OperatorProxy &parent_op,
                int adjusted_cost);
    void update_parent(const SearchNode &parent_node,
                       const OperatorProxy &parent_op,
                       int adjusted_cost);
    void close();
    void mark_as_dead_end();

    void dump(const TaskProxy &task_proxy) const;
};


class SearchSpace {
    PerStateInformation<SearchNodeInfo> search_node_infos;

    StateRegistry &state_registry;
public:
    explicit SearchSpace(StateRegistry &state_registry);

    SearchNode get_node(const GlobalState &state);
    void trace_path(const GlobalState &goal_state,
                    std::vector<OperatorID> &path) const;

    void dump(const TaskProxy &task_proxy) const;
    void print_statistics() const;
};

#endif
