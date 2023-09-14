#ifndef OPEN_LISTS_BEST_FIRST_OPEN_LIST_H
#define OPEN_LISTS_BEST_FIRST_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/options.h"
#include "../evaluator.h"

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
    explicit BestFirstOpenList(const plugins::Options &opts);
    explicit BestFirstOpenList(bool pref_only, std::shared_ptr<Evaluator> evaluator);

    BestFirstOpenList(const std::shared_ptr<Evaluator> &eval, bool preferred_only);
    virtual ~BestFirstOpenList() override = default;

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
BestFirstOpenList<Entry>::BestFirstOpenList(const plugins::Options &opts)
        : OpenList<Entry>(opts.get<bool>("pref_only")),
          size(0),
          evaluator(opts.get<std::shared_ptr<Evaluator>>("eval")) {
}



template<class Entry>
BestFirstOpenList<Entry>::BestFirstOpenList(bool pref_only, std::shared_ptr<Evaluator> evaluator)
        : OpenList<Entry>(pref_only),
          size(0),
          evaluator(evaluator) {
}



template<class Entry>
class TaskIndependentBestFirstOpenList: public TaskIndependentOpenList<Entry> {
private:
    bool pref_only;
    int size;
    std::shared_ptr<TaskIndependentEvaluator> evaluator;
public:
    explicit TaskIndependentBestFirstOpenList(
            bool pref_only,
    std::shared_ptr<TaskIndependentEvaluator> evaluator);
    virtual plugins::Any create_task_specific(std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map) override;


    virtual ~TaskIndependentBestFirstOpenList()  override = default;
};

template<class Entry>
TaskIndependentBestFirstOpenList<Entry>::TaskIndependentBestFirstOpenList(bool pref_only,
                                                                          std::shared_ptr<TaskIndependentEvaluator> evaluator)
        : pref_only(pref_only),
        size(0),
          evaluator(evaluator){
}

template<class Entry>
plugins::Any TaskIndependentBestFirstOpenList<Entry>::create_task_specific(std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map) {
    std::shared_ptr<BestFirstOpenList<Entry>> task_specific_best_first_open_list;
    plugins::Any any_obj;
    if (component_map -> contains_key(make_pair(task, static_cast<void*>(this)))){
        utils::g_log << "Reuse task specific BestFirstOpenList..." << std::endl;
        any_obj = component_map -> get_value(make_pair(task, static_cast<void*>(this)));
    } else {
        utils::g_log << "Creating task specific BestFirstOpenList..." << std::endl;

        auto task_specific_best_first_open_list = std::make_shared<BestFirstOpenList<Entry>>( pref_only, plugins::any_cast<std::shared_ptr<Evaluator>>(evaluator->create_task_specific(task, component_map)));
        any_obj = plugins::Any(task_specific_best_first_open_list);
        component_map -> add_entry(make_pair(task, static_cast<void*>(this)), any_obj);
    }
    return any_obj;
}



class BestFirstOpenListFactory : public OpenListFactory {
    plugins::Options options;
    bool pref_only;
    int size;
    std::shared_ptr<Evaluator> evaluator;
public:
    explicit BestFirstOpenListFactory(const plugins::Options &options);
    explicit BestFirstOpenListFactory(bool pref_only, std::shared_ptr<Evaluator> evaluator);
    virtual ~BestFirstOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};




class TaskIndependentBestFirstOpenListFactory : public TaskIndependentOpenListFactory {
    plugins::Options options; //TODO issue559 remove options field in the long run.
    int size;
    bool pref_only;
    std::shared_ptr<TaskIndependentEvaluator> evaluator;
public:
    explicit TaskIndependentBestFirstOpenListFactory(const plugins::Options &opts);
    explicit TaskIndependentBestFirstOpenListFactory(
            bool pref_only,
            std::shared_ptr<TaskIndependentEvaluator> evaluator);
    virtual ~TaskIndependentBestFirstOpenListFactory() override = default;

    virtual std::unique_ptr<TaskIndependentStateOpenList> create_task_independent_state_open_list() override;
    virtual std::unique_ptr<TaskIndependentEdgeOpenList> create_task_independent_edge_open_list() override;
};

}

#endif
