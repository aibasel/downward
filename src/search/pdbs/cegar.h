#ifndef PDBS_CEGAR_H
#define PDBS_CEGAR_H

#include "pattern_collection_information.h"
#include "pattern_database.h"

#include "../task_proxy.h"

#include "../utils/logging.h"

#include <memory>
#include <unordered_set>
#include <vector>

namespace options {
class OptionParser;
}

namespace utils {
class CountdownTimer;
class RandomNumberGenerator;
}

namespace pdbs {
/*
  This is used as a "collection entry" in the CEGAR algorithm. It stores
  the PDB (and with that, the pattern) and an optimal plan (in the wildcard
  format) for that PDB if it exists or unsolvable is true otherwise. It can
  be marked as "solved" to ignore it in further iterations of the CEGAR
  algorithm.
*/
class Projection {
    std::shared_ptr<PatternDatabase> pdb;
    std::vector<std::vector<OperatorID>> plan;
    bool unsolvable;
    bool solved;

public:
    Projection(
        const std::shared_ptr<PatternDatabase> &&pdb,
        const std::vector<std::vector<OperatorID>> &&plan,
        bool unsolvable)
        : pdb(move(pdb)),
          plan(move(plan)),
          unsolvable(unsolvable),
          solved(false) {}

    const std::shared_ptr<PatternDatabase> &get_pdb() const {
        return pdb;
    }

    const Pattern &get_pattern() const {
        return pdb->get_pattern();
    }

    const std::vector<std::vector<OperatorID>> &get_plan() const {
        return plan;
    }

    bool is_unsolvable() const {
        return unsolvable;
    }

    void mark_as_solved() {
        solved = true;
    }

    bool is_solved() {
        return solved;
    }
};

struct Flaw {
    int collection_index;
    int variable;

    Flaw(int collection_index, int variable)
        : collection_index(collection_index),
          variable(variable) {
    }
};

using FlawList = std::vector<Flaw>;

/*
  This class implements the CEGAR algorithm for computing pattern collections.

  In a nutshell, it receives a concrete task plus a (sub)set of its goals in a
  randomized order. Starting from the pattern collection consisting of a
  singleton pattern for each goal variable, it repeatedly attempts to execute
  an optimal plan of each pattern in the concrete task, collects reasons why
  this is not possible (so-called flaws) and refines the pattern in question
  by adding a variable to it.

  Further parameters allow blacklisting a (sub)set of the non-goal variables
  which are then never added to the collection, limiting PDB and collection
  size, setting a time limit and switching between computing regular or
  wildcard plans, where the latter are sequences of parallel operators
  inducing the same abstract transition.
*/
class CEGAR {
    const int max_pdb_size;
    const int max_collection_size;
    const bool wildcard_plans;
    const double max_time;
    const utils::Verbosity verbosity;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    const std::shared_ptr<AbstractTask> &task;
    const TaskProxy task_proxy;
    const std::vector<FactPair> goals;
    std::unordered_set<int> blacklisted_variables;

    std::vector<std::unique_ptr<Projection>> projection_collection;
    /*
      Map each variable of the task which is contained in the collection to the
      projection which it is part of.
    */
    std::unordered_map<int, int> variable_to_projection;
    int collection_size;

    void print_collection() const;
    bool time_limit_reached(const utils::CountdownTimer &timer) const;

    std::unique_ptr<Projection> compute_projection(Pattern &&pattern) const;
    void compute_initial_collection();

    /*
      Check if operator op is applicable in state, ignoring blacklisted
      variables. If it is, return an empty (flaw) list. Otherwise, return the
      violated preconditions.
    */
    FlawList get_violated_preconditions(
        int collection_index, const OperatorProxy &op, const State &state) const;
    /*
      Try to apply the plan of the projection at the given index in the
      concrete task starting at the given state. During application,
      blacklisted variables are ignored. If plan application succeeds,
      return an empty flaw list. Otherwise, return all precondition variables
      of all operators of the failing plan step. When the method returns,
      current is the last state reached when executing the plan.
     */
    FlawList apply_plan(int collection_index, State &current) const;
    /*
      Use apply_plan to generate flaws. Return true if there are no flaws and
      no blacklisted variables, in which case the concrete task is solved.
      Return false in all other cases. Append new flaws to the passed-in flaws.
    */
    bool get_flaws_for_projection(
        int collection_index, const State &concrete_init, FlawList &flaws);
    /*
      Use get_flaws_for_projection for all patterns of the collection. Append
      new flaws to the passed-in flaws. If the task is solved by the plan of
      any projection, return the index of the collection. Otherwise, return -1.
    */
    int get_flaws(const State &concrete_init, FlawList &flaws);

    // Methods related to refining.
    void add_pattern_for_var(int var);
    bool can_merge_patterns(int index1, int index2) const;
    void merge_patterns(int index1, int index2);
    bool can_add_variable_to_pattern(int index, int var) const;
    void add_variable_to_pattern(int collection_index, int var);
    void refine(const FlawList &flaws);
public:
    CEGAR(
        int max_pdb_size,
        int max_collection_size,
        bool wildcard_plans,
        double max_time,
        utils::Verbosity verbosity,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng,
        const std::shared_ptr<AbstractTask> &task,
        std::vector<FactPair> &&goals,
        std::unordered_set<int> &&blacklisted_variables = std::unordered_set<int>());
    PatternCollectionInformation compute_pattern_collection();
};

extern void add_cegar_options_to_parser(
    options::OptionParser &parser);
}

#endif
