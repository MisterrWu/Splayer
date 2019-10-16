//
// Created by wh on 2018/1/2.
//
#include <assert.h>
#include "ljplayer_jni.h"
#include "ljmediaplayer.h"

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define LJPlayer_Success 0;
#define LJPlayer_Fail -1;

#ifdef __cplusplus
extern "C" {
#endif

int jniRegisterNativeMethods(JNIEnv* env, const char* className,
                             const JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;

    clazz = env->FindClass( className);
    if (clazz == NULL) {
        return LJPlayer_Fail;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return LJPlayer_Fail;
    }
    return LJPlayer_Success;
}

int jniThrowException(JNIEnv* env, const char* className, const char* msg)
{
    jclass exceptionClass;

    exceptionClass = env->FindClass( className);
    if (exceptionClass == NULL) {
        assert(0);      /* fatal during dev; should always be fatal? */
        return LJPlayer_Fail;
    }

    if (env->ThrowNew( exceptionClass, msg) != JNI_OK) {
        assert(!"failed to throw");
    }
    return LJPlayer_Success;
}

#ifdef __cplusplus
}
#endif

static const char* const kMPClassPathName 					= 	"com/lj/videoPlayer/api/LJMediaPlayer";
static SDL_Mutex* g_mp_lock = NULL;

struct fields_t {
    jfieldID    context;
    //jfieldID    surface_texture;
    jmethodID   post_event;
};


static fields_t fields;
static JavaVM* g_java_vm = NULL;

// --------------------------------LJMediaPlayerListener--------------------------------------------
LJMediaPlayerListener::LJMediaPlayerListener(JavaVM* jvm_, jobject thiz, jobject weak_thiz) {
    // Hold onto the LJMediaPlayer class for use in calling the static method
    // that posts events to the application thread.
    jvm = jvm_;
    bool isAttached = false;
    JNIEnv* env = NULL;
    if (jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        jint res = jvm->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env) {
            return ;
        }
        isAttached = true;
    }

    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        jniThrowException(env, "java/lang/Exception", NULL);
        return;
    }
    mClass = (jclass)env->NewGlobalRef(clazz);

    // We use a weak reference so the LJMediaPlayer object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    mObject  = env->NewGlobalRef(weak_thiz);

    if (isAttached) {
        if (jvm->DetachCurrentThread() < 0) {
        }
    }
}

LJMediaPlayerListener::~LJMediaPlayerListener() {
    // remove global references
    bool isAttached = false;
    JNIEnv* env = NULL;
    if (jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        jint res = jvm->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env) {
            return ;
        }
        isAttached = true;
    }

    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);

    if (isAttached) {
        jvm->DetachCurrentThread();
    }
}

void LJMediaPlayerListener::notify(int msg, int ext1, int ext2, void *obj) {
    bool isAttached = false;
    JNIEnv* env = NULL;
    if (jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        jint res = jvm->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env) {
            return ;
        }
        isAttached = true;
    }

    env->CallStaticVoidMethod(mClass, fields.post_event, mObject, msg, ext1, ext2, NULL);

    if (isAttached) {
        jvm->DetachCurrentThread();
    }
}

// --------------------------------LJMediaPlayer--------------------------------------------
static LJMediaPlayer* getMediaPlayer(JNIEnv* env, jobject thiz)
{
    if(g_mp_lock)SDL_LockMutex(g_mp_lock);
    LJMediaPlayer* p = (LJMediaPlayer*)env->GetIntField(thiz, fields.context);
    if(g_mp_lock)SDL_UnlockMutex(g_mp_lock);
    return p;
}

static LJMediaPlayer* setMediaPlayer(JNIEnv* env, jobject thiz, LJMediaPlayer* context)
{
    if(g_mp_lock)SDL_LockMutex(g_mp_lock);
    LJMediaPlayer* old = (LJMediaPlayer*)env->GetIntField(thiz, fields.context);
    env->SetIntField(thiz, fields.context, (int)context);
    if(g_mp_lock)SDL_UnlockMutex(g_mp_lock);
    return old;
}

