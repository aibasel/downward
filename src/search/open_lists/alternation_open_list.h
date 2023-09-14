#ifndef OPEN_LISTS_ALTERNATION_OPEN_LIST_H
#define OPEN_LISTS_ALTERNATION_OPEN_LIST_H

#include "../open_list_factory.h"
#include "../plugins/options.h"
#include "../evaluator.h"

namespace alternation_open_list {

template<class Entry>
class AlternationOpenList : public OpenList<Entry> {
    std::vector<std::unique_ptr<OpenList<Entry>>> open_lists;
    std::vector<int> priorities;

    const int boost_amount;
    int size;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit AlternationOpenList(const plugins::Options &opts);
    explicit AlternationOpenList(
            int boost_amount,
            std::vector<std::shared_ptr<OpenListFactory>> open_list_factories);
    virtual ~AlternationOpenList() override = default;

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void boost_preferred() override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
    virtual bool is_dead_end(
            EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
            EvaluationContext &eval_context) const override;

};

template<class Entry>
AlternationOpenList<Entry>::AlternationOpenList(const plugins::Options &opts)
        : boost_amount(opts.get<int>("boost")),
          size(0) {
    std::vector<std::shared_ptr<OpenListFactory>> open_list_factories(
            opts.get_list<std::shared_ptr<OpenListFactory>>("sublists"));
    open_lists.reserve(open_list_factories.size());
    for (const auto &factory : open_list_factories)
        open_lists.push_back(factory->create_open_list<Entry>());

    priorities.resize(open_lists.size(), 0);
}


template<class Entry>
AlternationOpenList<Entry>::AlternationOpenList(int boost_amount,
                                                std::vector<std::shared_ptr<OpenListFactory>> open_list_factories)
        : boost_amount(boost_amount), size(0) {

        open_lists.reserve(open_list_factories.size());
        for (const auto &factory : open_list_factories)
            open_lists.push_back(factory->create_open_list<Entry>());

        priorities.resize(open_lists.size(), 0);
}


template<class Entry>
class TaskIndependentAlternationOpenList: public TaskIndependentOpenList<Entry> {
private:
    int  boost_amount;
    std::vector<std::shared_ptr<TaskIndependentOpenListFactory>> open_list_factories;
public:
    explicit TaskIndependentAlternationOpenList(int  boost_amount,
                                                std::vector<std::shared_ptr<TaskIndependentOpenListFactory>> open_list_factories);
    virtual plugins::Any create_task_specific(std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map) override;


    virtual ~TaskIndependentAlternationOpenList()  override = default;
};

template<class Entry>
TaskIndependentAlternationOpenList<Entry>::TaskIndependentAlternationOpenList(int  boost_amount,
                                                                              std::vector<std::shared_ptr<TaskIndependentOpenListFactory>> open_list_factories)
        : boost_amount(boost_amount),
          open_list_factories(open_list_factories){
}

template<class Entry>
plugins::Any TaskIndependentAlternationOpenList<Entry>::create_task_specific(std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map) {
    std::shared_ptr<AlternationOpenList<Entry>> task_specific_alternation_open_list;
    plugins::Any any_obj;
    if (component_map -> contains_key(make_pair(task, static_cast<void*>(this)))){
        utils::g_log << "Reuse task specific AlternationOpenList..." << std::endl;
        any_obj = component_map -> get_value(make_pair(task, static_cast<void*>(this)));
    } else {
        utils::g_log << "Creating task specific AlternationOpenList..." << std::endl;
        std::vector<std::shared_ptr<OpenListFactory>> td_list_factories(open_list_factories.size());
        transform( open_list_factories.begin(), open_list_factories.end(), td_list_factories.begin(),
                   [this, &task, &component_map](const std::shared_ptr<TaskIndependentOpenListFactory>& factory) {
                       return plugins::any_cast<std::shared_ptr<OpenListFactory>>(factory->create_task_independent_open_list<Entry>()->create_task_specific(task, component_map));
                   }
        );

        auto task_specific_alternation_open_list = std::make_shared<AlternationOpenList<Entry>>( boost_amount, td_list_factories);
        any_obj = plugins::Any(task_specific_alternation_open_list);
        component_map -> add_entry(make_pair(task, static_cast<void*>(this)), any_obj);
    }
    return any_obj;
}


class AlternationOpenListFactory : public OpenListFactory {
    plugins::Options options;
    int boost_amount;
    int size;
    std::vector<std::shared_ptr<OpenListFactory>> open_list_factories;
public:
    explicit AlternationOpenListFactory(const plugins::Options &opts);
    explicit AlternationOpenListFactory(int boost_amount, std::vector<std::shared_ptr<OpenListFactory>> open_list_factories);

    virtual ~AlternationOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};

class TaskIndependentAlternationOpenListFactory : public TaskIndependentOpenListFactory {
    plugins::Options options; //TODO issue559 remove options field in the long run.
    int  boost_amount;
    int size;
    std::vector<std::shared_ptr<TaskIndependentOpenListFactory>> open_list_factories;
public:
    explicit TaskIndependentAlternationOpenListFactory(const plugins::Options &opts);
    explicit TaskIndependentAlternationOpenListFactory(
            int boost_amount,
            std::vector<std::shared_ptr<TaskIndependentOpenListFactory>> open_list_factories);
    virtual ~TaskIndependentAlternationOpenListFactory() override = default;

    virtual std::unique_ptr<TaskIndependentStateOpenList> create_task_independent_state_open_list() override;
    virtual std::unique_ptr<TaskIndependentEdgeOpenList> create_task_independent_edge_open_list() override;
};




}

#endif
