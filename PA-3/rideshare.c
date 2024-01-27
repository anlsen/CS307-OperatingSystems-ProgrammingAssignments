
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>



pthread_mutex_t mutex_obj=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_objSecond=PTHREAD_MUTEX_INITIALIZER;;

pthread_mutex_t mutex_objA=PTHREAD_MUTEX_INITIALIZER;;
pthread_mutex_t mutex_objB=PTHREAD_MUTEX_INITIALIZER;;


//Declaration order of variables, give an overview of
//how the program works.

sem_t semaphore4First;

pthread_barrier_t barrier4First;
//OR
pthread_barrier_t barrier3;
//OR NOTHING

pthread_barrier_t barrier2A;
pthread_barrier_t barrier2B;


pthread_barrier_t barrier4Second;

pthread_barrier_t barrier4Third;



int carID=0;

//Counters
int Acount=0;
int Bcount=0;

//Flags for configuration
int OnlyOneShouldCallAFan=0;
int OnlyOneShouldBeCaptain=0;
int threadNumberBeforeReset = 0;
int TimeFor3Fans=0;



void * goGetACar(void* arg) {
    char * team= (char*)arg;
    
    sem_wait(&semaphore4First);

    pthread_mutex_lock(&mutex_obj);
    printf("Thread ID: %lu, Team: %s, I am looking for a car\n", (unsigned long)pthread_self(), team);
    pthread_mutex_unlock(&mutex_obj);

    
    
    if (strcmp(team, "A") == 0) {
        pthread_mutex_lock(&mutex_objA);
        Acount++;
        pthread_mutex_unlock(&mutex_objA);

    } else {

        pthread_mutex_lock(&mutex_objB);
        Bcount++;
        pthread_mutex_unlock(&mutex_objB);
    }



    //if we need to call only one, that thread should not wait in either of threads
    if(!OnlyOneShouldCallAFan && TimeFor3Fans){
        //barrier 3
        pthread_barrier_wait(&barrier3);
    }else if(!OnlyOneShouldCallAFan && !TimeFor3Fans){
        //barrier 4 default case
        pthread_barrier_wait(&barrier4First);
        

    }
    

    

    
    if (strcmp(team, "A") == 0) {
        pthread_barrier_wait(&barrier2A);
    } else {
        pthread_barrier_wait(&barrier2B);
    }

    


    //if we have 3-1 case, the last one that updated the counts will
    // eventually come and go inside the if block
    
    if((Acount==3 &&Bcount==1) || Acount==1 && Bcount==3){
        pthread_mutex_lock(&mutex_objSecond);
        if(!OnlyOneShouldCallAFan){
            OnlyOneShouldCallAFan=1;
            
            
            sem_post(&semaphore4First);
        }
        pthread_mutex_unlock(&mutex_objSecond);

    }
    //otherwise we would not need if check

   pthread_barrier_wait(&barrier4Second);



    //Now we gather 4 fans, satisfying the conditions
    //but Acount and Bcount includes the waiting threads at barrier.
    // We need to decrement by number of passing threads so that
    //for the next iteration our 3-1 check will be still correct

    if (strcmp(team, "A") == 0) {
        pthread_mutex_lock(&mutex_objB);
        Acount--;
        pthread_mutex_unlock(&mutex_objB);
    } else {
        pthread_mutex_lock(&mutex_objB);
        Bcount--;
        pthread_mutex_unlock(&mutex_objB);
        
    }
    



    pthread_mutex_lock(&mutex_obj);
    printf("Thread ID: %lu, Team: %s, I have found a spot in a car\n", (unsigned long)pthread_self(), team);
    pthread_mutex_unlock(&mutex_obj);

    pthread_barrier_wait(&barrier4Third);

    pthread_mutex_lock(&mutex_obj);
    if(!OnlyOneShouldBeCaptain){
        printf("Thread ID: %lu, Team: %s, I am the captain and driving the car with ID %d\n", (unsigned long)pthread_self(), team,carID);
        carID++;
        OnlyOneShouldBeCaptain=1;
        
    }
    pthread_mutex_unlock(&mutex_obj);

 

    //we want only the last thread to do the reseting action.
    pthread_mutex_lock(&mutex_objSecond);
    threadNumberBeforeReset++;
    if(threadNumberBeforeReset==4){
        //no risk because we are sure that the first 3 already pass this check
        //since it is not equal to 3.
        threadNumberBeforeReset=0;
        OnlyOneShouldBeCaptain=0;
        
        //if we call a fan that means we take action
        // but if we dont that means turn does not need special handle
        if(OnlyOneShouldCallAFan){
            //we should allow 3 at once
            //because we have 1 fans in barrier2's so
            //we allow 3 more and we look
            // if the 4 are able to pass, then pass
            //otherwise they should call one more fan
            OnlyOneShouldCallAFan=0;
            TimeFor3Fans=1;

            for(int i=0;i<3;i++){
                sem_post(&semaphore4First);
            }
                
        }else{
            TimeFor3Fans=0;
            for(int i=0;i<4;i++){
                sem_post(&semaphore4First);
            }
        }
    }
    pthread_mutex_unlock(&mutex_objSecond);

    
}



int main(int argc, char* argv[]) {
    int A = atoi(argv[1]);
    int B = atoi(argv[2]);


    if ((A % 2) != 0 || (B % 2) != 0 || (A + B) % 4 != 0) {
        printf("The main terminates\n");
        return 0;
    }

    pthread_t tids[A + B];

    
    sem_init(&semaphore4First, 0, 4);
    pthread_barrier_init(&barrier4First, NULL, 4);
    pthread_barrier_init(&barrier4Second, NULL, 4);
    pthread_barrier_init(&barrier4Third, NULL, 4);
    pthread_barrier_init(&barrier3, NULL, 3);
    pthread_barrier_init(&barrier2A, NULL, 2);
    pthread_barrier_init(&barrier2B, NULL, 2);
    





    for (int i = 0; i < A; i++) {
        pthread_create(&tids[i], NULL, goGetACar, "A");
    }

    for (int i = A; i < A + B; i++) {
        pthread_create(&tids[i], NULL, goGetACar, "B");
    }

    for (int i = 0; i < A + B; i++) {
        pthread_join(tids[i], NULL);
    }

    printf("The main terminates\n");

    return 0;
}
