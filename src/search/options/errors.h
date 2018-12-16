#ifndef OPTIONS_ERRORS_H
#define OPTIONS_ERRORS_H

#include "parse_tree.h"

#include <exception>
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
class OptionParserError : public std::exception {
    std::string msg;
public:
    explicit OptionParserError(const std::string &msg);

    virtual const char *what() const noexcept override;
};


class ParseError : public std::exception {
    std::string msg;
public:
    ParseError(const std::string &error, const ParseTree &parse_tree,
               const std::string &substring = "");

    virtual const char *what() const noexcept override;
};

extern std::string get_demangling_hint(const std::string &type_name);
}

#endif
