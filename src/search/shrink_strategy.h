#ifndef SHRINK_STRATEGY_H
#define SHRINK_STRATEGY_H
#include <string>
#include <utility>
#include <vector>
#include <ext/slist>

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


//TODO - document purpose of Signatures
typedef std::vector<std::pair<int, int> > SuccessorSignature;

struct Signature {
    int h;
    int group;
    SuccessorSignature succ_signature;
    int state;

    Signature(int h_, int group_, const SuccessorSignature &succ_signature_,
              int state_)
        : h(h_), group(group_), succ_signature(succ_signature_), state(state_) {
    }

    bool matches(const Signature &other) const {
        return h == other.h && group == other.group && succ_signature
               == other.succ_signature;
    }

    bool operator<(const Signature &other) const {
        if (h != other.h)
            return h < other.h;
        if (group != other.group)
            return group < other.group;
        if (succ_signature != other.succ_signature)
            return succ_signature < other.succ_signature;
        return state < other.state;
    }
};


#endif
