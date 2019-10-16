package com.splayer.hard;

import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.media.MediaPlayer;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.provider.MediaStore;
import android.support.constraint.ConstraintLayout;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.TextureView;

import com.sina.sinavideo.coreplayer.splayer.VideoView;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    private static final int REQUEST_VIDEO_CODE =0x011;
    private VideoView mVideoView;
    MediaPlayer mediaPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ConstraintLayout constraintLayout= (ConstraintLayout) findViewById(R.id.content);
        mVideoView=new VideoView(this,mHandler);
        constraintLayout.addView(mVideoView,new ConstraintLayout.LayoutParams(
                ConstraintLayout.LayoutParams.MATCH_PARENT,ConstraintLayout.LayoutParams.MATCH_PARENT));
        Intent intent = new Intent(Intent.ACTION_PICK, android.provider.MediaStore.Video.Media.EXTERNAL_CONTENT_URI);
        startActivityForResult(intent, REQUEST_VIDEO_CODE);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if(requestCode == REQUEST_VIDEO_CODE && resultCode == RESULT_OK){
            Uri uri=data.getData();
            if(uri == null)return;
            ContentResolver cr = this.getContentResolver();
            /** 数据库查询操作。
             * 第一个参数 uri：为要查询的数据库+表的名称。
             * 第二个参数 projection ： 要查询的列。
             * 第三个参数 selection ： 查询的条件，相当于SQL where。
             * 第三个参数 selectionArgs ： 查询条件的参数，相当于 ？。
             * 第四个参数 sortOrder ： 结果排序。
             */
            Cursor cursor = cr.query(uri, new String[]{MediaStore.Video.Media.DATA}, null, null, null);
            if(cursor != null && cursor.moveToFirst()){
                String videoPath = cursor.getString(cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA));
                mVideoView.setPlayURL(videoPath);
                //mVideoView.start();
            }
            if(cursor!=null)cursor.close();
        }
    }

    private void preper(){
        new Thread(new Runnable() {
            @Override
            public void run() {
                eventloop();
            }
        }).start();
        openStream();
    }

    private void preperSync(){
        new Thread(new Runnable() {
            @Override
            public void run() {
                eventloop();
            }
        }).start();
        new Thread(new Runnable() {
            @Override
            public void run() {
                openStream();
            }
        }).start();
    }

    private void eventloop(){
        for (;;){

        }
    }

    private void openStream(){

    }

    private Handler mHandler = new Handler(Looper.getMainLooper()){
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what){
                case 1:
                    mVideoView.start();
                    break;
            }
        }
    };
}
