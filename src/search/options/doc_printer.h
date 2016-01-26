#ifndef OPTIONS_DOC_PRINTER_H
#define OPTIONS_DOC_PRINTER_H

#include <iosfwd>
#include <string>

namespace options {
struct DocStruct;

class DocPrinter {
public:
    DocPrinter(std::ostream &out);
    virtual ~DocPrinter();

    virtual void print_all();
    virtual void print_category(std::string category_name);
    virtual void print_element(std::string call_name, const DocStruct &info);
protected:
    std::ostream &os;
    virtual void print_synopsis(const DocStruct &info) = 0;
    virtual void print_usage(std::string call_name, const DocStruct &info) = 0;
    virtual void print_arguments(const DocStruct &info) = 0;
    virtual void print_notes(const DocStruct &info) = 0;
    virtual void print_language_features(const DocStruct &info) = 0;
    virtual void print_properties(const DocStruct &info) = 0;
    virtual void print_category_header(std::string category_name) = 0;
    virtual void print_category_footer() = 0;
};

class Txt2TagsPrinter : public DocPrinter {
public:
    Txt2TagsPrinter(std::ostream &out);
    virtual ~Txt2TagsPrinter();
protected:
    virtual void print_synopsis(const DocStruct &info);
    virtual void print_usage(std::string call_name, const DocStruct &info);
    virtual void print_arguments(const DocStruct &info);
    virtual void print_notes(const DocStruct &info);
    virtual void print_language_features(const DocStruct &info);
    virtual void print_properties(const DocStruct &info);
    virtual void print_category_header(std::string category_name);
    virtual void print_category_footer();
};

class PlainPrinter : public DocPrinter {
public:
    PlainPrinter(std::ostream &out, bool print_all = false);
    virtual ~PlainPrinter();
protected:
    virtual void print_synopsis(const DocStruct &info);
    virtual void print_usage(std::string call_name, const DocStruct &info);
    virtual void print_arguments(const DocStruct &info);
    virtual void print_notes(const DocStruct &info);
    virtual void print_language_features(const DocStruct &info);
    virtual void print_properties(const DocStruct &info);
    virtual void print_category_header(std::string category_name);
    virtual void print_category_footer();
private:
    bool print_all; //if this is false, notes, properties and language_features are omitted
};
}

#endif
