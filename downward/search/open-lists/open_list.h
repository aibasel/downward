#ifndef OPEN_LIST_H
#define OPEN_LIST_H

class Evaluator;

template<class Entry>
class OpenList {

protected:
    virtual Evaluator* get_evaluator() = 0;

public:
    virtual ~OpenList() {}
    
    virtual int insert(const Entry &entry) = 0;
    virtual Entry remove_min() = 0;
    virtual bool empty() const = 0;

    virtual void evaluate(int g, bool preferred) = 0;
    virtual bool is_dead_end() const = 0;
    virtual bool dead_end_is_reliable() const = 0;
    
};

#endif
