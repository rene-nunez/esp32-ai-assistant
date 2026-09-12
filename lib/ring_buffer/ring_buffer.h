#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <Arduino.h>

template<typename T, int Capacity>

class RingBuffer {
public:
  void enqueue(T item) {
    int next = (tail_ + 1) % Capacity;
    if (next != head_) {
      buf_[tail_] = item;
      tail_ = next;
    }
  }

  bool empty() const {
    return head_ == tail_;
  }

  T dequeue() {
    if (empty()) return T();
    T item = buf_[head_];
    head_ = (head_ + 1) % Capacity;
    return item;
  }

private:
  T buf_[Capacity];
  int head_ = 0;
  int tail_ = 0;
};

#endif
