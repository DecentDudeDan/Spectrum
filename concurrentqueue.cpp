#include "concurrentqueue.h"

ConcurrentQueue::ConcurrentQueue(){
}

void ConcurrentQueue::enqueue(std::complex<double> value)
{
    mutex.lock();
    points.enqueue(value);
    mutex.unlock();
}

std::complex<double> ConcurrentQueue::dequeue()
{

    mutex.lock();
    return points.dequeue();
    mutex.unlock();

    std::complex<double> tmp;

    mutex.lock();

    if(!points.isEmpty())
    {
        tmp = points.dequeue();
    }

    mutex.unlock();

    return tmp;
}

bool ConcurrentQueue::isEmpty()
{
    return points.isEmpty();
}

int ConcurrentQueue::size()
{
    return points.size();
}

void ConcurrentQueue::unlock()
{
    mutex.unlock();
}