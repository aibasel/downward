#ifndef OPEN_LISTS_ALTERNATION_OPEN_LIST_H
#define OPEN_LISTS_ALTERNATION_OPEN_LIST_H

#include "open_list.h"

#include "open_list_factory.h"

#include "../option_parser_util.h"

#include <memory>
#include <vector>


template<class Entry>
class AlternationOpenList : public OpenList<Entry> {
    std::vector<std::unique_ptr<OpenList<Entry> > > open_lists;
    std::vector<int> priorities;

    const int boost_amount;
protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit AlternationOpenList(const Options &opts);
    AlternationOpenList(
        std::vector<std::unique_ptr<OpenList<Entry> > > &&sublists,
        int boost_amount);
    virtual ~AlternationOpenList() override = default;

    virtual Entry remove_min(std::vector<int> *key = 0) override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void boost_preferred() override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
};


class AlternationOpenListFactory : public OpenListFactory {
    Options options;
public:
    explicit AlternationOpenListFactory(const Options &options);
    virtual ~AlternationOpenListFactory() override = default;

    virtual std::unique_ptr<OpenList<StateID> > create_state_open_list() override;
};


#include "alternation_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
