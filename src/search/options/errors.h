#ifndef OPTIONS_ERRORS_H
#define OPTIONS_ERRORS_H

#include "parse_tree.h"

#include "../utils/exceptions.h"

#include <string>


#define ABORT_WITH_DEMANGLING_HINT(msg, type_name) \
    ( \
        (std::cerr << "Critical error in file " << __FILE__ \
                   << ", line " << __LINE__ << ": " << std::endl \
                   << (msg) << std::endl), \
        (std::cerr << options::get_demangling_hint(type_name) << std::endl), \
        (abort()), \
        (void)0 \
    )

namespace options {
class OptionParserError : public utils::Exception {
    std::string msg;
public:
    explicit OptionParserError(const std::string &msg);

    virtual void print() const override;
};


class ParseError : public utils::Exception {
    std::string msg;
    ParseTree parse_tree;
    std::string substring;
public:
    ParseError(const std::string &error, const ParseTree &parse_tree,
               const std::string &substring = "");

    virtual void print() const override;
};

extern std::string get_demangling_hint(const std::string &type_name);
}

#endif