// If exception is NULL and opStatus is not OK, this method sends an error
// event to the client application; otherwise, if exception is not NULL and
// opStatus is not OK, this method throws the given exception to the client
// application.
static void process_media_player_call(LJMediaPlayer* mp,JNIEnv *env, jobject thiz, int opStatus)
{
    if ( opStatus == LJPLAYER_ERROR_SPALYER_STATE ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
    } else if ( opStatus == LJPLAYER_ERROR_NO_MEMORY ) {
        jniThrowException(env, "java/lang/RuntimeException", NULL);
    } else if ( opStatus == LJPLAYER_ERROR_PARAMETER ) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
    } else if ( opStatus == LJPLAYER_ERROR_NETWORK ) {
        //LJMediaPlayer* mp = getMediaPlayer(env, thiz);
        if (mp != 0) {
            mp->notify(MEDIA_ERROR, MEDIA_ERROR_SERVER_DIED,MEDIA_ERROR_IO,0);
        }
    } else if ( opStatus == LJPLAYER_ERROR_UNSUPPORTED ) {
        //LJMediaPlayer* mp = getMediaPlayer(env, thiz);
        if (mp != 0) {
            mp->notify(MEDIA_ERROR, MEDIA_ERROR_SERVER_DIED,MEDIA_ERROR_UNSUPPORTED,0);
        }
    } else if ( opStatus == LJPLAYER_ERROR_NORMAL ) {
        //LJMediaPlayer* mp = getMediaPlayer(env, thiz);
        if (mp != 0) {
            mp->notify(MEDIA_ERROR, MEDIA_ERROR_SERVER_DIED,MEDIA_ERROR_UNSUPPORTED,0);
        }
    } else if ( opStatus == LJPLAYER_ERROR_INTERNAL ) {
        //LJMediaPlayer* mp = getMediaPlayer(env, thiz);
        if (mp != 0) {
            mp->notify(MEDIA_ERROR, MEDIA_ERROR_SERVER_DIED,MEDIA_ERROR_MALFORMED,0);
        }
    } else if ( opStatus == LJPLAYER_ERROR_TIMEDOUT ) {
        //LJMediaPlayer* mp = getMediaPlayer(env, thiz);
        if (mp != 0) {
            mp->notify(MEDIA_ERROR, MEDIA_ERROR_SERVER_DIED,MEDIA_ERROR_TIMED_OUT,0);
        }
    } else if ( opStatus != LJPLAYER_SUCCESS ){
        //LJMediaPlayer* mp = getMediaPlayer(env, thiz);
        if (mp != 0) {
            mp->notify(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, 0,0);
        }
    }
}


static void android_media_MediaPlayer_native_init(JNIEnv *env,jobject thiz)
{
    jclass clazz;

    clazz = env->FindClass(kMPClassPathName);
    if (clazz == NULL) {
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "I");
    if (fields.context == NULL) {
        return;
    }

    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                               "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        return;
    }

    if(NULL==g_mp_lock){
        g_mp_lock = SDL_CreateMutex();
    }
}


static void android_media_MediaPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this, jint hard)
{
    int ret;
    LJMediaPlayer* mp;
    mp = LJMediaPlayer::Create();

    if(NULL==mp){
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }
    ret = mp->setup(g_java_vm,thiz, weak_this);
    // Stow our new C++ LJMediaPlayer in an opaque field in the Java object.
    if(LJPLAYER_SUCCESS==ret){
        setMediaPlayer(env, thiz, mp);
    }

    process_media_player_call(mp, env, thiz, ret);
}

static void android_media_MediaPlayer_setDataSource(
        JNIEnv *env, jobject thiz, jstring path) {

    int ret;

    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if (path == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }

    const char *tmp = env->GetStringUTFChars(path, NULL);
    if (tmp == NULL) {  // Out of memory
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }
    ret = mp->set_datasource(tmp);

    env->ReleaseStringUTFChars(path, tmp);
    tmp = NULL;

    process_media_player_call(mp,env, thiz, ret);
}

static void android_media_MediaPlayer_setSurface(JNIEnv *env, jobject thiz, jobject jsurface)
{
    int ret;

    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    ret = mp->set_surface(g_java_vm,jsurface);

    process_media_player_call(mp, env, thiz, ret);
}

