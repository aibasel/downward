#ifndef OPEN_LISTS_TIEBREAKING_OPEN_LIST_H
#define OPEN_LISTS_TIEBREAKING_OPEN_LIST_H

#include "open_list.h"

#include "open_list_factory.h"

#include "../option_parser_util.h"

#include <deque>
#include <map>
#include <utility>
#include <vector>

class OptionParser;
class Options;
class ScalarEvaluator;


template<class Entry>
class TieBreakingOpenList : public OpenList<Entry> {
    using Bucket = std::deque<Entry>;

    std::map<const std::vector<int>, Bucket> buckets;
    int size;

    std::vector<ScalarEvaluator *> evaluators;
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
    explicit TieBreakingOpenList(const Options &opts);
    virtual ~TieBreakingOpenList() override = default;

    virtual Entry remove_min(std::vector<int> *key = 0) override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
};



class TieBreakingOpenListFactory : public OpenListFactory {
    Options options;
public:
    explicit TieBreakingOpenListFactory(const Options &options);
    virtual ~TieBreakingOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};


#include "tiebreaking_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
