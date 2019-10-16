//
// Created by wh on 2018/1/15.
//

#ifndef SPLAYER_LJSDL_THREAD_H
#define SPLAYER_LJSDL_THREAD_H

#include <pthread.h>

#define SDL_Thread pthread_t

SDL_Thread SDL_CreateThread( void* (*run)(void *arg), void *data); //return thread id
int SDL_WaitThread(SDL_Thread id, void ** ret_val);

#endif //SPLAYER_LJSDL_THREAD_H
