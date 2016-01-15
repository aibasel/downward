#ifndef OPTIONS_SYNERGY_H
#define OPTIONS_SYNERGY_H

#include <vector>

class Heuristic;

namespace options {
class Synergy {
public:
    std::vector<Heuristic *> heuristics;
};
}

#endif
