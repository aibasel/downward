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
