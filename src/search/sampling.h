#ifndef SAMPLING_H
#define SAMPLING_H

#include <exception>
#include <functional>
#include <vector>

class CountdownTimer;
class State;
class TaskProxy;


struct SamplingTimeout : public std::exception {};

double get_average_operator_cost(TaskProxy task_proxy);

std::vector<State> sample_states_with_random_walks(
    TaskProxy task_proxy,
    int num_samples,
    int init_h,
    double average_operator_cost,
    std::function<bool(State)> is_dead_end,
    const CountdownTimer &timer);

#endif
