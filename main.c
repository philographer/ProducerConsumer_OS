#include <stdio.h> // c standard io
#include <stdlib.h> // rand function
#include <unistd.h> // sleep function
#include <pthread.h> // thread
#include <semaphore.h> // semaphore
#include "buffer.h" //buffer typedef


void *producer(void *); //producer thread function
void *consumer(void *); //consumer thread function
void *monitor(void *); //monitor thread function

int insert_item(buffer_item); //insert function
int remove_item(buffer_item *); //remove function

buffer_item buffer[BUFFER_SIZE]; //버퍼 배열
int bufferSize = 0; //버퍼의 현재(전역변수)

pthread_t thread_t;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t empty; //empty semaphore 실행순서 제어(producer<->consumer)
sem_t full; //full semaphore 실행순서 제어(producer<->consumer)
sem_t op; //open semaphore 실행순서 제어(producer,consumer <-> monitor)
sem_t cl; //close semaphore 실행순서 제어(producer,consumer <-> monitor)

int fullStat = 0; //sem_init후의 상태
int emptyStat = 0; //sem_init후의 상태
int openStat = 0; //sem_init후의 상태
int closeStat = 0; //sem_init후의 상태

int main(int argc, char * argv[]){
    
    int i;
    
    fullStat = sem_init(&full, 0, 0); //0으로 초기화
    emptyStat = sem_init(&empty, 0, 10); //10으로 초기화
    openStat = sem_init(&op, 0, 1); //1로 초기화
    closeStat = sem_init(&cl, 0, 0); //0으로 초기화
    
    int sleepTime = atoi(argv[1]); //프로그램 수행시간
    int producerNum = atoi(argv[2]); //producer 쓰레드 갯수
    int consumerNum = atoi(argv[3]); //consumer 쓰레드 갯수
    
    
    for(i=0; i < 10; i++){ //버퍼 0으로 초기화
        buffer[i] = 0;
    }
    
    
    for(i=0; i < producerNum; i++){ //입력받은 갯수만큼 producer 쓰레드 생성
        if(pthread_create(&thread_t, NULL, producer, NULL) < 0){
            perror("producer thread create error:");
            exit(0);
        }
    }
    
    
    for(i=0; i < consumerNum; i++){//입력받은 갯수만큼 consumer 쓰레드 생성
        if(pthread_create(&thread_t, NULL, consumer, NULL) < 0){
            perror("consumer thread create error:");
            exit(0);
        }
    }
    
    
    //monitor 쓰레드 생성
    if(pthread_create(&thread_t, NULL, monitor, NULL) < 0){
        perror("monitor thread create error:");
        exit(0);
    }
    
    
    sleep(sleepTime);
    
    
    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&op);
    sem_destroy(&cl);

    
    return 0;
}

void *producer(void *param){
    buffer_item item;
    
    while(1){
        sleep(rand() % 10 + 1); //랜덤시간
        item = rand() % 10 + 1; //랜덤한 아이템 생성
        if(insert_item(item))
            printf("producer report error condition\n");
        else{
            printf("insert:%d \n", item); //생성된 아이템
        }
    }
}

void *consumer(void *param){
    buffer_item item;
    while(1){
        sleep(rand() % 10 + 1); //랜덤시간
        item = buffer[bufferSize-1]; //마지막 버퍼 제거하기로 지정
        if(remove_item(&item))
            printf("consumer report error condition\n");
        else{
            printf("remove:%d\n", item); //없어진 아이템
        }
    }
}

void *monitor(void *param){
    int no = 0;
    
    while(1){
        sem_wait(&cl);
        pthread_mutex_lock(&mutex);
        printf("Acknowledge no: %d -> count==: %d \n", no, bufferSize);
        no ++;
        pthread_mutex_unlock(&mutex);
        sem_post(&op);
    }
}


int insert_item(buffer_item item){
    
    int flag = 0; //정상 리턴값
    
    sem_wait(&empty); //producer, consumer 순서 제어
    sem_wait(&op); //monitor 순서 제어
    pthread_mutex_lock(&mutex); //atomic한 연산(critical section)
    if(bufferSize < 10){
        buffer[bufferSize] = item;
        bufferSize++;
    }else{
        flag = -1; //오류시 리턴값
    }
    pthread_mutex_unlock(&mutex); //atomic한 연산
    sem_post(&cl); //monitor 순서 제어
    sem_post(&full); //producer, consumer 순서 제어
    
    return flag;
    
}
int remove_item(buffer_item *item){
    
    int flag = 0;
    
    sem_wait(&full);
    sem_wait(&op);
    pthread_mutex_lock(&mutex);
    if(bufferSize > 0){
        *item = buffer[bufferSize-1];
        buffer[bufferSize-1] = 0;
        bufferSize--;
    }else{
        flag = -1;
    }
    pthread_mutex_unlock(&mutex);
    sem_post(&cl);
    sem_post(&empty);
    
    return flag;
}
