#ifndef SAMPLING_H
#define SAMPLING_H

#include <exception>
#include <functional>
#include <vector>

class State;
class SuccessorGenerator;
class TaskProxy;

namespace Utils {
class CountdownTimer;
}


struct SamplingTimeout : public std::exception {};

/*
  Perform 'num_samples' random walks with biomially distributed walk
  lenghts. Whenever a dead end is detected or a state has no
  successors, restart from the initial state. The function
  'is_dead_end' should return whether a given state is a dead end. If
  omitted, no dead end detection is performed. If 'timer' is given the
  sampling procedure will run for at most the specified time limit and
  possibly return less than 'num_samples' states.
*/
std::vector<State> sample_states_with_random_walks(
    TaskProxy task_proxy,
    const SuccessorGenerator &successor_generator,
    int num_samples,
    int init_h,
    double average_operator_cost,
    std::function<bool(State)> is_dead_end = [] (const State &) {
                                                 return false;
                                             },
    const Utils::CountdownTimer *timer = nullptr);

#endif
