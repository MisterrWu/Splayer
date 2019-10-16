//
// Created by wh on 2018/1/16.
//
#include "ljsdl_audio.h"

int SDL_OpenAudio(SDL_Aout* aout,SDL_AudioSpec* wanted_spec, SDL_AudioSpec* spec){
    int res;

    spec->channels = 1; ///Number of channels: 1 mono, 2 stereo
    spec->freq = 16000;
    spec->format = SDL_AUDIO_S16; ///Audio data format
    spec->silence = 0; ///Audio buffer silence value (calculated)
    spec->samples = 160*3; ///3 of 10ms 16000HZ sample rate
    spec->callback = wanted_spec->callback;
    spec->userdata = wanted_spec->userdata;
    spec->size = 2*spec->channels*spec->samples; ///Audio buffer size in samples (in bytes)

    if(NULL==aout->android_opensles)
        aout->android_opensles = new  AndroidOpenSLES(0);

    aout->android_opensles->AttachAudioBuffer(spec->callback,spec->userdata);

    res = aout->android_opensles->Init();
    if(0!=res){
        return -1;
    }
    //bool ok;
    res = aout->android_opensles->InitPlayout();
    if(0!=res){
        return -1;
    }
    return 0;
}

int SDL_PauseAudio(SDL_Aout* aout,int start){
    if(!aout->android_opensles){
        return -1;
    }
    if(start){
        return aout->android_opensles->StartPlayout();
    }else{
        return aout->android_opensles->StopPlayout();
    }
} //play voice

int SDL_CloseAudio(SDL_Aout* aout){
    if(aout->android_opensles){
        aout->android_opensles->StopPlayout();
        aout->android_opensles->Terminate();
        delete aout->android_opensles;
        aout->android_opensles = NULL;
    }
    return 0;
}