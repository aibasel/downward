#ifndef POR_NULL_POR_METHOD_H
#define POR_NULL_POR_METHOD_H

#include "../por_method.h"

class GlobalOperator;
class GlobalState;

namespace null_por_method {
class NullPORMethod : public PORMethod {
public:
    virtual void initialize();
    virtual void prune_operators(const GlobalState & /*state*/,
                                 std::vector<const GlobalOperator *> & /*ops*/) {}
    virtual void print_statistics() const {}
};
}

#endif
