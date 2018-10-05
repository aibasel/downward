#ifndef OPTIONS_ERRORS_H
#define OPTIONS_ERRORS_H

#include "parse_tree.h"

#include "../utils/system.h"

#include <ostream>
#include <string>

#define DEMANGLING_HINT(type_name) \
    (std::cerr << "To retrieve the demangled C++ type for gcc/clang, you " \
               << "can call \nc++filt -t " << type_name << std::endl)

#define ABORT_WITH_DEMANGLING_HINT(msg, type_name) \
    ( \
        (std::cerr << "Critical error in file " << __FILE__ \
                   << ", line " << __LINE__ << ": " << std::endl \
                   << (msg) << std::endl), \
        (DEMANGLING_HINT(type_name)), \
        (abort()), \
        (void)0 \
    )

namespace options {
struct ArgError {
    std::string msg;

    ArgError(const std::string &msg);

    friend std::ostream &operator<<(std::ostream &out, const ArgError &err);
};


struct ParseError {
    std::string msg;
    ParseTree parse_tree;
    std::string substring;

    ParseError(const std::string &msg, ParseTree parse_tree);
    ParseError(
        const std::string &msg, const ParseTree &parse_tree, const std::string &substring);

    friend std::ostream &operator<<(std::ostream &out, const ParseError &parse_error);
};


NO_RETURN extern void exit_with_demangling_hint(
    utils::ExitCode returncode, const std::string &type_name);
}

#endif
