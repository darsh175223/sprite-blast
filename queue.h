#ifndef QUEUE_H
#define QUEUE_H

class Queue {
private:
    static const int MAX_SIZE = 20;
    int arr[MAX_SIZE];
    int front;
    int rear;
    int size;

public:
    Queue();
    bool push(int value);
    bool empty();
    int pop();
};

Queue::Queue() {
    front = 0;
    rear = -1;
    size = 0;
}

bool Queue::push(int value) {
    if (size >= MAX_SIZE) {
        return false;
    }
    rear = (rear + 1) % MAX_SIZE;
    arr[rear] = value;
    size++;
    return true;
}

bool Queue::empty() {
    return size == 0;
}

int Queue::pop() {
    if (empty()) {
        return -1;
    }
    int value = arr[front];
    front = (front + 1) % MAX_SIZE;
    size--;
    return value;
}

#endif