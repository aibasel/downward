#ifndef ALGORITHMS_SUBSCRIBER_H
#define ALGORITHMS_SUBSCRIBER_H

#include <cassert>
#include <unordered_set>

/*
  The classes in this file allow objects of one class to react to the
  destruction of objects from a different class.
  To react to the desctruction of objects from a class T, derive T from
  SubscriberService<T>. Then derive the class that should react to the
  destruction from Subscriber<T> and override the method
  notify_service_destroyed.

  Example:

  class Star : public SubscriberService<Star> {
      string name;
      ...
  }

  vector<const Star *> galaxy;

  class Astronomer : public Subscriber<Star> {
      Astronomer() {
          for (const Star *star : galaxy) {
              star.subscribe(this);
          }
      }

      virtual void notify_service_destroyed(const Star *star) {
          cout << star->name << " is going supernova!\n";
      }
  }
*/


namespace subscriber {
template<typename T>
class SubscriberService;

/*
  A Subscriber can subscribe to a SubscriberService and is notified if that
  service is destroyed. The template parameter T should be the class of the
  SubscriberService (see usage example above).
*/
template<typename T>
class Subscriber {
    friend class SubscriberService<T>;
    std::unordered_set<const SubscriberService<T> *> services;
    virtual void notify_service_destroyed(const T *) = 0;
public:
    virtual ~Subscriber() {
        for (const SubscriberService<T> *service : services) {
            service->unsubscribe(this);
        }
    }
};

template<typename T>
class SubscriberService {
    /*
      We make the set of subscribers mutable, which means that it is possible
      to subscribe to `const` objects. This can be justified by arguing that
      subscribing to an object is not conceptually a mutation of the object,
      and the set of subscribers is only incidentally (for implementation
      efficiency) stored along with the object. Of course it is possible to
      argue the other way, too. We made this design decision because being able
      to subscribe to const objects is very useful in the planner.
    */
    mutable std::unordered_set<Subscriber<T> *> subscribers;
public:
    virtual ~SubscriberService() {
        for (Subscriber<T> *subscriber : subscribers) {
            subscriber->notify_service_destroyed(static_cast<T *>(this));
        }
    }

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
