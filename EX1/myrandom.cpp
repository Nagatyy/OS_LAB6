#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#define BUFFERSIZE 10
int N;                          // number of consumer threads

int in = 0, out = 0, buffer[BUFFERSIZE];
QSemaphore space(BUFFERSIZE);   // space is initialized to 10
QSemaphore nitems;              // nitmes is initialized to 0
QMutex ctrl, ctrl2;                    // mutex is a binary semaphore with values 0 or 1


//===============================================
class Producer : public QThread
{
private:
    int total;      // number of random numbers to generate
    int ID;
public:
    Producer(int i, int ID) : total(i) {     // constructor initializes 'total' member variable
        this -> ID = ID + 1;
        srand(time(0));                 // use srand() to generate a diff sequence of random number with every program run

        ctrl2.lock();
        cout << "Producer " << this -> ID << " will generate " << total << " random numbers" << endl;
        ctrl2.unlock();

    }
    void run()
    {
        for (int j = 0; j < total; j++)
        {
            space.acquire();                // decrement the number of available spaces in the array
            int random = rand();            // generate a random integer number
            ctrl2.lock();
            buffer[in++] = random;            // store the random integer number in the array
            in %= BUFFERSIZE;                 // array of 10 elements has index 0 to 9, reset to 0 when index reaches 10
            ctrl2.unlock();
            nitems.release();               // increment the number of available items in the array to read
        }
        // write -1 to buffer to terminate consumer threads
        for (int j = 0; j < N; j++)
        {
            space.acquire();
            ctrl2.lock();
            buffer[in++] = -1;                // consumer threads will exit when they read -1
            in %= BUFFERSIZE;
            ctrl2.unlock();
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
int main(int argc, char** argv){

    srand(time(0)); 

     if (argc != 4){
        std::cout << "Incorrect Number of Arguments!" <<std::endl;
        return 0; 
    }
    
    int M = atoi(argv[1]);                 // num of producer threads
    N = atoi(argv[2]);                 
    int NUM = atoi(argv[3]);               // total number of random numbers to be generated




    Producer* p[M];                 // M producer threads
    Consumer* c[N];                 // N consumer threads

    int numbersToProduce;
    int totalNumbersProduced = 0;   // to hold the number of numbers produced so far

    for(int i = 0; i < M; i++){

        // the last producer thread will proeduce NUM - total numbers produced
        if(i == M-1){ 
            p[i] = new Producer(NUM - totalNumbersProduced, i);
            p[i] -> start();
        }
        else {
            numbersToProduce = rand() % (NUM - totalNumbersProduced);
            totalNumbersProduced+= numbersToProduce;
            p[i] = new Producer(numbersToProduce, i);
            p[i] -> start();
        }
    }

    
    for (int i = 0; i < N; i++) {
        c[i] = new Consumer(i);
        c[i]->start();
    }

    for(int i = 0; i < M; i++)
        p[i] -> wait();


    for (int i = 0; i < N; i++)
        c[i]->wait();           // wait for the consumer threads
    return 0;
}