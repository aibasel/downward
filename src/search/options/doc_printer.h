#ifndef OPTIONS_DOC_PRINTER_H
#define OPTIONS_DOC_PRINTER_H

#include <iostream>
#include <string>

namespace options {
struct DocStruct;
struct PluginTypeDoc;

class DocPrinter {
    virtual void print_category(const PluginTypeDoc &plugin_type_doc);
    virtual void print_element(const std::string &call_name, const DocStruct &info);

protected:
    std::ostream &os;

    virtual void print_synopsis(const DocStruct &info) = 0;
    virtual void print_usage(const std::string &call_name, const DocStruct &info) = 0;
    virtual void print_arguments(const DocStruct &info) = 0;
    virtual void print_notes(const DocStruct &info) = 0;
    virtual void print_language_features(const DocStruct &info) = 0;
    virtual void print_properties(const DocStruct &info) = 0;
    virtual void print_category_header(const std::string &category_name) = 0;
    virtual void print_category_synopsis(const std::string &synopsis) = 0;
    virtual void print_category_footer() = 0;

public:
    explicit DocPrinter(std::ostream &out);
    virtual ~DocPrinter();

    virtual void print_all();
};


class Txt2TagsPrinter : public DocPrinter {
protected:
    virtual void print_synopsis(const DocStruct &info) override;
    virtual void print_usage(const std::string &call_name, const DocStruct &info) override;
    virtual void print_arguments(const DocStruct &info) override;
    virtual void print_notes(const DocStruct &info) override;
    virtual void print_language_features(const DocStruct &info) override;
    virtual void print_properties(const DocStruct &info) override;
    virtual void print_category_header(const std::string &category_name) override;
    virtual void print_category_synopsis(const std::string &synopsis) override;
    virtual void print_category_footer() override;

public:
    explicit Txt2TagsPrinter(std::ostream &out);
};


class PlainPrinter : public DocPrinter {
    // If this is false, notes, properties and language_features are omitted.
    bool print_all;

protected:
    virtual void print_synopsis(const DocStruct &info) override;
    virtual void print_usage(const std::string &call_name, const DocStruct &info) override;
    virtual void print_arguments(const DocStruct &info) override;
    virtual void print_notes(const DocStruct &info) override;
    virtual void print_language_features(const DocStruct &info) override;
    virtual void print_properties(const DocStruct &info) override;
    virtual void print_category_header(const std::string &category_name) override;
    virtual void print_category_synopsis(const std::string &synopsis) override;
    virtual void print_category_footer() override;

public:
    // TODO: Change back to false.
    PlainPrinter(std::ostream &out, bool print_all = true);
};
}

#endif
