package com.my.newproject;

import android.view.View;
import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.SurfaceView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class MainActivity extends AppCompatActivity {

    private truesingularityclass cameraEngine;
    private static final int CAMERA_PERMISSION_CODE = 100;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            initCameraEngine();
        } else {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO}, CAMERA_PERMISSION_CODE);
        }
    }

    private void initCameraEngine() {
        SurfaceView surfaceView = findViewById(R.id.videoview3);
        TextView lblTimer = findViewById(R.id.textview3);
        TextView lblPhoto = findViewById(R.id.textview1);
        TextView lblVideo = findViewById(R.id.textview2);
        ImageView btnGallery = findViewById(R.id.imageview1);
        ImageView btnShutter = findViewById(R.id.circleimageview1);
        ImageView btnFlip = findViewById(R.id.imageview2);
                if (btnFlip != null) {
            btnFlip.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    btnFlip.animate().rotationBy(360f).setDuration(300).withEndAction(new Runnable() {
                        @Override
                        public void run() {
                            btnFlip.setRotation(0f);
                        }
                    }).start();

                    if (cameraEngine != null) {
                        cameraEngine.flipCameraAction();
                    }
                }
            });
        }

        
        ImageView btnFlash = findViewById(R.id.imageview3);
        LinearLayout zoomLayout = findViewById(R.id.linear4);

        cameraEngine = new truesingularityclass(
            this, surfaceView, lblTimer, lblPhoto, lblVideo, 
            btnGallery, btnShutter, btnFlip, btnFlash, zoomLayout
        );
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == CAMERA_PERMISSION_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                initCameraEngine();
            } else {
                Toast.makeText(this, "कैमरा चलाने के लिए परमिशन ज़रूरी है!", Toast.LENGTH_SHORT).show();
                finish();
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (cameraEngine != null) {
            cameraEngine.destroyEngine();
        }
    }
}
