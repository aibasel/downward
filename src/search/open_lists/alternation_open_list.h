#ifndef OPEN_LISTS_ALTERNATION_OPEN_LIST_H
#define OPEN_LISTS_ALTERNATION_OPEN_LIST_H

#include "open_list.h"
#include "../evaluator.h"
#include "../plugin.h"

#include <vector>

class Options;
class OptionParser;

template<class Entry>
class AlternationOpenList : public OpenList<Entry> {
    std::vector<OpenList<Entry> *> open_lists;
    std::vector<int> priorities;

    // roughly speaking, boosting is how often the boosted queue should be
    // preferred when removing an entry
    int boosting;
    int last_used_list;

protected:
    Evaluator *get_evaluator() {return this; }

public:
    AlternationOpenList(const Options &opts);
    AlternationOpenList(const std::vector<OpenList<Entry> *> &sublists,
                        int boost_influence);
    virtual ~AlternationOpenList() override;

    // OpenList interface
    virtual void insert(EvaluationContext &eval_context,
                        const Entry &entry) override;
    virtual Entry remove_min(std::vector<int> *key = 0) override;
    virtual bool empty() const override;
    virtual void clear() override;

    // Evaluator interface
    virtual void evaluate(int g, bool preferred) override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override;

    virtual void boost_preferred() override;

    static OpenList<Entry> *_parse(OptionParser &parser);
};

#include "alternation_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
