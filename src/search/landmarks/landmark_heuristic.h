#ifndef LANDMARKS_LANDMARK_HEURISTIC_H
#define LANDMARKS_LANDMARK_HEURISTIC_H

# include "../heuristic.h"

class BitsetView;

namespace successor_generator {
class SuccessorGenerator;
}

namespace landmarks {
class LandmarkGraph;
class LandmarkNode;
class LandmarkStatusManager;

class LandmarkHeuristic : public Heuristic {
public:
    explicit LandmarkHeuristic(const plugins::Options &opts);
};
}

#endif
