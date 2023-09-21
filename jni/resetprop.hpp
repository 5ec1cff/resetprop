#pragma once

#include <string>
#include <map>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <api/_system_properties.h>
#include "base.hpp"

struct prop_cb {
    virtual void exec(const char *name, const char *value) = 0;
};

using prop_list = std::map<std::string, std::string>;

struct prop_collector : prop_cb {
    explicit prop_collector(prop_list &list) : list(list) {}
    void exec(const char *name, const char *value) override {
        list.insert({name, value});
    }
private:
    prop_list &list;
};

// System properties
std::string get_prop(const char *name, bool persist = false);
int delete_prop(const char *name, bool persist = false);
int set_prop(const char *name, const char *value, bool skip_svc = false);
void load_prop_file(const char *filename, bool skip_svc = false);

static inline void prop_cb_exec(prop_cb &cb, const char *name, const char *value) {
    cb.exec(name, value);
}

// https://github.com/topjohnwu/Magisk/commit/8d81bd0e33a5ff25bb85b73b9198b7259213e7bb#diff-563644449824d750c091a4a472a0aa6fb7403e317a387051fce6b0ec7d7edf4e
void persist_get_prop(const char *name, prop_cb *prop_cb);
void persist_get_props(prop_cb *prop_cb);
bool persist_delete_prop(const char *name);
bool persist_set_prop(const char *name, const char *value);

// misc.hpp
static inline bool str_contains(std::string_view s, std::string_view ss) {
    return s.find(ss) != std::string::npos;
}
static inline bool str_starts(std::string_view s, std::string_view ss) {
    return s.size() >= ss.size() && s.compare(0, ss.size(), ss) == 0;
}
static inline bool str_ends(std::string_view s, std::string_view ss) {
    return s.size() >= ss.size() && s.compare(s.size() - ss.size(), std::string::npos, ss) == 0;
}