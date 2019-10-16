//
// Created by wh on 2018/1/16.
//
#include <malloc.h>
#include "ljsdl_video.h"
#include "ljsdl_vdeoframe.h"
#include "ijksdl_egl.h"

int SDL_LockYUVOverlay(SDL_Overlay* overlay){
    return SDL_LockMutex(overlay->mutex);
}

int SDL_UnlockYUVOverlay(SDL_Overlay* overlay){
    return SDL_UnlockMutex(overlay->mutex);
}

SDL_Overlay* SDL_CreateYUVOverlay(int width, int height, int format){

    SDL_Overlay* overlay = (SDL_Overlay*)malloc(sizeof(SDL_Overlay));
    if(NULL==overlay){
        return NULL;
    }
    overlay->format = format;				/**< Read-only */
    overlay->w = width;
    overlay->h = height;				/**< Read-only */
    overlay->planes = 3;				/**< Read-only */

    //unsigned char *buf = (unsigned char *)malloc(width*height*2);
    int length = width*height*2; //(width*height*3)/2;
    overlay->frame = new  VideoFrame();
    if(!overlay->frame){
        return NULL;
    }
    overlay->frame->VerifyAndAllocate( length );
    length = (width*height*3)/2;
    overlay->frame->SetLength( length );
    overlay->frame->SetWidth(width);
    overlay->frame->SetHeight(height);
    overlay->pixels[0] =	overlay->frame->Buffer();		/**< Read-write */
    overlay->pixels[1] =	overlay->pixels[0] + width*height;		/**< Read-write */
    overlay->pixels[2] =	overlay->pixels[1] + (width*height)/4;		/**< Read-write */
    overlay->pixels[3] =	overlay->pixels[2] + (width*height)/4;		/**< Read-write */
    overlay->pitches[0] = width;			/**< Read-only */
    overlay->pitches[1] = width/2;			/**< Read-only */
    overlay->pitches[2] = width/2;			/**< Read-only */
    overlay->pitches[3] = width/2;			/**< Read-only */

    overlay->mutex =  SDL_CreateMutex();
    if(NULL==overlay->mutex){
        return NULL;
    }
    return overlay;
}

int SDL_FreeYUVOverlay(SDL_Overlay* overlay){
    if(overlay->frame){
        overlay->frame->Free();
        delete overlay->frame;
    }

    SDL_DestroyMutex(overlay->mutex);
    free(overlay);
    return 0;
}

int SDL_DisplayYUVOverlay(SDL_Overlay* overlay){
    IJK_EGL* egl = IJK_EGL_create();
    IJK_EGL_display(egl,NULL,overlay);
}

