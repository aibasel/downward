#ifndef OPEN_LISTS_TIEBREAKING_OPEN_LIST_H
#define OPEN_LISTS_TIEBREAKING_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"

#include "../evaluator.h"

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

template<class Entry>
TieBreakingOpenList<Entry>::TieBreakingOpenList(const plugins::Options &opts)
    : OpenList<Entry>(opts.get<bool>("pref_only")),
      size(0), evaluators(opts.get_list<std::shared_ptr<Evaluator>>("evals")),
      allow_unsafe_pruning(opts.get<bool>("unsafe_pruning")) {
}

template<class Entry>
TieBreakingOpenList<Entry>::TieBreakingOpenList(bool pref_only,
                                                std::vector<std::shared_ptr<Evaluator>> evaluators,
                                                bool allow_unsafe_pruning)
    : OpenList<Entry>(pref_only),
      size(0), evaluators(evaluators),
      allow_unsafe_pruning(allow_unsafe_pruning) {
}

template<class Entry>
void TieBreakingOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    std::vector<int> key;
    key.reserve(evaluators.size());
    for (const std::shared_ptr<Evaluator> &evaluator : evaluators)
        key.push_back(eval_context.get_evaluator_value_or_infinity(evaluator.get()));

    buckets[key].push_back(entry);
    ++size;
}

template<class Entry>
Entry TieBreakingOpenList<Entry>::remove_min() {
    assert(size > 0);
    typename std::map<const std::vector<int>, Bucket>::iterator it;
    it = buckets.begin();
    assert(it != buckets.end());
    assert(!it->second.empty());
    --size;
    Entry result = it->second.front();
    it->second.pop_front();
    if (it->second.empty())
        buckets.erase(it);
    return result;
}

template<class Entry>
bool TieBreakingOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void TieBreakingOpenList<Entry>::clear() {
    buckets.clear();
    size = 0;
}

template<class Entry>
int TieBreakingOpenList<Entry>::dimension() const {
    return evaluators.size();
}

template<class Entry>
void TieBreakingOpenList<Entry>::get_path_dependent_evaluators(
    std::set<Evaluator *> &evals) {
    for (const std::shared_ptr<Evaluator> &evaluator : evaluators)
        evaluator->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool TieBreakingOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    // TODO: Properly document this behaviour.
    // If one safe heuristic detects a dead end, return true.
    if (is_reliable_dead_end(eval_context))
        return true;
    // If the first heuristic detects a dead-end and we allow "unsafe
    // pruning", return true.
    if (allow_unsafe_pruning &&
        eval_context.is_evaluator_value_infinite(evaluators[0].get()))
        return true;
    // Otherwise, return true if all heuristics agree this is a dead-end.
    for (const std::shared_ptr<Evaluator> &evaluator : evaluators)
        if (!eval_context.is_evaluator_value_infinite(evaluator.get()))
            return false;
    return true;
}

template<class Entry>
bool TieBreakingOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (const std::shared_ptr<Evaluator> &evaluator : evaluators)
        if (eval_context.is_evaluator_value_infinite(evaluator.get()) &&
            evaluator->dead_ends_are_reliable())
            return true;
    return false;
}

template<class Entry>
class TaskIndependentTieBreakingOpenList : public TaskIndependentOpenList<Entry> {
private:
    bool pref_only;
    std::vector<std::shared_ptr<TaskIndependentEvaluator>> evaluators;
    /*
      If allow_unsafe_pruning is true, we ignore (don't insert) states
      which the first evaluator considers a dead end, even if it is
      not a safe heuristic.
    */
    bool allow_unsafe_pruning;
public:
    explicit TaskIndependentTieBreakingOpenList(bool pref_only,
                                                std::vector<std::shared_ptr<TaskIndependentEvaluator>> evaluators,
                                                bool allow_unsafe_pruning);
   virtual plugins::Any create_task_specific(std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map) override;

    virtual ~TaskIndependentTieBreakingOpenList()  override = default;
};

template<class Entry>
TaskIndependentTieBreakingOpenList<Entry>::TaskIndependentTieBreakingOpenList(bool preferred_only,
                                                                              std::vector<std::shared_ptr<TaskIndependentEvaluator>> evaluators,
                                                                              bool allow_unsafe_pruning)
    : TaskIndependentOpenList<Entry>(preferred_only),
      pref_only(preferred_only), evaluators(evaluators), allow_unsafe_pruning(allow_unsafe_pruning) {
}

template<class Entry>
plugins::Any TaskIndependentTieBreakingOpenList<Entry>::create_task_specific(std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map) {
    std::shared_ptr<TieBreakingOpenList<Entry>> task_specific_tie_breaking_open_list;
    plugins::Any any_obj;
    if (component_map -> contains_key(make_pair(task, static_cast<void*>(this)))){
        utils::g_log << "Reuse task specific TieBreakingOpenList..." << std::endl;
        any_obj = component_map -> get_value(make_pair(task, static_cast<void*>(this)));
    } else {
        utils::g_log << "Creating task specific TieBreakingOpenList..." << std::endl;
        std::vector<std::shared_ptr<Evaluator>> td_evaluators(evaluators.size());
        transform(evaluators.begin(), evaluators.end(), td_evaluators.begin(),
                  [this, &task, &component_map](const std::shared_ptr<TaskIndependentEvaluator> &eval) {
                      return plugins::any_cast<std::shared_ptr<Evaluator>>(eval->create_task_specific(task, component_map));
                  }
        );

        task_specific_tie_breaking_open_list = std::make_shared<TieBreakingOpenList<Entry>>(pref_only,
                                                                                   td_evaluators,
                                                                                   allow_unsafe_pruning);
        any_obj = plugins::Any(task_specific_tie_breaking_open_list);
        component_map -> add_entry(make_pair(task, static_cast<void*>(this)), any_obj);
    }
    return any_obj;
}



class TieBreakingOpenListFactory : public OpenListFactory {
    plugins::Options options; //TODO issue559 remove options field in the long run.
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
class TaskIndependentTieBreakingOpenListFactory : public TaskIndependentOpenListFactory {
    plugins::Options options; //TODO issue559 remove options field in the long run.
    bool pref_only;
    int size;
    std::vector<std::shared_ptr<TaskIndependentEvaluator>> evaluators;
    bool allow_unsafe_pruning;
public:
    explicit TaskIndependentTieBreakingOpenListFactory(const plugins::Options &opts);
    explicit TaskIndependentTieBreakingOpenListFactory(
        bool pref_only,
        std::vector<std::shared_ptr<TaskIndependentEvaluator>> evaluators,
        bool allow_unsafe_pruning);
    virtual ~TaskIndependentTieBreakingOpenListFactory() override = default;

    virtual std::unique_ptr<TaskIndependentStateOpenList> create_task_independent_state_open_list() override;
    virtual std::unique_ptr<TaskIndependentEdgeOpenList> create_task_independent_edge_open_list() override;
};
}

#endif
