#ifndef SHRINK_STRATEGY_H
#define SHRINK_STRATEGY_H
#include <string>
#include <utility>
#include <vector>
#include <ext/slist>

/*
  TODO: 

  * consider performing normalize before actual shrink,
  on all bisim strategies...
  At least in gripper it improves performance...
  In further label reductions, this should at least be done, 
  and if normalize includes
  the update of the transition by source database, 
  this should be done so the transition by source database is updated.
*/

class Abstraction;

typedef int AbstractStateRef;
typedef std::vector<AbstractStateRef> Bucket;

class ShrinkStrategy {
public:
    ShrinkStrategy();
    virtual ~ShrinkStrategy();

    /* Shrink the abstraction to size threshold.

     This also prunes all irrelevant and unreachable states, which
     may cause the resulting size to be lower than threshold.

     Does nothing if threshold >= abs.size() unless force is true (in
     which case it only prunes irrelevant and unreachable states).
     */
    virtual void shrink(Abstraction &abs, int threshold, bool force = false) const = 0;
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
