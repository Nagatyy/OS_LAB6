#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#define BUFFERSIZE 10
#define NCONSUMERS 3

int in = 0, out = 0, buffer[BUFFERSIZE];
QSemaphore space(BUFFERSIZE);   // space is initialized to 10
QSemaphore nitems;              // nitmes is initialized to 0
QMutex ctrl;                    // mutex is a binary semaphore with values 0 or 1


/* we will create ONE producer thread that will generate 100 random integers
and store them in the 10 element integer array */
//===============================================
class Producer : public QThread
{
private:
    int total;      // number of random numbers to generate
public:
    Producer(int i) : total(i) {     // constructor initializes 'total' member variable
        srand(time(0));                 // use srand() to generate a diff sequence of random number with every program run
    }
    void run()
    {
        for (int j = 0; j < total; j++)
        {
            space.acquire();                // decrement the number of available spaces in the array
            int random = rand();            // generate a random integer number
            buffer[in++] = random;            // store the random integer number in the array
            in %= BUFFERSIZE;                 // array of 10 elements has index 0 to 9, reset to 0 when index reaches 10
            nitems.release();               // increment the number of available items in the array to read
        }
        // write -1 to buffer to terminate consumer threads
        for (int j = 0; j < NCONSUMERS; j++)
        {
            space.acquire();
            buffer[in++] = -1;                // consumer threads will exit when they read -1
            in %= BUFFERSIZE;
            nitems.release();
        }// producer thread will finally write the -1s and exit
    }
};


/* we will create 3 consumer thread that will read the numbers stored in the array
1. no 2 consumer threads are allowed to read the same array element - use a mutex lock
3. all 3 tghreads are allowed to access the buffer array simultaneously - use a semaphore */
//===============================================
class Consumer : public QThread
{
private:
    int ID; // identifies the consumer thread
public:
    Consumer(int i) : ID(i) {} // constructor to initialize the consumer thread id
    void run()
    {
        int item, loc, nread = 0;
        while (1) // loop infinitely until it reads a -1
        {
            nitems.acquire();               // decrement the number of available items available in the buffer
            ctrl.lock();                    // lock access to shared resource 'out'
            loc = out;                      // 'out' is the index used to read array elements
            out = (out + 1) % BUFFERSIZE;     // increment 'out' so that next thread will not read the same array element
            ctrl.unlock();                  // unlock access to shared resource 'out'
            item = buffer[loc];             // can we swap these two statements?
            space.release();

            if (item < 0) break;               // consumer thread will break and exit if it reads -1 (<0)
            else nread++;                   // increment valid numbers read by the consumer thread
        }
        ctrl.lock();
        cout << "Consumer Thread " << ID << " read a total of " << nread << endl;       // consumer thread prints how many numbers it had read
        ctrl.unlock();
    }
};

//================================================
int main(int argc, char* argv[])
{
    Producer p(100);                // generate 100 random numbers (total = 100)
    Consumer* c[NCONSUMERS];        // 3 consumer threads
    p.start();                      // start the producer thread

    for (int i = 0; i < NCONSUMERS; i++)   // loop three times and dynamically create the consumer threads
    {
        c[i] = new Consumer(i);
        c[i]->start();
    }

    p.wait();                       // wait for the producer thread

    for (int i = 0; i < NCONSUMERS; i++)
        c[i]->wait();           // wait for the consumer threads
    return 0;
}