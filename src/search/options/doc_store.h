#ifndef OPTIONS_DOC_STORE_H
#define OPTIONS_DOC_STORE_H

#include "bounds.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace options {
using ValueExplanations = std::vector<std::pair<std::string, std::string>>;

struct ArgumentInfo {
    ArgumentInfo(
        const std::string &k, const std::string &h, const std::string &t_n, const std::string &def_val,
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
    PropertyInfo(const std::string &prop, const std::string &descr)
        : property(prop),
          description(descr) {
    }
    std::string property;
    std::string description;
};

struct NoteInfo {
    NoteInfo(const std::string &n, const std::string &descr, bool long_text_)
        : name(n),
          description(descr),
          long_text(long_text_) {
    }
    std::string name;
    std::string description;
    bool long_text;
};


struct LanguageSupportInfo {
    LanguageSupportInfo(const std::string &feat, const std::string &descr)
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

    void register_object(std::string k, const std::string &type);

    void add_arg(const std::string &k,
                 const std::string &arg_name,
                 const std::string &help,
                 const std::string &type,
                 const std::string &default_value,
                 Bounds bounds,
                 ValueExplanations value_explanations = ValueExplanations());
    void add_value_explanations(const std::string &k,
                                const std::string &arg_name,
                                ValueExplanations value_explanations);
    void set_synopsis(const std::string &k,
                      const std::string &name, const std::string &description);
    void add_property(const std::string &k,
                      const std::string &name, const std::string &description);
    void add_feature(const std::string &k,
                     const std::string &feature, const std::string &description);
    void add_note(const std::string &k,
                  const std::string &name, const std::string &description, bool long_text);
    void hide(const std::string &k);

    bool contains(const std::string &k);
    DocStruct get(const std::string &k);
    std::vector<std::string> get_keys();
    std::vector<std::string> get_types();

private:
    DocStore() = default;
    std::map<std::string, DocStruct> registered;
};
}

#endif
