#ifndef OPEN_LISTS_BEST_FIRST_OPEN_LIST_H
#define OPEN_LISTS_BEST_FIRST_OPEN_LIST_H

#include "../evaluator.h"
#include "../open_list.h"
#include "../open_list_factory.h"

#include "../plugins/options.h"
#include "../plugins/plugin.h"
#include "../utils/memory.h"

#include <cassert>
#include <deque>
#include <map>

/*
  Open list indexed by a single int, using FIFO tie-breaking.

  Implemented as a map from int to deques.
*/

namespace standard_scalar_open_list {
template<class Entry>
class BestFirstOpenList : public OpenList<Entry> {
    typedef std::deque<Entry> Bucket;

    std::map<int, Bucket> buckets;
    int size;

    std::shared_ptr<Evaluator> evaluator;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    BestFirstOpenList(const std::shared_ptr<Evaluator> &eval, bool preferred_only);
    
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
BestFirstOpenList<Entry>::BestFirstOpenList(
    const std::shared_ptr<Evaluator> &evaluator, bool preferred_only)
    : OpenList<Entry>(preferred_only),
      size(0),
      evaluator(evaluator) {
}


class BestFirstOpenListFactory : public OpenListFactory {
    std::shared_ptr<Evaluator> eval;
    bool pref_only;
public:
    BestFirstOpenListFactory(
        const std::shared_ptr<Evaluator> &eval, bool pref_only);

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};


class TaskIndependentBestFirstOpenListFactory : public TaskIndependentOpenListFactory {
    std::shared_ptr<TaskIndependentEvaluator> evaluator;
    bool pref_only;
    int size;
protected:
    std::string get_product_name() const override {return "BestFirstOpenListFactory";}
public:
    TaskIndependentBestFirstOpenListFactory(
        std::shared_ptr<TaskIndependentEvaluator> evaluator,
        bool pref_only);
    virtual ~TaskIndependentBestFirstOpenListFactory() override = default;

    using AbstractProduct = OpenListFactory;
    using ConcreteProduct = BestFirstOpenListFactory;


    std::shared_ptr<AbstractProduct>
    get_task_specific(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map,
                      int depth = -1) const override;

    std::shared_ptr<ConcreteProduct> create_ts(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const;
};
}

#endif
