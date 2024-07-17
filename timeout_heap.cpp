#include "timeout_heap.h"
#include <algorithm>

void TimeoutHeap::heapifyUp(int index)
{
    while(index>0)
    {
        int parent = (index-1)/2;
        if(heap[parent].expiry <= heap[index].expiry)
            break;
        std::swap(heap[parent], heap[index]);
        index = parent;
    }
}

void TimeoutHeap::heapifyDown(int index)
{
    int size = heap.size();
    while(1)
    {
        int left = 2*index + 1;
        int right = 2*index + 2;
        int smallest = index;

        if(left<size && heap[left].expiry < heap[smallest].expiry)
            smallest = left;
        if(right<size && heap[right].expiry < heap[smallest].expiry)
            smallest = right;

        if(smallest == index)
            break;
        std::swap(heap[index], heap[smallest]);
        index = smallest;
    }
}

void TimeoutHeap::insert(int fd,time_t expiry)
{
    heap.push_back(HeapItem(fd, expiry));
    heapifyUp(heap.size()-1);
}

bool TimeoutHeap::extractMin(HeapItem& item)
{
    if(heap.empty()) return false;

    item = heap[0];
    heap[0] = heap.back();
    heap.pop_back();

    if(!heap.empty())
        heapifyDown(0);

    return true;
}

void TimeoutHeap::update(int fd,time_t new_expiry)
{
    for(size_t i=0;i<heap.size();i++)
    {
        if(heap[i].fd == fd)
        {
            heap[i].expiry = new_expiry;
            heapifyUp(i);
            heapifyDown(i);
            break;
        }
    }
}