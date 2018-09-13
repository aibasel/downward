#ifndef ALGORITHMS_SUBSCRIBER_H
#define ALGORITHMS_SUBSCRIBER_H

#include <cassert>
#include <unordered_set>

namespace subscriber {
template<typename T>
class SubscriberService;
/*
  A Subscriber can subscribe to a SubscriberService and is notified if that
  service is destroyed.
*/
template<typename T>
class Subscriber {
    friend class SubscriberService<T>;
    std::unordered_set<const SubscriberService<T> *> services;
    virtual void notify_service_destroyed(const T *) = 0;
public:
    virtual ~Subscriber() {
        for (const SubscriberService<T> * service : services) {
            service->unsubscribe(this);
        }
    }
};

template<typename T>
class SubscriberService {
    /*
      Note that conceptually the list of subscribers should not be mutable and
      the methods to (un)subscribe should not be const. However, making these
      methods non-const, would mean that the state registry could no longer be
      const in PerStateInformation and AbstractTask could not be const in
      PerTaskInformation.
    */
    mutable std::unordered_set<Subscriber<T> *> subscribers;
public:
    virtual ~SubscriberService() {
        for (Subscriber<T> *subscriber : subscribers) {
            subscriber->notify_service_destroyed(static_cast<T *>(this));
        }
    }

    /*
      Remembers the given Subscriber. If this SubscriberService is
      destroyed, it notifies all current subscribers.
    */
    void subscribe(Subscriber<T> *subscriber) const {
        assert(subscribers.find(subscriber) == subscribers.end());
        subscribers.insert(subscriber);
        assert(subscriber->services.find(this) == subscriber->services.end());
        subscriber->services.insert(this);
    }

    void unsubscribe(Subscriber<T> *subscriber) const {
        assert(subscribers.find(subscriber) != subscribers.end());
        subscribers.erase(subscriber);
        assert(subscriber->services.find(this) != subscriber->services.end());
        subscriber->services.erase(this);
    }
};
}
#endif
