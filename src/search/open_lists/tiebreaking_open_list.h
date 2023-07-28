#ifndef OPEN_LISTS_TIEBREAKING_OPEN_LIST_H
#define OPEN_LISTS_TIEBREAKING_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"


#include <deque>
#include <map>

namespace tiebreaking_open_list {
template<class Entry>
class TieBreakingOpenList : public OpenList<Entry> {
    using Bucket = std::deque<Entry>;

    std::map<const std::vector<int>, Bucket> buckets;
    int size;

    std::vector<std::shared_ptr<Evaluator>> evaluators;
    /*
      If allow_unsafe_pruning is true, we ignore (don't insert) states
      which the first evaluator considers a dead end, even if it is
      not a safe heuristic.
    */
    bool allow_unsafe_pruning;

    int dimension() const;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit TieBreakingOpenList(const plugins::Options &opts);
    explicit TieBreakingOpenList(
            bool pref_only,
            std::vector<std::shared_ptr<Evaluator>> evaluators,
            bool allow_unsafe_pruning);
    virtual ~TieBreakingOpenList() override = default;

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
    virtual bool is_dead_end(
            EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
            EvaluationContext &eval_context) const override;
};




class TieBreakingOpenListFactory : public OpenListFactory {
    plugins::Options options; //TODOissue559 remove options field in the long run.
    bool pref_only;
    int size;
    std::vector<std::shared_ptr<Evaluator>> evaluators;
    bool allow_unsafe_pruning;
public:
    explicit TieBreakingOpenListFactory(const plugins::Options &options);
    explicit TieBreakingOpenListFactory(
        bool pref_only,
        std::vector<std::shared_ptr<Evaluator>> evaluators,
        bool allow_unsafe_pruning);
    virtual ~TieBreakingOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif
