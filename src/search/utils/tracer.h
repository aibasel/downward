#ifndef UTILS_TRACER_H
#define UTILS_TRACER_H

#include <string>


class TraceBlock {
    std::string block_name;
public:
    explicit TraceBlock(const std::string &block_name);
    ~TraceBlock();
};

extern void trace(const std::string &msg = "");

#endif
