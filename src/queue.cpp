#include "config.h"

static String queue[MAX_QUEUE];
static int head = 0;
static int tail = 0;

void enqueue(String s) {                                         // bounded ring buffer — predictable RAM
    int next = (tail + 1) % MAX_QUEUE;
    if (next != head) {                                          // drops newest silently on full (beats OOM)
        queue[tail] = s;
        tail = next;
    }
}

bool queue_empty() {
    return head == tail;
}

String dequeue() {
    String s = queue[head];
    head = (head + 1) % MAX_QUEUE;
    return s;
}
