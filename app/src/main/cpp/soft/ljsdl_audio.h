//
// Created by wh on 2018/1/16.
//

#ifndef SPLAYER_LJSDL_AUDIO_H
#define SPLAYER_LJSDL_AUDIO_H

///aduio
#include "ljsdl_opensles.h"

#define SDL_AUDIO_S16 16
typedef struct SDL_AudioSpec{
    int channels; ///Number of channels: 1 mono, 2 stereo
    int freq;
    int format; ///Audio data format
    int silence; ///Audio buffer silence value (calculated)
    int samples; ///Audio buffer size in samples (power of 2)
    void (*callback)(void *opaque, char *stream, int len);
    void *userdata;
    int size; ///Audio buffer size in samples (in bytes)
}SDL_AudioSpec;

struct SDL_Aout {
    SDL_Mutex* mtx;
    AndroidOpenSLES* android_opensles;
};

int SDL_OpenAudio(SDL_Aout* aout,SDL_AudioSpec* wanted_spec, SDL_AudioSpec* spec);
int SDL_PauseAudio(SDL_Aout* aout,int arg); //play voice
int SDL_CloseAudio(SDL_Aout* aout);

#endif //SPLAYER_LJSDL_AUDIO_H
