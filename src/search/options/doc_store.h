#ifndef OPTIONS_DOC_STORE_H
#define OPTIONS_DOC_STORE_H

#include "bounds.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace options {
typedef std::vector<std::pair<std::string, std::string>> ValueExplanations;
struct ArgumentInfo {
    ArgumentInfo(
        std::string k, std::string h, std::string t_n, std::string def_val,
        const Bounds &bounds, ValueExplanations val_expl)
        : kwd(k),
          help(h),
          type_name(t_n),
          default_value(def_val),
          bounds(bounds),
          value_explanations(val_expl) {
    }
    std::string kwd;
    std::string help;
    std::string type_name;
    std::string default_value;
    Bounds bounds;
    std::vector<std::pair<std::string, std::string>> value_explanations;
};

struct PropertyInfo {
    PropertyInfo(std::string prop, std::string descr)
        : property(prop),
          description(descr) {
    }
    std::string property;
    std::string description;
};

struct NoteInfo {
    NoteInfo(std::string n, std::string descr, bool long_text_)
        : name(n),
          description(descr),
          long_text(long_text_) {
    }
    std::string name;
    std::string description;
    bool long_text;
};


struct LanguageSupportInfo {
    LanguageSupportInfo(std::string feat, std::string descr)
        : feature(feat),
          description(descr) {
    }
    std::string feature;
    std::string description;
};

//stores documentation for a single type, for use in combination with DocStore
struct DocStruct {
    std::string type;
    std::string full_name;
    std::string synopsis;
    std::vector<ArgumentInfo> arg_help;
    std::vector<PropertyInfo> property_help;
    std::vector<LanguageSupportInfo> support_help;
    std::vector<NoteInfo> notes;
    bool hidden;
};

//stores documentation for types parsed in help mode
class DocStore {
public:
    static DocStore *instance() {
        static DocStore instance_;
        return &instance_;
    }

    void register_object(std::string k, std::string type);

    void add_arg(std::string k,
                 std::string arg_name,
                 std::string help,
                 std::string type,
                 std::string default_value,
                 Bounds bounds,
                 ValueExplanations value_explanations = ValueExplanations());
    void add_value_explanations(std::string k,
                                std::string arg_name,
                                ValueExplanations value_explanations);
    void set_synopsis(std::string k,
                      std::string name, std::string description);
    void add_property(std::string k,
                      std::string name, std::string description);
    void add_feature(std::string k,
                     std::string feature, std::string description);
    void add_note(std::string k,
                  std::string name, std::string description, bool long_text);
    void hide(std::string k);

    bool contains(std::string k);
    DocStruct get(std::string k);
    std::vector<std::string> get_keys();
    std::vector<std::string> get_types();

private:
    DocStore() = default;
    std::map<std::string, DocStruct> registered;
};
}

#endif
