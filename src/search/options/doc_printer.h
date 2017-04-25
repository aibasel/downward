#ifndef OPTIONS_DOC_PRINTER_H
#define OPTIONS_DOC_PRINTER_H

#include <iostream>
#include <string>

namespace options {
struct DocStruct;

class DocPrinter {
protected:
    std::ostream &os;

    virtual void print_synopsis(const DocStruct &info) = 0;
    virtual void print_usage(const std::string &call_name, const DocStruct &info) = 0;
    virtual void print_arguments(const DocStruct &info) = 0;
    virtual void print_notes(const DocStruct &info) = 0;
    virtual void print_language_features(const DocStruct &info) = 0;
    virtual void print_properties(const DocStruct &info) = 0;
    virtual void print_category_header(const std::string &category_name) = 0;
    virtual void print_category_footer() = 0;

public:
    explicit DocPrinter(std::ostream &out);
    virtual ~DocPrinter();

    virtual void print_all();
    virtual void print_category(const std::string &category_name);
    virtual void print_element(const std::string &call_name, const DocStruct &info);
};


class Txt2TagsPrinter : public DocPrinter {
protected:
    virtual void print_synopsis(const DocStruct &info);
    virtual void print_usage(const std::string &call_name, const DocStruct &info);
    virtual void print_arguments(const DocStruct &info);
    virtual void print_notes(const DocStruct &info);
    virtual void print_language_features(const DocStruct &info);
    virtual void print_properties(const DocStruct &info);
    virtual void print_category_header(const std::string &category_name);
    virtual void print_category_footer();

public:
    explicit Txt2TagsPrinter(std::ostream &out);
};


class PlainPrinter : public DocPrinter {
    // If this is false, notes, properties and language_features are omitted.
    bool print_all;

protected:
    virtual void print_synopsis(const DocStruct &info);
    virtual void print_usage(const std::string &call_name, const DocStruct &info);
    virtual void print_arguments(const DocStruct &info);
    virtual void print_notes(const DocStruct &info);
    virtual void print_language_features(const DocStruct &info);
    virtual void print_properties(const DocStruct &info);
    virtual void print_category_header(const std::string &category_name);
    virtual void print_category_footer();

public:
    PlainPrinter(std::ostream &out, bool print_all = false);
};
}

#endif
