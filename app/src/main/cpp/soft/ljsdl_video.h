//
// Created by wh on 2018/1/16.
//

#ifndef SPLAYER_LJSDL_VIDEO_H
#define SPLAYER_LJSDL_VIDEO_H

#include "ljsdl_mutex.h"
#include "ljsdl_vdeoframe.h"

typedef struct SDL_Overlay {
    unsigned int format;				/**< Read-only */
    int w, h;				/**< Read-only */
    int planes;				/**< Read-only */
    int pitches[4];			/**< Read-only */
    unsigned char *pixels[4];				/**< Read-write */
    //SDL_Surface* render;
    SDL_Mutex* mutex;
    VideoFrame* frame;
} SDL_Overlay;

int SDL_LockYUVOverlay(SDL_Overlay* overlay);
int SDL_UnlockYUVOverlay(SDL_Overlay* overlay);
SDL_Overlay* SDL_CreateYUVOverlay(int width, int height, int format);
int SDL_FreeYUVOverlay(SDL_Overlay* overlay);
int SDL_DisplayYUVOverlay(SDL_Overlay* overlay);

#endif //SPLAYER_LJSDL_VIDEO_H
