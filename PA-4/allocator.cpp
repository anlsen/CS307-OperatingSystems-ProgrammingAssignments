#include <pthread.h>
#include <iostream>
#include <unistd.h>


using namespace std;






class HeapManager{

private:
   
    struct HeapListNode{
        int id;
        int size;
        int index;
        HeapListNode *next;
        HeapListNode(int id,int size,int index){
            this->id=id;
            this->size=size;
            this->index=index;
            next=nullptr;
        }
    };

    HeapListNode* head= nullptr;
    pthread_mutex_t heapLock;
    pthread_mutex_t printLock;

public:

    HeapManager() { //initialize the lock
        heapLock = PTHREAD_MUTEX_INITIALIZER;
        printLock = PTHREAD_MUTEX_INITIALIZER;
        
    }

    ~HeapManager() { //initialize the lock
        HeapListNode* traverser=head;
        cout<<"Execution is done"<<endl;
        print();
        while(traverser){
            HeapListNode* toBeDeleted=traverser;
            traverser=traverser->next;
            delete toBeDeleted;
        }
        
    }


    int initHeap(int size){
        head= new HeapListNode(-1,size,0);
        cout<<"Memory initialized"<<endl;
        print();
        return 1;
    }

    void print(){
        pthread_mutex_lock(&printLock);

        HeapListNode* traverser=head;
        while(traverser){
            cout<<"["<<traverser->id<<"]"<<"["<<traverser->size<<"]"<<"["<<traverser->index<<"]";
            traverser=traverser->next;
            if(traverser){
                cout<<"---";
            }
        }
        cout<<endl;

        pthread_mutex_unlock(&printLock);
    }
    int myMalloc(int ID, int size){
        pthread_mutex_lock(&heapLock);
        HeapListNode* traverser=head;
        while(traverser){
            if(traverser->id==-1 && traverser->size >= size){

                if(traverser->size==size){
                    //if the size is perfectly fit
                    // no need to divide into two
                    traverser->id=ID;
                }else{
                    //oneAfter is for the other part after dividing into two
                    //for the remaining free space
                    HeapListNode* oneAfter=traverser->next;
                    int remainingSize=traverser->size - size;
                    int newIndex=traverser->index + size;

                    traverser->id=ID;
                    traverser->size=size;

                    traverser->next=new HeapListNode(-1,remainingSize,newIndex);
                    traverser->next->next=oneAfter;

                    cout<<"Allocated for thread "<<ID<<endl;
                    print();
                    pthread_mutex_unlock(&heapLock);
                    return traverser->index;

                }
                

            }else{
                traverser=traverser->next;
            }
        }
        cout<<"Can not allocate, requested size "<<size<<" for thread "<<ID<<" is bigger than the remaining size."<<endl;
        print();
        pthread_mutex_unlock(&heapLock);
        return -1;
    }

    int myFree(int ID,int index){
        pthread_mutex_lock(&heapLock);

        HeapListNode* traverser=head;

        HeapListNode* previous=nullptr;
        while(traverser){
            
            if(traverser->id==ID && traverser->index==index){
                //we found the block, so need to deallocate. But
                //While deallocating we also need to check previous and next
                //in order to perform coalescing
                bool aheadFree=false;
                bool backFree=false;

                HeapListNode* next=traverser->next;

                if(next && next->id==-1){
                    aheadFree=true;
                }
                if(previous && previous->id ==-1){
                    backFree=true;
                }
                
                if(backFree&& aheadFree){
                    // they should gather up at previous
                    previous->id=-1;
                    previous->size+=next->size + traverser->size;
                    previous->next=next->next;

                    //now delete traverser and next
                    delete traverser;
                    delete next;
                }else if(backFree){
                    //they should gather up at previous
                    previous->id=-1;
                    previous->size+=traverser->size;
                    previous->next=traverser->next;

                    delete traverser;
                }else if(aheadFree){
                    //they should gather up at traverser
                    traverser->id=-1;
                    traverser->size+=next->size;
                    traverser->next=next->next;

                    delete next;
                    
                }else{
                    //none of them is (either back or front) free so just adjust the current node as free
                    traverser->id=-1;
                }
                cout<<"Freed for thread "<<ID<<endl;
                print();
                pthread_mutex_unlock(&heapLock);
                return 1;

            }else{
                previous=traverser;
                traverser=traverser->next;
            }


        }
        cout << "No matching node to free for the thread " << ID << endl;
        print();
        pthread_mutex_unlock(&heapLock);
        return -1;
    }
};