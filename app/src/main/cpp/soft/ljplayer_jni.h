//
// Created by wh on 2018/1/2.
//

#ifndef SPLAYER_LJPLAYER_H
#define SPLAYER_LJPLAYER_H

#include <jni.h>

class LJMediaPlayerListener{

public:
    LJMediaPlayerListener(JavaVM* jvm_, jobject thiz, jobject weak_thiz);
    ~LJMediaPlayerListener();
    void notify(int msg, int ext1, int ext2, void *obj );

private:
    jclass      mClass;     // Reference to MediaPlayer class
    jobject     mObject;    // Weak ref to MediaPlayer Java object to call on
    JavaVM* 	jvm;
};

#endif //SPLAYER_LJPLAYER_H
