#include "doc_printer.h"

#include "plugin.h"

#include "../utils/strings.h"

#include <iostream>
#include <map>

using namespace std;

namespace plugins {
DocPrinter::DocPrinter(ostream &out, Registry &registry)
    : os(out),
      registry(registry) {
}

void DocPrinter::print_all() const {
    FeatureTypes feature_types = registry.get_feature_types();
    sort(feature_types.begin(), feature_types.end(),
         [](const FeatureType *t1, const FeatureType *t2) {
             return t1->name() < t2->name();
         }
         );
    for (const FeatureType *type : feature_types) {
        print_category(*type);
    }
}

void DocPrinter::print_feature(const string &name) const {
    print_feature(*registry.get_feature(name));
}

void DocPrinter::print_category(const FeatureType &type) const {
    print_category_header(type.name());
    print_category_synopsis(type.get_synopsis(), type.supports_variable_binding());
    map<string, vector<const Feature *>> subcategories;
    for (const shared_ptr<const Feature> &feature: registry.get_features()) {
        if (feature->get_type() == type) {
            subcategories[feature->get_subcategory()].push_back(feature.get());
        }
    }
    /*
      Note on sorting: Because we use a map keyed on the subcategory names,
      the subcategories are sorted by these names. For the time being, this
      seems as good as any other order. For the future, we might
      consider influencing the sort order by adding a sort priority
      item to SubcategoryPlugin.

      Note on empty subcategories: if a subcategory is not used (i.e., has no
      plug-ins inside it), then it does not appear in the
      documentation. This is intentional. For example, it means that
      we could introduce groups in "core code" that may or may not be
      used by plug-ins, and if they are not used, they do not clutter the
      documentation.
     */
    for (auto &pair: subcategories) {
        string subcategory_name = pair.first;
        vector<const Feature *> &features = pair.second;
        sort(features.begin(), features.end(),
             [](const Feature *p1, const Feature *p2) {
                 return p1->get_key() < p2->get_key();
             }
             );
        print_subcategory(subcategory_name, features);
    }
    print_category_footer();
}

void DocPrinter::print_subcategory(
    const string &subcategory_name, const vector<const Feature *> &features) const {
    if (!subcategory_name.empty()) {
        const SubcategoryPlugin &subcategory_plugin =
            registry.get_subcategory_plugin(subcategory_name);
        os << endl << "= " << subcategory_plugin.get_title() << " =" << endl;
        if (!subcategory_plugin.get_synopsis().empty()) {
            os << subcategory_plugin.get_synopsis() << endl;
        }
        os << endl;
    }
    for (const Feature *feature : features) {
        print_feature(*feature);
    }
}

void DocPrinter::print_feature(const Feature &feature) const {
    print_synopsis(feature);
    print_usage(feature);
    print_arguments(feature);
    print_notes(feature);
    print_language_features(feature);
    print_properties(feature);
}

Txt2TagsPrinter::Txt2TagsPrinter(ostream &out, Registry &registry)
    : DocPrinter(out, registry) {
}

void Txt2TagsPrinter::print_synopsis(const Feature &feature) const {
    string title = feature.get_title();
    if (title.empty()) {
        title = feature.get_key();
    }
    os << "== " << title << " ==" << endl;
    if (!feature.get_synopsis().empty())
        os << feature.get_synopsis() << endl;
}

void Txt2TagsPrinter::print_usage(const Feature &feature) const {
    if (!feature.get_key().empty()) {
        os << "``` " << feature.get_key() << "(";
        vector<string> argument_help_strings;
        for (const ArgumentInfo &arg_info : feature.get_arguments()) {
            string arg_help = arg_info.key;
            if (!arg_info.default_value.empty()) {
                arg_help += "=" + arg_info.default_value;
            }
            argument_help_strings.push_back(arg_help);
        }
        os << utils::join(argument_help_strings, ", ")
           << ")" << endl;
    }
}

void Txt2TagsPrinter::print_arguments(const Feature &feature) const {
    for (const ArgumentInfo &arg_info : feature.get_arguments()) {
        const Type &arg_type = arg_info.type;
        os << "- //" << arg_info.key << "// ("
           << arg_type.name();
        if (arg_info.bounds.has_bound())
            os << " \"\"" << arg_info.bounds << "\"\"";
        os << "): " << arg_info.help << endl;
        if (arg_type.is_enum_type()) {
            for (const pair<string, string> &explanation :
                 arg_type.get_documented_enum_values()) {
                os << "    - ``" << explanation.first << "``: "
                   << explanation.second << endl;
            }
        }
    }
    os << endl << endl;
}

void Txt2TagsPrinter::print_notes(const Feature &feature) const {
    for (const NoteInfo &note : feature.get_notes()) {
        if (note.long_text) {
            os << "=== " << note.name << " ===" << endl
               << note.description << endl << endl;
        } else {
            os << "**" << note.name << ":** " << note.description << endl << endl;
        }
    }
}

void Txt2TagsPrinter::print_language_features(const Feature &feature) const {
    if (!feature.get_language_support().empty()) {
        os << "Supported language features:" << endl;
        for (const LanguageSupportInfo &ls : feature.get_language_support()) {
            os << "- **" << ls.feature << ":** " << ls.description << endl;
        }
        os << endl << endl;
    }
}

void Txt2TagsPrinter::print_properties(const Feature &feature) const {
    if (!feature.get_properties().empty()) {
        os << "Properties:" << endl;
        for (const PropertyInfo &prop : feature.get_properties()) {
            os << "- **" << prop.property << ":** " << prop.description << endl;
        }
        os << endl << endl;
    }
}

void Txt2TagsPrinter::print_category_header(const string &category_name) const {
    os << ">>>>CATEGORY: " << category_name << "<<<<" << endl;
}

void Txt2TagsPrinter::print_category_synopsis(const string &synopsis,
                                              bool supports_variable_binding) const {
    if (!synopsis.empty()) {
        os << synopsis << endl;
    }
    if (supports_variable_binding) {
        os << endl << "This feature type can be bound to variables using "
           << "``let(variable_name, variable_definition, expression)"
           << "`` where ``expression`` can use ``variable_name``. "
           << "Predefinitions using ``--evaluator``, ``--heuristic``, and "
           << "``--landmarks`` are automatically transformed into ``let``-"
           << "expressions but are deprecated."
           << endl;
    }
    os << endl;
}

void Txt2TagsPrinter::print_category_footer() const {
    os << endl
       << ">>>>CATEGORYEND<<<<" << endl;
}

PlainPrinter::PlainPrinter(ostream &out, Registry &registry, bool print_all)
    : DocPrinter(out, registry),
      print_all(print_all) {
}

void PlainPrinter::print_synopsis(const Feature &feature) const {
    string title = feature.get_title();
    if (title.empty()) {
        title = feature.get_key();
    }
    os << "== " << title << " ==" << endl;
    if (print_all && !feature.get_synopsis().empty()) {
        os << feature.get_synopsis() << endl;
    }
}

void PlainPrinter::print_usage(const Feature &feature) const {
    if (!feature.get_key().empty()) {
        os << feature.get_key() << "(";
        vector<string> argument_help_strings;
        for (const ArgumentInfo &arg_info : feature.get_arguments()) {
            string arg_help = arg_info.key;
            if (!arg_info.default_value.empty()) {
                arg_help += "=" + arg_info.default_value;
            }
            argument_help_strings.push_back(arg_help);
        }
        os << utils::join(argument_help_strings, ", ") << ")" << endl;
    }
}

void PlainPrinter::print_arguments(const Feature &feature) const {
    for (const ArgumentInfo &arg_info : feature.get_arguments()) {
        os << " " << arg_info.key << " ("
           << arg_info.type.name();
        if (arg_info.bounds.has_bound())
            os << " " << arg_info.bounds;
        os << "): " << arg_info.help << endl;
        const Type &arg_type = arg_info.type;
        if (arg_type.is_enum_type()) {
            for (const pair<string, string> &explanation :
                 arg_type.get_documented_enum_values()) {
                os << " - " << explanation.first << ": "
                   << explanation.second << endl;
            }
        }
    }
}

void PlainPrinter::print_notes(const Feature &feature) const {
    if (print_all) {
        for (const NoteInfo &note : feature.get_notes()) {
            if (note.long_text) {
                os << "=== " << note.name << " ===" << endl
                   << note.description << endl << endl;
            } else {
                os << " * " << note.name << ": " << note.description << endl << endl;
            }
        }
    }
}

void PlainPrinter::print_language_features(const Feature &feature) const {
    if (print_all && !feature.get_language_support().empty()) {
        os << "Language features supported:" << endl;
        for (const LanguageSupportInfo &ls : feature.get_language_support()) {
            os << " * " << ls.feature << ": " << ls.description << endl;
        }
    }
}

void PlainPrinter::print_properties(const Feature &feature) const {
    if (print_all && !feature.get_properties().empty()) {
        os << "Properties:" << endl;
        for (const PropertyInfo &prop : feature.get_properties()) {
            os << " * " << prop.property << ": " << prop.description << endl;
        }
    }
}

void PlainPrinter::print_category_header(const string &category_name) const {
    os << "Help for " << category_name << endl << endl;
}

void PlainPrinter::print_category_synopsis(const string &synopsis,
                                           bool supports_variable_binding) const {
    if (print_all && !synopsis.empty()) {
        os << synopsis << endl;
    }
    if (supports_variable_binding) {
        os << endl << "This feature type can be bound to variables using "
           << "``let(variable_name, variable_definition, expression)"
           << "`` where ``expression`` can use ``variable_name``. "
           << "Predefinitions using ``--evaluator``, ``--heuristic``, and "
           << "``--landmarks`` are automatically transformed into ``let``-"
           << "expressions but are deprecated."
           << endl;
    }
}

void PlainPrinter::print_category_footer() const {
    os << endl;
}
}
