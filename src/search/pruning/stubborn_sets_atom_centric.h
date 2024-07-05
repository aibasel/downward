#ifndef PRUNING_STUBBORN_SETS_ATOM_CENTRIC_H
#define PRUNING_STUBBORN_SETS_ATOM_CENTRIC_H

#include "stubborn_sets.h"

namespace stubborn_sets_atom_centric {
static const int MARKED_VALUES_NONE = -2;
static const int MARKED_VALUES_ALL = -1;

// See the .cc file for an explanation of the strategies.
enum class AtomSelectionStrategy {
    FAST_DOWNWARD,
    QUICK_SKIP,
    STATIC_SMALL,
    DYNAMIC_SMALL
};

class StubbornSetsAtomCentric : public stubborn_sets::StubbornSets {
    const bool use_sibling_shortcut;
    const AtomSelectionStrategy atom_selection_strategy;

    // consumers[v][d] contains the ID of operator o if pre(o) contains the fact v=d.
    std::vector<std::vector<std::vector<int>>> consumers;
    /*
      Marked producer and consumer facts.
      marked_{producers,consumers}[v][d] is true iff fact v=d is marked.
    */
    std::vector<std::vector<bool>> marked_producers;
    std::vector<std::vector<bool>> marked_consumers;
    /*
      Data structures for shortcut handling of siblings.
      marked_*_variables[v] = d iff all sibling facts v=d' with d'!=d are marked
      marked_*_variables[v] = MARKED_VALUES_ALL iff all facts for v are marked
      marked_*_variables[v] = MARKED_VALUES_NONE iff we have no such information
    */
    std::vector<int> marked_producer_variables;
    std::vector<int> marked_consumer_variables;

    std::vector<FactPair> producer_queue;
    std::vector<FactPair> consumer_queue;

    void compute_consumers(const TaskProxy &task_proxy);
    bool operator_is_applicable(int op, const State &state) const;
    void enqueue_producers(const FactPair &fact);
    void enqueue_consumers(const FactPair &fact);
    void enqueue_sibling_consumers(const FactPair &fact);
    void enqueue_sibling_producers(const FactPair &fact);
    FactPair select_fact(const std::vector<FactPair> &facts, const State &state) const;
    void enqueue_nes(int op, const State &state);
    void enqueue_interferers(int op);
    void handle_stubborn_operator(const State &state, int op);
    virtual void compute_stubborn_set(const State &state) override;
public:
    explicit StubbornSetsAtomCentric(
        bool use_sibling_shortcut,
        AtomSelectionStrategy atom_selection_strategy,
        utils::Verbosity verbosity);
    virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
