#ifndef SUCCESSOR_GENERATOR_H
#define SUCCESSOR_GENERATOR_H

#include <iostream>
#include <vector>

class GlobalOperator;
class GlobalState;

class GlobalSuccessorGenerator {
public:
    virtual ~GlobalSuccessorGenerator() {}
    virtual void generate_applicable_ops(const GlobalState &curr,
                                         std::vector<const GlobalOperator *> &ops) = 0;
    void dump() {_dump("  "); }
    virtual void _dump(std::string indent) = 0;
};

GlobalSuccessorGenerator *read_successor_generator(std::istream &in);

#endif
