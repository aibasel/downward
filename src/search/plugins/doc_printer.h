#ifndef PLUGINS_DOC_PRINTER_H
#define PLUGINS_DOC_PRINTER_H

#include "registry.h"

#include <iostream>
#include <string>
#include <vector>

namespace plugins {
class Feature;
class Registry;

class DocPrinter {
    virtual void print_category(const FeatureType &type) const;
    virtual void print_subcategory(const std::string &subcategory_name,
                                   const std::vector<const Feature *> &plugins) const;
    virtual void print_feature(const Feature &plugin) const;

protected:
    std::ostream &os;
    const Registry &registry;

    virtual void print_synopsis(const Feature &plugin) const = 0;
    virtual void print_usage(const Feature &plugin) const = 0;
    virtual void print_arguments(const Feature &plugin) const = 0;
    virtual void print_notes(const Feature &plugin) const = 0;
    virtual void print_language_features(const Feature &plugin) const = 0;
    virtual void print_properties(const Feature &plugin) const = 0;
    virtual void print_category_header(const std::string &category_name) const = 0;
    virtual void print_category_synopsis(const std::string &synopsis,
                                         bool supports_variable_binding) const = 0;
    virtual void print_category_footer() const = 0;

public:
    DocPrinter(std::ostream &out, Registry &registry);
    virtual ~DocPrinter() = default;

    void print_all() const;
    void print_feature(const std::string &name) const;
};


class Txt2TagsPrinter : public DocPrinter {
protected:
    virtual void print_synopsis(const Feature &plugin) const override;
    virtual void print_usage(const Feature &plugin) const override;
    virtual void print_arguments(const Feature &plugin) const override;
    virtual void print_notes(const Feature &plugin) const override;
    virtual void print_language_features(const Feature &plugin) const override;
    virtual void print_properties(const Feature &plugin) const override;
    virtual void print_category_header(const std::string &category_name) const override;
    virtual void print_category_synopsis(const std::string &synopsis,
                                         bool supports_variable_binding) const override;
    virtual void print_category_footer() const override;

public:
    Txt2TagsPrinter(std::ostream &out, Registry &registry);
};


class PlainPrinter : public DocPrinter {
    // If this is false, notes, properties and language_features are omitted.
    bool print_all;

protected:
    virtual void print_synopsis(const Feature &plugin) const override;
    virtual void print_usage(const Feature &plugin) const override;
    virtual void print_arguments(const Feature &plugin) const override;
    virtual void print_notes(const Feature &plugin) const override;
    virtual void print_language_features(const Feature &plugin) const override;
    virtual void print_properties(const Feature &plugin) const override;
    virtual void print_category_header(const std::string &category_name) const override;
    virtual void print_category_synopsis(const std::string &synopsis,
                                         bool supports_variable_binding) const override;
    virtual void print_category_footer() const override;

public:
    PlainPrinter(std::ostream &out, Registry &registry, bool print_all = false);
};
}

#endif
