#ifndef OPEN_LISTS_ALTERNATION_OPEN_LIST_H
#define OPEN_LISTS_ALTERNATION_OPEN_LIST_H

#include "../evaluator.h"
#include "../open_list_factory.h"

#include "../plugins/options.h"

namespace alternation_open_list {
template<class Entry>
class AlternationOpenList : public OpenList<Entry> {
    std::vector<std::shared_ptr<OpenList<Entry>>> open_lists;
    std::vector<int> priorities;

    const int boost_amount;
    int size;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit AlternationOpenList(std::vector<std::shared_ptr<OpenListFactory>> open_list_factories,
                                 int boost_amount);
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
AlternationOpenList<Entry>::AlternationOpenList(std::vector<std::shared_ptr<OpenListFactory>> open_list_factories,
                                                int boost_amount)
    : boost_amount(boost_amount), size(0) {
    open_lists.reserve(open_list_factories.size());
    for (const auto &factory : open_list_factories)
        open_lists.push_back(factory->create_open_list<Entry>());

    priorities.resize(open_lists.size(), 0);
}


class AlternationOpenListFactory : public OpenListFactory {
    int boost_amount;
    int size;
    std::vector<std::shared_ptr<OpenListFactory>> open_list_factories;
public:
    explicit AlternationOpenListFactory(std::vector<std::shared_ptr<OpenListFactory>> open_list_factories,
                                        int boost_amount);

    virtual ~AlternationOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};

class TaskIndependentAlternationOpenListFactory : public TaskIndependentOpenListFactory {
    int boost_amount;
    int size;
    std::vector<std::shared_ptr<TaskIndependentOpenListFactory>> open_list_factories;
protected:
    std::string get_product_name() const override {return "AlternationOpenListFactory";}
public:
    explicit TaskIndependentAlternationOpenListFactory(
        std::vector<std::shared_ptr<TaskIndependentOpenListFactory>> open_list_factories,
        int boost_amount);
    virtual ~TaskIndependentAlternationOpenListFactory() override = default;

    using AbstractProduct = OpenListFactory;
    using ConcreteProduct = AlternationOpenListFactory;

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
