//
// Created by wh on 2018/1/15.
//
#include <malloc.h>
#include "ljsdl_mutex.h"

SDL_Mutex* SDL_CreateMutex(){
    SDL_Mutex* mtx= (SDL_Mutex *) malloc(sizeof(SDL_Mutex));
    if(mtx==NULL){
        return NULL;
    }
    int res = pthread_mutex_init(mtx,NULL);
    if(0!=res){
        return NULL;
    }
    return mtx;
}

int SDL_LockMutex(SDL_Mutex* mtx)
{
    return pthread_mutex_lock(mtx);
}

int SDL_UnlockMutex(SDL_Mutex* mtx)
{
    return pthread_mutex_unlock(mtx);
}

int SDL_DestroyMutex(SDL_Mutex* mtx)
{
    if(mtx){
        pthread_mutex_destroy(mtx);
        free(mtx);
        return 0;
    }
    return -1;
}

SDL_Cond* SDL_CreateCond()
{
    SDL_Cond* cond = (SDL_Cond*)malloc(sizeof(SDL_Cond));
    if(cond==NULL){
        return NULL;
    }
    int res = pthread_cond_init(cond,NULL);
    if(0!=res){
        return NULL;
    }
    return cond;
}

int SDL_SignalCond(SDL_Cond* cond)
{
    return pthread_cond_signal(cond);
}

int SDL_WaitCond(SDL_Cond* cond, SDL_Mutex* mtx)
{
    return pthread_cond_wait(cond, mtx);
}

int SDL_TimeWaitCond(SDL_Cond* cond, SDL_Mutex* mtx, int ms)
{
    int retval;
    struct timeval delta;
    struct timespec abstime;
    gettimeofday(&delta, NULL);
    abstime.tv_sec = delta.tv_sec + (ms/1000);
    abstime.tv_nsec = (delta.tv_usec + (ms%1000) * 1000) * 1000;
    if ( abstime.tv_nsec > 1000000000 ) {
        abstime.tv_sec += 1;
        abstime.tv_nsec -= 1000000000;
    }

    tryagain:
    retval = pthread_cond_timedwait(cond, mtx, &abstime);
    switch (retval) {
        case EINTR:
            goto tryagain;
            break;
        case ETIMEDOUT:
            retval = 1;
            break;
        case 0:
            break;
        default:
            retval = -1;
            break;
    }
    return retval;
}


int SDL_DestroyCond(SDL_Cond* cond)
{
    if(cond){
        pthread_cond_destroy(cond);
        free(cond);
        return 0;
    }
    return -1;
}

int SDL_Delay(int ms)
{
    int retval;
    struct timeval delta;
    struct timespec abstime;
    pthread_cond_t cond;
    pthread_mutex_t mutex;

    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);

    gettimeofday(&delta, NULL);

    abstime.tv_sec = delta.tv_sec + (ms/1000);
    abstime.tv_nsec = (delta.tv_usec + (ms%1000) * 1000) * 1000;
    if ( abstime.tv_nsec > 1000000000 ) {
        abstime.tv_sec += 1;
        abstime.tv_nsec -= 1000000000;
    }
    tryagain:
    pthread_mutex_lock(&mutex);
    retval = pthread_cond_timedwait(&cond, &mutex, &abstime);
    pthread_mutex_unlock(&mutex);
    switch (retval) {
        case EINTR:
            goto tryagain;
            break;
        case ETIMEDOUT:
            retval = 1;
            break;
        case 0:
            break;
        default:
            retval = -1;
            break;
    }
    return retval;
}