#include "landmark_heuristic.h"

#include "landmark.h"
#include "landmark_factory.h"
#include "landmark_status_manager.h"

#include "../plugins/plugin.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../tasks/cost_adapted_task.h"
#include "../tasks/root_task.h"

using namespace std;

namespace landmarks {
LandmarkHeuristic::LandmarkHeuristic(const plugins::Options &opts)
    : Heuristic(opts) {
}
}
