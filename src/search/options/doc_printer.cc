#include "doc_printer.h"

#include "doc_utils.h"
#include "registries.h"

#include "../utils/strings.h"

#include <iostream>
#include <map>

using namespace std;

namespace options {
static bool is_call(const string &s) {
    return s.find("(") != string::npos;
}

DocPrinter::DocPrinter(ostream &out, Registry &registry)
    : os(out),
      registry(registry) {
}

DocPrinter::~DocPrinter() {
}

void DocPrinter::print_all() {
    for (const PluginTypeInfo &info : registry.get_sorted_type_infos()) {
        print_category(info.type_name, info.documentation,
                       info.predefinition_key, info.alias);
    }
}

void DocPrinter::print_plugin(const string &name) {
    print_plugin(name, registry.get_plugin_info(name));
}

void DocPrinter::print_category(
    const string &plugin_type_name, const string &synopsis,
    const string &predefinition_key, const string &alias) {
    print_category_header(plugin_type_name);
    print_category_synopsis(synopsis);
    print_category_predefinitions(predefinition_key, alias);
    map<string, vector<PluginInfo>> groups;
    for (const string &key : registry.get_sorted_plugin_info_keys()) {
        const PluginInfo &info = registry.get_plugin_info(key);
        if (info.type_name == plugin_type_name && !info.hidden) {
            groups[info.group].push_back(info);
        }
    }
    /*
      Note on sorting: Because we use a map keyed on the group IDs,
      the sections are sorted by these IDs. For the time being, this
      seems as good as any other order. For the future, we might
      consider influencing the sort order by adding a sort priority
      item to PluginGroupPlugin.

      Note on empty groups: if a group is not used (i.e., has no
      plug-ins inside it), then it does not appear in the
      documentation. This is intentional. For example, it means that
      we could introduce groups in "core code" that may or may not be
      used by plug-ins, and if they are not used, they do not clutter the
      documentation.
     */
    for (const auto &pair: groups) {
        print_section(pair.first, pair.second);
    }
    print_category_footer();
}

void DocPrinter::print_section(
    const string &group_id, const vector<PluginInfo> &infos) {
    if (!group_id.empty()) {
        const PluginGroupInfo &group =
            registry.get_group_info(group_id);
        os << endl << "= " << group.doc_title << " =" << endl << endl;
    }
    for (const PluginInfo &info : infos) {
        print_plugin(info.key, info);
    }
}

void DocPrinter::print_plugin(const string &name, const PluginInfo &info) {
    print_synopsis(info);
    print_usage(name, info);
    print_arguments(info);
    print_notes(info);
    print_language_features(info);
    print_properties(info);
}

Txt2TagsPrinter::Txt2TagsPrinter(ostream &out, Registry &registry)
    : DocPrinter(out, registry) {
}

void Txt2TagsPrinter::print_synopsis(const PluginInfo &info) {
    os << "== " << info.name << " ==" << endl;
    if (!info.synopsis.empty())
        os << info.synopsis << endl;
}

void Txt2TagsPrinter::print_usage(const string &name, const PluginInfo &info) {
    if (!name.empty()) {
        os << "``` " << name << "(";
        for (size_t i = 0; i < info.arg_help.size(); ++i) {
            ArgumentInfo arg = info.arg_help[i];
            os << arg.key;
            if (!arg.default_value.empty())
                os << "=" << arg.default_value;
            if (i != info.arg_help.size() - 1)
                os << ", ";
        }
        os << ")" << endl << endl << endl;
    }
}

void Txt2TagsPrinter::print_arguments(const PluginInfo &info) {
    for (const ArgumentInfo &arg : info.arg_help) {
        os << "- //" << arg.key << "// (" << arg.type_name;
        if (arg.bounds.has_bound())
            os << " \"\"" << arg.bounds << "\"\"";
        os << "): " << arg.help << endl;
        if (!arg.value_explanations.empty()) {
            for (const pair<string, string> &explanation : arg.value_explanations) {
                if (is_call(explanation.first)) {
                    os << endl << "```" << endl << explanation.first << endl
                       << "```" << endl << " " << explanation.second << endl;
                } else {
                    os << " - ``" << explanation.first << "``: "
                       << explanation.second << endl;
                }
            }
        }
    }
}

void Txt2TagsPrinter::print_notes(const PluginInfo &info) {
    for (const NoteInfo &note : info.notes) {
        if (note.long_text) {
            os << "=== " << note.name << " ===" << endl
               << note.description << endl << endl;
        } else {
            os << "**" << note.name << ":** " << note.description << endl << endl;
        }
    }
}

void Txt2TagsPrinter::print_language_features(const PluginInfo &info) {
    if (!info.support_help.empty()) {
        os << "Language features supported:" << endl;
    }
    for (const LanguageSupportInfo &ls : info.support_help) {
        os << "- **" << ls.feature << ":** " << ls.description << endl;
    }
}

void Txt2TagsPrinter::print_properties(const PluginInfo &info) {
    if (!info.property_help.empty()) {
        os << "Properties:" << endl;
    }
    for (const PropertyInfo &prop : info.property_help) {
        os << "- **" << prop.property << ":** " << prop.description << endl;
    }
}

void Txt2TagsPrinter::print_category_header(const string &category_name) {
    os << ">>>>CATEGORY: " << category_name << "<<<<" << endl;
}

void Txt2TagsPrinter::print_category_synopsis(const string &synopsis) {
    if (!synopsis.empty()) {
        os << synopsis << endl;
    }
}

void Txt2TagsPrinter::print_category_predefinitions(
    const string &predefinition_key, const string &alias) {
    if (!predefinition_key.empty()) {
        os << endl << "This plugin type can be predefined using ``--"
           << predefinition_key << "``." << endl;
    }
    if (!alias.empty()) {
        os << "The old predefinition key ``--" << alias << "`` is still "
           << "supported but deprecated." << endl;
    }
}


void Txt2TagsPrinter::print_category_footer() {
    os << endl
       << ">>>>CATEGORYEND<<<<" << endl;
}

PlainPrinter::PlainPrinter(ostream &out, Registry &registry, bool print_all)
    : DocPrinter(out, registry),
      print_all(print_all) {
}

void PlainPrinter::print_synopsis(const PluginInfo &info) {
    os << "== " << info.name << " ==" << endl;
    if (print_all && !info.synopsis.empty()) {
        os << info.synopsis << endl;
    }
}

void PlainPrinter::print_usage(const string &name, const PluginInfo &info) {
    if (!name.empty()) {
        os << name << "(";
        for (size_t i = 0; i < info.arg_help.size(); ++i) {
            ArgumentInfo arg = info.arg_help[i];
            os << arg.key;
            if (!arg.default_value.empty())
                os << "=" << arg.default_value;
            if (i != info.arg_help.size() - 1)
                os << ", ";
        }
        os << ")" << endl;
    }
}

void PlainPrinter::print_arguments(const PluginInfo &info) {
    for (const ArgumentInfo &arg : info.arg_help) {
        os << " " << arg.key << " (" << arg.type_name;
        if (arg.bounds.has_bound())
            os << " " << arg.bounds;
        os << "): " << arg.help << endl;
    }
}

void PlainPrinter::print_notes(const PluginInfo &info) {
    if (print_all) {
        for (const NoteInfo &note : info.notes) {
            if (note.long_text) {
                os << "=== " << note.name << " ===" << endl
                   << note.description << endl << endl;
            } else {
                os << " * " << note.name << ": " << note.description << endl << endl;
            }
        }
    }
}

void PlainPrinter::print_language_features(const PluginInfo &info) {
    if (print_all) {
        if (!info.support_help.empty()) {
            os << "Language features supported:" << endl;
        }
        for (const LanguageSupportInfo &ls : info.support_help) {
            os << " * " << ls.feature << ": " << ls.description << endl;
        }
    }
}

void PlainPrinter::print_properties(const PluginInfo &info) {
    if (print_all) {
        if (!info.property_help.empty()) {
            os << "Properties:" << endl;
        }
        for (const PropertyInfo &prop : info.property_help) {
            os << " * " << prop.property << ": " << prop.description << endl;
        }
    }
}

void PlainPrinter::print_category_header(const string &category_name) {
    os << "Help for " << category_name << endl << endl;
}

void PlainPrinter::print_category_synopsis(const string &synopsis) {
    if (print_all && !synopsis.empty()) {
        os << synopsis << endl;
    }
}

void PlainPrinter::print_category_predefinitions(
    const string &predefinition_key, const string &alias) {
    if (!predefinition_key.empty()) {
        os << endl << "This plugin type can be predefined using --"
           << predefinition_key << "." << endl;
    }
    if (!alias.empty()) {
        os << "The old predefinition key --" << alias << " is still "
           << "supported but deprecated." << endl;
    }
}

void PlainPrinter::print_category_footer() {
    os << endl;
}
}
