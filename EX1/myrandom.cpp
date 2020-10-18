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
QSemaphore space(BUFFERSIZE);   
QSemaphore nitems;              
QMutex ctrl, ctrl2;                  
int numOfNumbersProducedSoFar = 0;
int NUM;


//===============================================
class Producer : public QThread
{
private:
    int total;      // number of random numbers to generate
    int ID;
public:
    Producer(int i, int ID) : total(i) {     
        this -> ID = ID + 1;
        srand(time(0));                

        ctrl2.lock();
        cout << "Producer " << this -> ID << " will generate " << total << " random numbers" << endl;
        ctrl2.unlock();

    }
    void run()
    {
        for (int j = 0; j < total; j++)
        {
            space.acquire();                
            int random = rand();            
            ctrl2.lock();
            buffer[in++] = random;           
            in %= BUFFERSIZE;                
            numOfNumbersProducedSoFar++;
            ctrl2.unlock();
            nitems.release();               
        }
        // write -1 to buffer to terminate consumer threads ONLY if all numbers have been generate

        if(numOfNumbersProducedSoFar == NUM){
            for (int j = 0; j < N; j++){
                space.acquire();
                ctrl2.lock();
                buffer[in++] = -1;                // consumer threads will exit when they read -1
                in %= BUFFERSIZE;
                ctrl2.unlock();
                nitems.release();
            } 

        }
    }
};


//===============================================
class Consumer : public QThread
{
private:
    int ID; // identifies the consumer thread
public:
    Consumer(int i) : ID(i) {} 
    void run()
    {
        int item, loc, nread = 0;
        while (1) // loop infinitely until it reads a -1
        {
            nitems.acquire();               
            ctrl.lock();                    
            loc = out;                      
            out = (out + 1) % BUFFERSIZE;     
            ctrl.unlock();                  
            item = buffer[loc];             
            space.release();

            if (item < 0) break;              
            else nread++;                  
        }
        ctrl.lock();
        cout << "Consumer Thread " << ID << " read a total of " << nread << endl;
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
    NUM = atoi(argv[3]);               // total number of random numbers to be generated




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