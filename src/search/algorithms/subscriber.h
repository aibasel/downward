#ifndef ALGORITHMS_SUBSCRIBER
#define ALGORITHMS_SUBSCRIBER

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
    virtual void notify_service_destroyed(const T *) = 0;
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
        subscribers.insert(subscriber);
    }

    void unsubscribe(Subscriber<T> *subscriber) const {
        subscribers.erase(subscriber);
    }
};
}
#endif
