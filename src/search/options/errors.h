#ifndef OPTIONS_ERRORS_H
#define OPTIONS_ERRORS_H

#include "parse_tree.h"

#include <exception>
#include <ostream> // TODO: Remove.
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


struct ParseError {
    std::string msg;
    ParseTree parse_tree;
    std::string substring;

    ParseError(const std::string &msg, ParseTree parse_tree);
    ParseError(const std::string &msg, const ParseTree &parse_tree,
               const std::string &substring);

    friend std::ostream &operator<<(std::ostream &out,
                                    const ParseError &parse_error);
};

extern std::string get_demangling_hint(const std::string &type_name);
}

#endif
