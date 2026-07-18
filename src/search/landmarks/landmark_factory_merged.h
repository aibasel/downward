#ifndef LANDMARKS_LANDMARK_FACTORY_MERGED_H
#define LANDMARKS_LANDMARK_FACTORY_MERGED_H

#include "landmark_factory.h"

#include <vector>

namespace landmarks {
class LandmarkFactoryMerged : public LandmarkFactory {
    std::vector<std::shared_ptr<LandmarkFactory>> landmark_factories;

    std::vector<std::shared_ptr<LandmarkGraph>>
    generate_landmark_graphs_of_subfactories(
        const std::shared_ptr<AbstractTask> &task);
    void add_atomic_landmarks(const std::vector<std::shared_ptr<LandmarkGraph>>
                                  &landmark_graphs) const;
    void add_disjunctive_landmarks(
        const std::vector<std::shared_ptr<LandmarkGraph>> &landmark_graphs)
        const;
    void add_landmark_orderings(
        const std::vector<std::shared_ptr<LandmarkGraph>> &landmark_graphs)
        const;
    virtual void generate_landmarks(
        const std::shared_ptr<AbstractTask> &task) override;
    void postprocess();
    LandmarkNode *get_matching_landmark(const Landmark &landmark) const;
public:
    LandmarkFactoryMerged(
        const std::shared_ptr<AbstractTask> &task,
        const std::vector<std::shared_ptr<LandmarkFactory>> &lm_factories,
        utils::Verbosity verbosity);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