static void android_media_MediaPlayer_prepare(JNIEnv *env, jobject thiz)
{
    int ret;

    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    ret = mp->prepare();

    process_media_player_call( mp, env, thiz,ret );
}

static void android_media_MediaPlayer_prepareAsync(JNIEnv *env, jobject thiz)
{
    int ret ;
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    ret = mp->prepareAsync();

    process_media_player_call( mp, env, thiz,ret );
}

static void android_media_MediaPlayer_start(JNIEnv *env, jobject thiz)
{
    int ret;
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    ret = mp->start();

    process_media_player_call( mp, env, thiz, ret );
}

static void android_media_MediaPlayer_stop(JNIEnv *env, jobject thiz)
{
    int ret;
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    ret = mp->stop();

    process_media_player_call( mp, env, thiz, ret );
}

static void android_media_MediaPlayer_pause(JNIEnv *env, jobject thiz)
{
    int ret;
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    ret = mp->pause();

    process_media_player_call( mp, env, thiz, ret );
}

static jboolean android_media_MediaPlayer_isPlaying(JNIEnv *env, jobject thiz)
{
    int ret;

    LJMediaPlayer* mp = getMediaPlayer(env, thiz);

    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return (jboolean) false;
    }
    jboolean is_playing;

    is_playing = (jboolean) mp->is_playing();

    return is_playing;
}

static void android_media_MediaPlayer_seekTo(JNIEnv *env, jobject thiz, int msec)
{
    int ret;
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    ret = mp->seekto(msec);

    process_media_player_call( mp, env, thiz, ret );
}

static int android_media_MediaPlayer_getVideoWidth(JNIEnv *env, jobject thiz)
{
    int ret;

    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int w = mp->get_video_width();

    return w;
}

static int android_media_MediaPlayer_getVideoHeight(JNIEnv *env, jobject thiz)
{
    int ret;

    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int h = mp->get_video_height();

    return h;
}


static float android_media_MediaPlayer_getVideoAspectRatio(JNIEnv *env, jobject thiz)
{
    int ret;

    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    float v = mp->get_video_aspect_ratio();

    return v;
}


static int android_media_MediaPlayer_getCurrentPosition(JNIEnv *env, jobject thiz)
{
    int ret;

    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        ///应该去掉抛出异常?还是上面捕捉?
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int msec = mp->get_current_position();

    return msec;
}

static int android_media_MediaPlayer_getDuration(JNIEnv *env, jobject thiz)
{
    int ret;
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int msec =	mp->get_duration();

    return msec;
}

static void android_media_MediaPlayer_reset(JNIEnv *env, jobject thiz)
{
    int ret;

    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    ret = mp->reset();

    process_media_player_call(mp, env, thiz, ret );
}

static void android_media_MediaPlayer_setLooping(JNIEnv *env, jobject thiz, jboolean looping)
{
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    int ret = mp->set_looping(looping);

    process_media_player_call(mp,  env, thiz, ret );
}

static jboolean android_media_MediaPlayer_isLooping(JNIEnv *env, jobject thiz)
{
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return (jboolean) false;
    }
    jboolean islooping = (jboolean) mp->is_looping();

    return islooping;
}

static void android_media_MediaPlayer_release(JNIEnv *env, jobject thiz)
{
    int ret ;

    LJMediaPlayer* mp = setMediaPlayer(env, thiz, 0);
    if (mp != NULL) {
        ret = mp->release();
        LJMediaPlayer::Destroy(mp);
        //process_media_player_call(mp,  env, thiz, ret );
    }
}

static void android_media_MediaPlayer_native_finalize(JNIEnv *env, jobject thiz)
{
    android_media_MediaPlayer_release(env, thiz);
}


static void android_media_MediaPlayer_config(JNIEnv * env,jobject thiz,jstring cfg,jint val)
{
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    const char* cfg_ = env->GetStringUTFChars( cfg , NULL);
    if(cfg_==NULL){
        return ;
    }
    int ret = mp->config(cfg_,val);

    env->ReleaseStringUTFChars(cfg, cfg_);
    //process_media_player_call(mp,  env, thiz, ret );
}

