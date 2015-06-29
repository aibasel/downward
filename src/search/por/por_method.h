#ifndef POR_POR_METHOD_H
#define POR_POR_METHOD_H

#include <string>
#include <vector>

class GlobalOperator;
class OptionParser;
class GlobalState;

namespace POR {
class PORMethod;

extern void add_parser_option(
    OptionParser &parser, const std::string &option_name);
extern PORMethod *create(int option);

enum PORMethodEnum {
    NO_POR_METHOD = 0,
    SIMPLE_STUBBORN_SETS = 1,
    SSS_EXPANSION_CORE = 2
};

class PORMethod {
public:
    PORMethod();
    virtual ~PORMethod();
    virtual void prune_operators(const GlobalState &state,
                                 std::vector<const GlobalOperator *> &ops) = 0;
    virtual void dump_options() const = 0;
    virtual void dump_statistics() const = 0;
};


class NullPORMethod : public PORMethod {
public:
    NullPORMethod();
    virtual ~NullPORMethod();
    virtual void prune_operators(const GlobalState & /*state*/,
                                 std::vector<const GlobalOperator *> & /*ops*/) {}
    virtual void dump_options() const;
    virtual void dump_statistics() const {}
};


class PORMethodWithStatistics : public PORMethod {
    long unpruned_successors_generated;
    long pruned_successors_generated;
protected:
    virtual void do_pruning(const GlobalState &state,
                            std::vector<const GlobalOperator *> &ops) = 0;
public:
    PORMethodWithStatistics();
    virtual ~PORMethodWithStatistics();

    virtual void prune_operators(const GlobalState &state,
                                 std::vector<const GlobalOperator *> &ops);
    virtual void dump_statistics() const;
};
}

#endif
