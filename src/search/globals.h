#ifndef GLOBALS_H
#define GLOBALS_H

#include "operator_id.h"

#include <memory>
#include <string>
#include <vector>

class AbstractTask;
class AxiomEvaluator;
class TaskProxy;

namespace int_packer {
class IntPacker;
}

namespace successor_generator {
class SuccessorGenerator;
}

namespace utils {
struct Log;
}

/*
  Set generates_multiple_plan_files to true if the planner can find more than
  one plan and should number the plans as FILENAME.1, ..., FILENAME.n.
*/
void save_plan(const std::vector<OperatorID> &plan,
               const TaskProxy &task_proxy,
               bool generates_multiple_plan_files = false);
int calculate_plan_cost(const std::vector<OperatorID> &plan, const TaskProxy &task_proxy);

void read_everything(std::istream &in);
// TODO: move this to task_utils or a new file with dump methods for all proxy objects.
void dump_everything();

// The following function is deprecated. Use task_properties.h instead.
bool is_unit_cost();

extern bool g_use_metric;

extern int_packer::IntPacker *g_state_packer;
extern AxiomEvaluator *g_axiom_evaluator;
extern successor_generator::SuccessorGenerator *g_successor_generator;
extern std::string g_plan_filename;
extern int g_num_previously_generated_plans;
extern bool g_is_part_of_anytime_portfolio;

extern std::shared_ptr<AbstractTask> g_root_task;

extern utils::Log g_log;


/*
  This vector holds the initial values *before* the axioms have been evaluated.
  Use a state registry to obtain the real initial state.
*/
/*
  TODO: This variable is deprecated and will be removed soon.
  Currently, it is still needed by the search engine to create a registry.
  (The state data should come from the task instead.)
*/
extern std::vector<int> g_initial_state_data;

#endif
