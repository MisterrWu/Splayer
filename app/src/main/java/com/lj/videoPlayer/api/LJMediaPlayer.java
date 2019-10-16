package com.lj.videoPlayer.api;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.sina.sinavideo.coreplayer.splayer.MediaPlayer;

import java.io.IOException;
import java.lang.ref.WeakReference;

/**
 * Created by wh on 2018/1/2.
 */

public class LJMediaPlayer {

    static {
        System.loadLibrary("splayer");
        native_init();
    }

    public LJMediaPlayer(){
        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else {
            mEventHandler = null;
        }
        native_setup(new WeakReference<LJMediaPlayer>(this));
    }

    @Override
    protected void finalize() {
        native_finalize();
    }

    private static native final void native_init();
    private native final void native_setup(Object mediaplayer_this);
    private native final void native_finalize();

    public native void setDataSource(String path)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    private native void _setVideoSurface(Surface surface);

    public native void prepare() throws IOException, IllegalStateException;

    public native void prepareAsync() throws IllegalStateException;

    private native void native_start() throws IllegalStateException;

    private native void native_stop() throws IllegalStateException;

    private native void native_pause() throws IllegalStateException;

    public native int getVideoWidth();

    public native int getVideoHeight();

    public native float getVideoAspectRatio();

    public native boolean isPlaying();

    public native void seekTo(int msec) throws IllegalStateException;

    public native int getCurrentPosition();

    public native int getDuration();

    private native void native_release();

    private native void native_reset();

    public native void setLooping(boolean looping);

    public native boolean isLooping();

    public native boolean isBuffering();

    public native void setBufferSize(int msec);

    public native void setVolume(float leftVolume, float rightVolume);

    public native void config(String cfg, int val) throws IllegalStateException;

    public void setDisplay(SurfaceHolder sh){
        Surface surface;
        if (sh != null) {
            surface = sh.getSurface();
        } else {
            surface = null;
        }
        _setVideoSurface(surface);
    }

    public void setSurface(Surface surface){
        _setVideoSurface(surface);
    }

    public  void start() throws IllegalStateException {
        //stayAwake(true);
        native_start();
    }

    public void stop() throws IllegalStateException {
        //stayAwake(false);
        native_stop();
    }

    public void pause() throws IllegalStateException {
        ///stayAwake(false);
        native_pause();
    }

    public void release() {
        stayAwake(false);
        //updateSurfaceScreenOn();
        /*mOnPreparedListener = null;
        mOnBufferingUpdateListener = null;
        mOnCompletionListener = null;
        mOnSeekCompleteListener = null;
        mOnErrorListener = null;
        mOnInfoListener = null;
        mOnVideoSizeChangedListener = null;
        mOnTimedTextListener = null;*/
        native_release();
    }

    public void reset() {
        stayAwake(false);
        native_reset();
        // make sure none of the listeners get called anymore
        mEventHandler.removeCallbacksAndMessages(null);
    }

    @SuppressLint("WakelockTimeout")
    public void setWakeMode(Context context, int mode) {
        boolean washeld = false;
        if (mWakeLock != null) {
            if (mWakeLock.isHeld()) {
                washeld = true;
                mWakeLock.release();
            }
            mWakeLock = null;
        }

        PowerManager pm = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
        if(pm == null)return;
        mWakeLock = pm.newWakeLock(mode|PowerManager.ON_AFTER_RELEASE, MediaPlayer.class.getName());
        mWakeLock.setReferenceCounted(false);
        if (washeld) {
            mWakeLock.acquire();
        }
    }

    private void stayAwake(boolean awake) {
        if (mWakeLock != null) {
            if (awake && !mWakeLock.isHeld()) {
                mWakeLock.acquire();
            } else if (!awake && mWakeLock.isHeld()) {
                mWakeLock.release();
            }
        }
        mStayAwake = awake;
    }

    private EventHandler mEventHandler;
    private PowerManager.WakeLock mWakeLock = null;
    private boolean mStayAwake;
    private int mNativeContext;  // accessed by native methods
    private static final String TAG="LJMediaPlayer";

    private static void postEventFromNative(Object mediaplayer_ref,
                                            int what, int arg1, int arg2, Object obj)
    {
        LJMediaPlayer mp = (LJMediaPlayer)((WeakReference)mediaplayer_ref).get();
        if (mp == null) {
            return;
        }

        if (mp.mEventHandler != null) {
            Message m = mp.mEventHandler.obtainMessage(what, arg1, arg2, obj);
            mp.mEventHandler.sendMessage(m);
        }
    }

    private class EventHandler extends Handler {

        private LJMediaPlayer mLjMediaPlayer;

        EventHandler(LJMediaPlayer mp,Looper looper){
            super(looper);
            mLjMediaPlayer=mp;
        }
        @Override
        public void handleMessage(Message msg) {
            if (mLjMediaPlayer.mNativeContext == 0) {
                Log.w(TAG, "mediaplayer went away with unhandled events");
                return;
            }
            switch (msg.what){

            }
        }
    }
}
