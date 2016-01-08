#ifndef POR_METHOD_H
#define POR_METHOD_H

#include <vector>

class GlobalOperator;
class GlobalState;

class PORMethod {
public:
    virtual void prune_operators(const GlobalState &state,
                                 std::vector<const GlobalOperator *> &ops) = 0;
    virtual void dump_options() const = 0;
    virtual void print_statistics() const = 0;
    virtual void initialize() = 0;
};

#endif
