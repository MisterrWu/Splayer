//
// Created by wh on 2018/1/15.
//
#include "ljsdl_thread.h"
//thread id should not equal 0
SDL_Thread SDL_CreateThread( void* (*run)(void *arg), void *data)
{
    int err;
    SDL_Thread id = 0;
    err = pthread_create(&id,NULL,run,data);
    if(err != 0){
        return 0;
    }
    return id;
}


int SDL_WaitThread(SDL_Thread id, void ** ret_val)
{
    return pthread_join(id, ret_val);
}