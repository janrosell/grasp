/*!
    \file plugins/ConcurrentQueue.hh
    \brief Queue data structure for concurrent access

    Naive queue data structure for correct concurrent access and decent performance.
    Heavily inspired in
    <a href="https://gist.github.com/dpressel/de9ea7603fa3f20b55bf">this gist</a>
*/

#ifndef __CONCURRENT_QUEUE_H__
#define __CONCURRENT_QUEUE_H__

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>

/// \brief FIFO queue for threadsafe access
///
/// Generic concurrent access queue for consumer-producer scenario
class ConcurrentQueue
{
    /// Condition variable
    std::condition_variable cond_var;
    /// Mutex
    std::mutex mutex;
    /// Generic queue
    std::queue<T> queue;
    /// Max size of queue
    int max_size;

    /// \brief Constructor
    /// \param size maximum queue size
    public: ConcurrentQueue(int size):
        max_size(size) {}

    /// \brief Add a data object to tail
    /// \param data Object to add
    public: void enqueue(T data)
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond_var.wait(lock, [this](){ return !isFull();});
        queue.push(data);
        lock.unlock();
        cond_var.notify_all();
    }

    /// \brief Remove data object from head
    /// \param data Removed object
    public: void dequeue(T &data)
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond_var.wait(lock, [this](){ return !isEmpty();});
        data = queue.front();
        queue.pop();
        lock.unlock();
        cond_var.notify_all();   
    }

    /// \brief  Clear queue
    public: void clear()
    {
        std::unique_lock<std::mutex> lock(mutex);
        while(!isEmpty()) {
            queue.pop();
        }
        lock.unlock();
        cond_var.notify_all();
    }
    
    /// \brief Returns whether queue is full
    /// \return true if queue is full, false otherwise
    private: bool isFull() const
    {
        bool full = queue.size() >= max_size;
        return queue.size() >= max_size;
    }

    /// \brief Returns whether queue is empty
    /// \return true if queue is empty, false otherwise
    private: bool isEmpty() const
    {
        return queue.size() == 0;
    }

    /// \brief Returns queue's current size
    /// \return queue current size
    public: int length() const
    {
        return queue.size();
    }
};

#endif
