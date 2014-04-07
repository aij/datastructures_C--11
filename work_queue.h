#include <atomic>
#include <condition_variable>
#include <mutex>
#include "list.h"

#ifndef WORKQUEUE_H
#define WORKQUEUE_H

template <typename T>
class WorkQueueNode: public ListNode_base<WorkQueueNode<T>> {
  public:
    T data;
    WorkQueueNode(T new_data) {
      data = new_data;
    }
};

template <typename T>
class WorkQueue {
  private:
    unsigned int waiters;
    unsigned int datum;
    std::mutex m;
    std::condition_variable convar;
    List<WorkQueueNode<T>> list;
  public:
    WorkQueue();
    // default destructor is fine
    void enqueue(T data);
    T dequeue();
    unsigned int getWaiters();
    unsigned int getDatum();
};

template <typename T>
WorkQueue<T>::WorkQueue() {
  waiters=0;
  datum=0;
}

template <typename T>
void WorkQueue<T>::enqueue(T data) {
  auto n = new WorkQueueNode<T>(data) ;
  std::unique_lock<std::mutex> l(m);
  list.enqueue(n);
  datum++;
  if (waiters > 0) {
    convar.notify_one(); // Mutex is held, this is slow and conservative
  }
}

template <typename T>
T WorkQueue<T>::dequeue() {
  std::unique_lock<std::mutex> l(m);
  waiters++; //we're waiting
  auto n = list.dequeue();
  while (!n) {
    convar.wait(l); // unlocks, and relocks, the mutex
    n = list.dequeue();
  } 
  waiters--; // we're no longer waiting
  // Note that we may increment and decrement waiters immediatly.
  // It's faster than a conditional anyway, and no-one else will see it.
  datum--; // we pulled an element
  m.unlock();
  T data = n->data;
  delete n;
  return data;
}

template <typename T>
unsigned int WorkQueue<T>::getWaiters() {
  return waiters;
}

template <typename T>
unsigned int WorkQueue<T>::getDatum() {
  return datum;
}

#endif
