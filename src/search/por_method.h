#ifndef POR_METHOD_H
#define POR_METHOD_H

#include <string>
#include <vector>

class GlobalOperator;
class OptionParser;
class GlobalState;

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

#endif
