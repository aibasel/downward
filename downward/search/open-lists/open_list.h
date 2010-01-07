#ifndef OPEN_LIST_H
#define OPEN_LIST_H

class Evaluator;

template<class Entry>
class OpenList {

protected:
    virtual Evaluator* get_evaluator() = 0;
    bool only_preferred;

public:
    OpenList(bool preferred_only=false) : only_preferred(preferred_only) {} 
    virtual ~OpenList() {}
    
    virtual int insert(const Entry &entry) = 0;
    virtual Entry remove_min() = 0;
    virtual bool empty() const = 0;
    virtual void clear() = 0;
    bool only_preferred_states() const {return only_preferred;}
    // should only be used within alternation open lists
    // a search does not have to care about this because 
    // it is handled by the open list whether the entry will
    // be inserted
    
    virtual void evaluate(int g, bool preferred) = 0;
    virtual bool is_dead_end() const = 0;
    virtual bool dead_end_is_reliable() const = 0;

    virtual int boost_preferred() { return 0; }
    virtual void boost_last_used_list() { return; }
};

#endif
