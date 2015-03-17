#ifndef OPEN_LISTS_ALTERNATION_OPEN_LIST_H
#define OPEN_LISTS_ALTERNATION_OPEN_LIST_H

#include "open_list.h"

#include "../plugin.h"

#include <vector>

class Options;
class OptionParser;

template<class Entry>
class AlternationOpenList : public OpenList<Entry> {
    std::vector<OpenList<Entry> *> open_lists;
    std::vector<int> priorities;

    const int boost_amount;
protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit AlternationOpenList(const Options &opts);
    AlternationOpenList(const std::vector<OpenList<Entry> *> &sublists,
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

    static OpenList<Entry> *_parse(OptionParser &parser);
};

#include "alternation_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
