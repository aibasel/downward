#ifndef OPTIONS_DOC_PRINTER_H
#define OPTIONS_DOC_PRINTER_H

#include <iostream>
#include <string>

namespace options {
struct PluginInfo;

class DocPrinter {
    virtual void print_category(const std::string &plugin_type_name, const std::string &synopsis);
    virtual void print_plugin(const std::string &name, const PluginInfo &info);

protected:
    std::ostream &os;

    virtual void print_synopsis(const PluginInfo &info) = 0;
    virtual void print_usage(const std::string &name, const PluginInfo &info) = 0;
    virtual void print_arguments(const PluginInfo &info) = 0;
    virtual void print_notes(const PluginInfo &info) = 0;
    virtual void print_language_features(const PluginInfo &info) = 0;
    virtual void print_properties(const PluginInfo &info) = 0;
    virtual void print_category_header(const std::string &category_name) = 0;
    virtual void print_category_synopsis(const std::string &synopsis) = 0;
    virtual void print_category_footer() = 0;

public:
    explicit DocPrinter(std::ostream &out);
    virtual ~DocPrinter();

    void print_all();
    void print_plugin(const std::string &name);
};


class Txt2TagsPrinter : public DocPrinter {
protected:
    virtual void print_synopsis(const PluginInfo &info) override;
    virtual void print_usage(const std::string &name, const PluginInfo &info) override;
    virtual void print_arguments(const PluginInfo &info) override;
    virtual void print_notes(const PluginInfo &info) override;
    virtual void print_language_features(const PluginInfo &info) override;
    virtual void print_properties(const PluginInfo &info) override;
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
    virtual void print_synopsis(const PluginInfo &info) override;
    virtual void print_usage(const std::string &name, const PluginInfo &info) override;
    virtual void print_arguments(const PluginInfo &info) override;
    virtual void print_notes(const PluginInfo &info) override;
    virtual void print_language_features(const PluginInfo &info) override;
    virtual void print_properties(const PluginInfo &info) override;
    virtual void print_category_header(const std::string &category_name) override;
    virtual void print_category_synopsis(const std::string &synopsis) override;
    virtual void print_category_footer() override;

public:
    PlainPrinter(std::ostream &out, bool print_all = false);
};
}

#endif
