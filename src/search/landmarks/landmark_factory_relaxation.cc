#include "landmark_factory_relaxation.h"

#include "exploration.h"

using namespace std;

namespace landmarks {
void LandmarkFactoryRelaxation::generate_landmarks(const std::shared_ptr<AbstractTask>& task) {
    TaskProxy task_proxy(*task);
    Exploration exploration(task_proxy);
    generate_landmarks(task, exploration);
}
}