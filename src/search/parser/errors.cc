#include "errors.h"

#include "../utils/strings.h"

using namespace std;

namespace parser {
TokenStreamError::TokenStreamError(const string &msg)
    : utils::Exception(msg) {
}

void Traceback::push(const string &layer) {
    stack.push_back(layer);
}

void Traceback::pop() {
    stack.pop_back();
}

string Traceback::str() const {
    string message = "Traceback:\n";
    if (stack.empty()) {
        message += "\tEmpty";
    } else {
        message += "\t" + utils::join(stack, "\n\t-> ");
    }
    return message;
}

ParserError::ParserError(const string &msg, const Traceback &traceback)
    : utils::Exception(traceback.str() + "\n" + msg) {
}
}
