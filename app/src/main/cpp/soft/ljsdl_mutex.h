//
// Created by wh on 2018/1/15.
//

#ifndef SPLAYER_LJSDL_MUTEX_H
#define SPLAYER_LJSDL_MUTEX_H

#include <pthread.h>

///mutex
#define SDL_Mutex pthread_mutex_t

SDL_Mutex* SDL_CreateMutex();

int SDL_LockMutex(SDL_Mutex* mtx);

int SDL_UnlockMutex(SDL_Mutex* mtx);

int SDL_DestroyMutex(SDL_Mutex* mtx);

#define SDL_Cond pthread_cond_t

SDL_Cond* SDL_CreateCond();

int SDL_SignalCond(SDL_Cond* cond);

int SDL_WaitCond(SDL_Cond* cond, SDL_Mutex* mtx);

int SDL_TimeWaitCond(SDL_Cond* cond, SDL_Mutex* mtx, int ms);

int SDL_DestroyCond(SDL_Cond* cond);

int SDL_Delay(int ms);

#endif //SPLAYER_LJSDL_MUTEX_H
