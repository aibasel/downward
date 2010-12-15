#ifndef PLUGIN_H
#define PLUGIN_H

#include "search_engine.h"

class LandmarksGraph;
class OptionParser;
template <class Entry>
class OpenList;


struct Synergy {
    std::vector<Heuristic *> heuristics;
};

//a registry<T> maps a string to a T-factory
template <class T>
class Registry{
public:
    typedef T (*Factory)(OptionParser&);
    static Registry<T>* instance()
    {
        if (!instance_) {
            instance_ = new Registry<T>();
        }
        return instance_;
    }
            
    void register_object(std::string k, Factory f) {
        registered[k] = f;
    }

    bool contains(std::string k) {
        return registered.find(k) != registered.end();
    }

    Factory get(std::string k) {
        return registered[k];
    }
private:
    Registry(){};
    static Registry<T> *instance_;
    std::map<std::string, Factory> registered;
};

template <class T> Registry<T>* Registry<T>::instance_ = 0;


//Predefinitions<T> maps strings to pointers to
//already created Heuristics/LandmarksGraphs
template <class T>
class Predefinitions {
public:
    static Predefinitions<T>* instance()
    {
        if (!instance_) {
            instance_ = new Predefinitions<T>();
        }
        return instance_;
    }

    void predefine(std::string k, T obj) {
        predefined[k] = obj;
    }

    bool contains(std::string k) {
        return predefined.find(k) != predefined.end();
    }

    T get(std::string k) {
        return predefined[k];
    }

private:
    Predefinitions<T>(){};
    static Predefinitions<T>* instance_;
    std::map<std::string, T> predefined;
};

template <class T> Predefinitions<T>* Predefinitions<T>::instance_ = 0;


class ScalarEvaluatorPlugin {
      ScalarEvaluatorPlugin(const ScalarEvaluatorPlugin &copy);
public:
    ScalarEvaluatorPlugin(const std::string &key,
                          Registry<ScalarEvaluator *>::Factory factory);
    ~ScalarEvaluatorPlugin();
};


class SynergyPlugin {
    SynergyPlugin(const SynergyPlugin &copy);
public:
    //there isn't really a Synergy class, it's just a pseudoclass 
    //to unify syntax.
    SynergyPlugin(const std::string &key,
                  Registry<Synergy *>::Factory factory);
    ~SynergyPlugin();
};


class LandmarkGraphPlugin {
    LandmarkGraphPlugin(const LandmarkGraphPlugin &copy);
public:
    LandmarkGraphPlugin(const std::string &key,
                        Registry<LandmarksGraph *>::Factory factory);
    ~LandmarkGraphPlugin();
};


class EnginePlugin {
    EnginePlugin(const EnginePlugin &copy);
public:
    EnginePlugin(const std::string &key,
                 Registry<SearchEngine *>::Factory factory);
    ~EnginePlugin();
};

template <class Entry>
class OpenListPlugin {
    OpenListPlugin(const OpenListPlugin<Entry> &copy);
public:
    OpenListPlugin(const std::string &key,
                   typename Registry<OpenList<Entry > *>::Factory factory);
    ~OpenListPlugin();
};




#endif
