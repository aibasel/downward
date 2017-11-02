#include "doc_printer.h"

#include "doc_store.h"
#include "registries.h"

#include <iostream>

using namespace std;

namespace options {
static bool is_call(const string &s) {
    return s.find("(") != string::npos;
}

DocPrinter::DocPrinter(ostream &out)
    : os(out) {
    for (const string &key : DocStore::instance()->get_keys()) {
        DocStore::instance()->get(key).fill_docs();
    }
}

DocPrinter::~DocPrinter() {
}

void DocPrinter::print_all() {
    for (const PluginTypeInfo &info : PluginTypeRegistry::instance()->get_sorted_types()) {
        print_category(info.get_type_name(), info.get_documentation());
    }
}

void DocPrinter::print_plugin(const string &name) {
    print_plugin(name, DocStore::instance()->get(name));
}

void DocPrinter::print_category(const string &plugin_type_name, const string &synopsis) {
    print_category_header(plugin_type_name);
    print_category_synopsis(synopsis);
    DocStore *doc_store = DocStore::instance();
    for (const string &key : doc_store->get_keys()) {
        PluginInfo info = doc_store->get(key);
        if (info.get_type_name() == plugin_type_name && !info.hidden) {
            print_plugin(key, info);
        }
    }
    print_category_footer();
}

void DocPrinter::print_plugin(const string &name, const PluginInfo &info) {
    print_synopsis(info);
    print_usage(name, info);
    print_arguments(info);
    print_notes(info);
    print_language_features(info);
    print_properties(info);
}

Txt2TagsPrinter::Txt2TagsPrinter(ostream &out)
    : DocPrinter(out) {
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

void Txt2TagsPrinter::print_category_footer() {
    os << endl
       << ">>>>CATEGORYEND<<<<" << endl;
}

PlainPrinter::PlainPrinter(ostream &out, bool pa)
    : DocPrinter(out),
      print_all(pa) {
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

void PlainPrinter::print_category_footer() {
    os << endl;
}
}
