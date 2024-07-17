#pragma once 

#include <vector>
#include <ctime>

struct HeapItem{
    int fd;
    time_t expiry; // time_t is a type that represents time in seconds since epoch

    HeapItem(): fd(-1), expiry(0) {}
    HeapItem(int fd, time_t expiry): fd(fd), expiry(expiry) {}
};

class TimeoutHeap{
    private:
        std::vector<HeapItem> heap;

        void heapifyUp(int index);
        void heapifyDown(int index);
    
    public:
        void insert(int fd, time_t expiry);
        bool extractMin(HeapItem& item);
        void update(int fd, time_t expiry);
        bool empty() const { return heap.empty(); }
        time_t nextExpiry() const { return empty() ? 0 : heap[0].expiry;}
};

