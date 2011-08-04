#ifndef SHRINK_STRATEGY_H
#define SHRINK_STRATEGY_H
#include <string>
#include <vector>
#include <ext/slist>

class Abstraction;

typedef int AbstractStateRef;
typedef std::vector<AbstractStateRef> Bucket;

class ShrinkStrategy {
public:
    ShrinkStrategy();
    virtual ~ShrinkStrategy();
    virtual void shrink(Abstraction &abs, int threshold, bool force = false)=0;
    enum {
        QUITE_A_LOT = 1000000000
    };

    virtual bool has_memory_limit() const = 0;
    virtual bool is_bisimulation() const = 0;
    virtual bool is_dfp() const = 0;

    virtual std::string description() const = 0;

protected:
    bool must_shrink(const Abstraction &abs, int threshold, bool force) const;
    void apply(
        Abstraction &abs, 
        std::vector<__gnu_cxx::slist<AbstractStateRef> > &collapsed_groups, 
        int threshold) const;


};


#endif
