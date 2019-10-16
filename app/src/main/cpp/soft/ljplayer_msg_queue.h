//
// Created by wh on 2018/1/15.
//

#ifndef SPLAYER_LJPLAYER_MSG_QUEQUE_H
#define SPLAYER_LJPLAYER_MSG_QUEQUE_H

#include <list>
#include "ljsdl_mutex.h"

//event
#define SDL_ALLEVENTS 					0
#define SDL_ALLOC_EVENT   				1
#define SDL_QUIT_EVENT    				2
#define SDL_REFRESH_EVENT 				3
#define SDL_PAUSE_EVENT   				4
#define SDL_SEEK_EVENT    				5
#define SDL_RESET_EVENT    				6
#define SDL_SET_VOLUME_EVENT    		7

typedef struct AVMessage{
    int type;
    void *data;
}AVMessage;

typedef struct MessageQueue{
    std::list<AVMessage> msg_queue;
    SDL_Mutex* mtx;
}MessageQueue;

inline static int msg_queue_put_private(MessageQueue* q,AVMessage *msg)
{
    if(!q){
        return -1;
    }
    if(!msg){
        return -1;
    }
    AVMessage e;
    e.type = msg->type;
    e.data = msg->data;

    if(NULL==q->mtx){
        q->mtx =SDL_CreateMutex();
        if(NULL==q->mtx){
            return -1;
        }
    }
    SDL_LockMutex(q->mtx);
    q->msg_queue.push_back( e );
    SDL_UnlockMutex(q->mtx);
    return 0;
}

inline static int msg_queue_get(MessageQueue* q,AVMessage *msg, int type)
{
    if(!q){
        return -1;
    }
    if(!msg){
        return -1;
    }
    if(NULL==q->mtx){
        q->mtx = SDL_CreateMutex();
        if(NULL==q->mtx){
            return -1;
        }
    }
    SDL_LockMutex(q->mtx);

    if(0==type){
        if(!q->msg_queue.empty()){
            msg->type  = q->msg_queue.front().type;
            msg->data  =  q->msg_queue.front().data;
            q->msg_queue.pop_front( );

            SDL_UnlockMutex(q->mtx);
            return 1;
        }
    }else{
        std::list<AVMessage>::iterator it;
        for (it=q->msg_queue.begin(); it!=q->msg_queue.end(); it++){
            if(type==(*it).type){
                msg->type  =  (*it).type;
                msg->data  =  (*it).data;
                q->msg_queue.erase( it );
                SDL_UnlockMutex(q->mtx);
                return 1;
            }
        }
    }
   SDL_UnlockMutex(q->mtx);
    return 0;
}

#endif //SPLAYER_LJPLAYER_MSG_QUEQUE_H