static jboolean android_media_MediaPlayer_isBuffering(JNIEnv *env, jobject thiz)
{
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return (jboolean) false;
    }
    jboolean isbuffering = (jboolean) mp->is_buffering();

    return isbuffering;
}

static void android_media_MediaPlayer_setBufferSize(JNIEnv *env, jobject thiz, jint buf_size)
{
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if(buf_size>=0){
        mp->config("buffer",buf_size);
    }
}


static void android_media_MediaPlayer_setVolume(JNIEnv *env, jobject thiz, jfloat left, jfloat right)
{
    LJMediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    mp->setVolume(left, right);
}


// ----------------------------------------------------------------------------
//  [Ljava/lang/String; String数组
static JNINativeMethod gMethods[] = {
        {"native_init",         "()V",                               (void *)android_media_MediaPlayer_native_init},
        {"native_setup",        "(Ljava/lang/Object;I)V",          (void *)android_media_MediaPlayer_native_setup},
        {"native_finalize",     "()V",                              (void *)android_media_MediaPlayer_native_finalize},
        {"setDataSource", 		 "(Ljava/lang/String;)V",           (void *)android_media_MediaPlayer_setDataSource},
        {"_setVideoSurface",   "(Landroid/view/Surface;)V",      (void *)android_media_MediaPlayer_setSurface},
        {"prepare",             "()V",                              (void *)android_media_MediaPlayer_prepare},
        {"prepareAsync",        "()V",                              (void *)android_media_MediaPlayer_prepareAsync},
        {"native_start",        "()V",                              (void *)android_media_MediaPlayer_start},
        {"native_stop",         "()V",                              (void *)android_media_MediaPlayer_stop},
        {"getVideoWidth",       "()I",                              (void *)android_media_MediaPlayer_getVideoWidth},
        {"getVideoHeight",      "()I",                              (void *)android_media_MediaPlayer_getVideoHeight},
        {"getVideoAspectRatio", "()F",                              (void *)android_media_MediaPlayer_getVideoAspectRatio},
        {"seekTo",              "(I)V",                             (void *)android_media_MediaPlayer_seekTo},
        {"native_pause",        "()V",                              (void *)android_media_MediaPlayer_pause},
        {"isPlaying",           "()Z",                              (void *)android_media_MediaPlayer_isPlaying},
        {"getCurrentPosition",  "()I",                              (void *)android_media_MediaPlayer_getCurrentPosition},
        {"getDuration",         "()I",                              (void *)android_media_MediaPlayer_getDuration},
        {"native_release",      "()V",                              (void *)android_media_MediaPlayer_release},
        {"native_reset",        "()V",                              (void *)android_media_MediaPlayer_reset},
        {"setLooping",          "(Z)V",                             (void *)android_media_MediaPlayer_setLooping},
        {"isLooping",           "()Z",                              (void *)android_media_MediaPlayer_isLooping},
        {"isBuffering",         "()Z",                              (void *)android_media_MediaPlayer_isBuffering},
        {"setBufferSize",       "(I)V",                             (void *)android_media_MediaPlayer_setBufferSize},
        {"setVolume",       	"(FF)V",                            (void *)android_media_MediaPlayer_setVolume},
        {"config",           	"(Ljava/lang/String;I)V",           (void *)android_media_MediaPlayer_config},
};


// This function only registers the native methods
static int register_android_media_MediaPlayer(JNIEnv *env)
{
    return jniRegisterNativeMethods(env,kMPClassPathName, gMethods, NELEM(gMethods));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if(vm==NULL){
        return result;
    }
    g_java_vm = vm;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        goto bail;
    }
    assert(env != NULL);

    if (register_android_media_MediaPlayer(env) < 0) {
        goto bail;
    }

    result = JNI_VERSION_1_4;

    bail:
    return result;
}


void JNI_OnUnload(JavaVM* vm, void* reserved)
{
    if(g_mp_lock!=NULL){
        SDL_DestroyMutex(g_mp_lock);
        g_mp_lock = NULL;
    }
}
