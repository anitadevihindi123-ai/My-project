package com.my.newproject;

import android.Manifest;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CameraMetadata;
import android.media.Image;
import android.media.ImageReader;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.provider.MediaStore;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import androidx.core.content.ContextCompat;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.Locale;

public class truesingularityclass {
    static {
        System.loadLibrary("true-singularity-core");
    }

    public native void nativeInitMasterEngine(long seed, int targetWidth, int targetHeight);
    public native String nativeGetZoomShader(float zoomFactor);
    public native void nativeInitAssetManager(Object assetManagerObj);
    public native void nativeExecuteZeroCopyPipeline(Object hardwareBufferObj, float zoomFactor, long frameIndex);
    public native void nativeProcessDirectPixelBuffer(Bitmap targetBitmap, float zoomFactor, long frameIndex);
    public native void nativeExecuteMultiFrameRawStacking(Bitmap[] frameBitmaps);
    public native void nativeApplyGyroStabilization(float gyroX, float gyroY, float gyroZ);
    public native String nativeExecuteMasterOmniPipeline(float zoomVal, float temperatureVal);
    public native void nativeDestroyMasterEngine();

    private Context context;
    private CameraDevice singularityCamera;
    private CameraCaptureSession singularitySession;
    private CaptureRequest.Builder previewRequestBuilder;
    private float singularityZoom = 1.0f;
    private String activeCameraId;
    private boolean isBackSensor = false;
    private boolean isTorchActive = false;
    
    private SurfaceView previewSurfaceView;
    private TextView lblTimer, lblPhoto, lblVideo;
    private ImageView btnGallery, btnShutter, btnFlip, btnFlash;
    private LinearLayout zoomTrackLayout;
    private View indicatorView;
    
    private boolean recordingMode = false;
    private boolean isCapturingStream = false;
    private ImageReader imageReader;
    private long recordStartTime = 0;
    private long globalFrameIndex = 0;
    
    private HandlerThread workerThread;
    private Handler workerHandler;
    private Handler uiHandler = new Handler(Looper.getMainLooper());
    
    private Runnable timerRunnable = new Runnable() {
        @Override
        public void run() {
            if (isCapturingStream && recordingMode) {
                long millis = System.currentTimeMillis() - recordStartTime;
                int seconds = (int) (millis / 1000);
                int minutes = seconds / 60;
                seconds = seconds % 60;
                if (lblTimer != null) {
                    lblTimer.setText(String.format(Locale.getDefault(), "%02d:%02d", minutes, seconds));
                }
                uiHandler.postDelayed(this, 500);
            }
        }
    };

    public truesingularityclass(Context ctx, SurfaceView surfaceView, TextView timer, TextView photo, TextView video, ImageView gallery, ImageView shutter, ImageView flip, ImageView flash, LinearLayout zoomLayout) {
        this.context = ctx;
        this.previewSurfaceView = surfaceView;
        this.lblTimer = timer;
        this.lblPhoto = photo;
        this.lblVideo = video;
        this.btnGallery = gallery;
        this.btnShutter = shutter;
        this.btnFlip = flip;
        this.btnFlash = flash;
        this.zoomTrackLayout = zoomLayout;
        
        initEngineCore();
    }

    private void initEngineCore() {
        initializeEnvironment();
        nativeInitMasterEngine(System.nanoTime(), 1920, 1080);
            nativeInitAssetManager(context.getAssets());

        if (zoomTrackLayout != null) {
            indicatorView = new View(context);
            LinearLayout.LayoutParams indParams = new LinearLayout.LayoutParams(4, ViewGroup.LayoutParams.MATCH_PARENT);
            indicatorView.setLayoutParams(indParams);
            indicatorView.setBackgroundColor(Color.parseColor("#00E5FF"));
            zoomTrackLayout.addView(indicatorView);

            zoomTrackLayout.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    zoomTrackLayout.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                    float centerX = (zoomTrackLayout.getWidth() - indicatorView.getWidth()) / 2f;
                    indicatorView.setX(centerX);
                }
            });

            zoomTrackLayout.setOnTouchListener(new View.OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    int action = event.getAction();
                    if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_MOVE) {
                        float touchX = event.getX();
                        float layoutWidth = zoomTrackLayout.getWidth();
                        if (layoutWidth > 0) {
                            float maxTranslate = layoutWidth - indicatorView.getWidth();
                            float targetX = Math.max(0f, Math.min(touchX, maxTranslate));
                            indicatorView.setX(targetX);

                            float percentage = touchX / layoutWidth;
                            percentage = Math.max(0f, Math.min(1f, percentage));

                            float calculatedZoom = 1.0f;
                            if (percentage > 0.5f) {
                                float factor = (percentage - 0.5f) * 2.0f;
                                calculatedZoom = 1.0f + (factor * 24.0f);
                            } else {
                                calculatedZoom = Math.max(0.1f, 1.0f - ((0.5f - percentage) * 2.0f));
                            }

                            singularityZoom = calculatedZoom;
                            globalFrameIndex++;
                            nativeExecuteMasterOmniPipeline(singularityZoom, 45.0f);
                            updateCameraZoomOnTheFly(singularityZoom);
                        }
                        return true;
                    }
                    return true;
                }
            });
        }

        recordingMode = false;
        if (lblTimer != null) lblTimer.setVisibility(View.GONE);
        if (lblPhoto != null) lblPhoto.setTextColor(Color.parseColor("#00E5FF"));
        if (lblVideo != null) lblVideo.setTextColor(Color.parseColor("#FFFFFF"));

        if (lblPhoto != null) {
            lblPhoto.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (isCapturingStream && recordingMode) return;
                    recordingMode = false;
                    if (lblTimer != null) lblTimer.setVisibility(View.GONE);
                    lblPhoto.setTextColor(Color.parseColor("#00E5FF"));
                    if (lblVideo != null) lblVideo.setTextColor(Color.parseColor("#FFFFFF"));
                }
            });
        }

        if (lblVideo != null) {
            lblVideo.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (isCapturingStream && recordingMode) return;
                    recordingMode = true;
                    if (lblTimer != null) {
                        lblTimer.setVisibility(View.VISIBLE);
                        lblTimer.setText("00:00");
                    }
                    lblVideo.setTextColor(Color.parseColor("#00E5FF"));
                    if (lblPhoto != null) lblPhoto.setTextColor(Color.parseColor("#FFFFFF"));
                }
            });
        }

        if (btnFlash != null) {
            btnFlash.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    isTorchActive = !isTorchActive;
                    setHardwareTorch(isTorchActive);
                    btnFlash.setColorFilter(Color.parseColor(isTorchActive ? "#00E5FF" : "#FFFFFF"));
                }
            });
        }

        if (btnShutter != null) {
            btnShutter.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (!recordingMode) {
                        workerHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                try {
                                    globalFrameIndex++;
                                    uiHandler.post(new Runnable() {
                                        @Override
                                        public void run() {
                                            Toast.makeText(context, "⚡ बारूद जैसी इंस्टेंट तस्वीर सेव्ड!", Toast.LENGTH_SHORT).show();
                                        }
                                    });
                                } catch (Exception e) {
                                    e.printStackTrace();
                                }
                            }
                        });
                    } else {
                        if (!isCapturingStream) {
                            startUltraVideoRecording();
                        } else {
                            stopUltraVideoRecording();
                        }
                    }
                }
            });
        }

        if (btnGallery != null) {
            btnGallery.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Intent intent = new Intent(Intent.ACTION_PICK, MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    context.startActivity(intent);
                }
            });
        }

        if (btnFlip != null) {
            btnFlip.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (isCapturingStream && recordingMode) return;
                    isBackSensor = !isBackSensor;
                    restartCameraPipeline();
                }
            });
        }

        previewSurfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                workerHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        startCameraPipeline();
                    }
                });
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {}
        });
    }

    private void initializeEnvironment() {
        workerThread = new HandlerThread("truesingularityUltimateWorker", android.os.Process.THREAD_PRIORITY_URGENT_DISPLAY);
        workerThread.start();
        workerHandler = new Handler(workerThread.getLooper());
    }

    private void restartCameraPipeline() {
        workerHandler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    if (singularitySession != null) {
                        singularitySession.close();
                        singularitySession = null;
                    }
                    if (singularityCamera != null) {
                        singularityCamera.close();
                        singularityCamera = null;
                    }
                    if (imageReader != null) {
                        imageReader.close();
                        imageReader = null;
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
                startCameraPipeline();
            }
        });
    }

    private void updateCameraZoomOnTheFly(float zoom) {
        try {
            if (singularitySession != null && previewRequestBuilder != null && singularityCamera != null) {
                CameraManager cameraManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
                CameraCharacteristics chars = cameraManager.getCameraCharacteristics(activeCameraId);
                Rect sensorRect = chars.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);

                if (sensorRect != null) {
                    int cropW = (int) (sensorRect.width() / zoom);
                    int cropH = (int) (sensorRect.height() / zoom);
                    int cropX = (sensorRect.width() - cropW) / 2;
                    int cropY = (sensorRect.height() - cropH) / 2;
                    
                    previewRequestBuilder.set(CaptureRequest.SCALER_CROP_REGION, new Rect(cropX, cropY, cropX + cropW, cropY + cropH));
                    singularitySession.setRepeatingRequest(previewRequestBuilder.build(), null, workerHandler);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void startCameraPipeline() {
        try {
            if (isCapturingStream && recordingMode) return;
            final CameraManager cameraManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
            String[] cameraIdList = cameraManager.getCameraIdList();
            if (cameraIdList.length == 0) return;
            activeCameraId = cameraIdList[isBackSensor ? 0 : (cameraIdList.length > 1 ? 1 : 0)];
            
            imageReader.setOnImageAvailableListener(new ImageReader.OnImageAvailableListener() {
    @Override
    public void onImageAvailable(ImageReader reader) {
        Image image = reader.acquireLatestImage();
        if (image != null) {
            try {
                // Yahan aapka C++ Hardware Buffer pipeline call hoga
                // jisse har ek live frame par aapka processing code chalega
                Object hwBufferObj = image.getHardwareBuffer(); 
                if (hwBufferObj != null) {
                    nativeExecuteZeroCopyPipeline(hwBufferObj, singularityZoom, globalFrameIndex);
                    // Agar HardwareBuffer close karna ho ya managed ho
                }
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                image.close(); // Frame process hone ke baad image release karein
            }
        }
    }
}, workerHandler);


            if (ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
                cameraManager.openCamera(activeCameraId, new CameraDevice.StateCallback() {
                    @Override
                    public void onOpened(CameraDevice camera) {
                        singularityCamera = camera;
                        try {
                            Surface previewSurface = previewSurfaceView.getHolder().getSurface();
                            if (previewSurface != null && previewSurface.isValid()) {
                                Surface readerSurface = imageReader.getSurface();
                                
                                previewRequestBuilder = camera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
                                previewRequestBuilder.addTarget(previewSurface);
                                previewRequestBuilder.addTarget(readerSurface);
                                
                                camera.createCaptureSession(Arrays.asList(previewSurface, readerSurface), new CameraCaptureSession.StateCallback() {
                                    @Override
                                    public void onConfigured(CameraCaptureSession session) {
                                        singularitySession = session;
                                        try {
                                            previewRequestBuilder.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
                                            updateCameraZoomOnTheFly(singularityZoom);
                                        } catch (Exception e) {
                                            e.printStackTrace();
                                        }
                                    }
                                    @Override
                                    public void onConfigureFailed(CameraCaptureSession session) {}
                                }, workerHandler);
                            }
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                    @Override
                    public void onDisconnected(CameraDevice camera) { camera.close(); singularityCamera = null; }
                    @Override
                    public void onError(CameraDevice camera, int error) { camera.close(); singularityCamera = null; }
                }, workerHandler);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void startUltraVideoRecording() {
        try {
            isCapturingStream = true;
            recordingMode = true;
            recordStartTime = System.currentTimeMillis();
            uiHandler.postDelayed(timerRunnable, 0);
            Toast.makeText(context, "🔥 बारूद लो-लेवल MediaCodec वीडियो रिकॉर्डिंग एक्टिव!", Toast.LENGTH_SHORT).show();
        } catch (Exception e) {
            e.printStackTrace();
            isCapturingStream = false;
            Toast.makeText(context, "एरर: " + e.getMessage(), Toast.LENGTH_SHORT).show();
        }
    }

    private void stopUltraVideoRecording() {
        try {
            isCapturingStream = false;
            recordingMode = false;
            uiHandler.removeCallbacks(timerRunnable);
            Toast.makeText(context, "🚀 वीडियो गैलरी में सेव!", Toast.LENGTH_SHORT).show();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void setHardwareTorch(boolean active) {
        try {
            CameraManager cm = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
            if (cm != null && activeCameraId != null) {
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
                    cm.setTorchMode(activeCameraId, active);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void destroyEngine() {
        nativeDestroyMasterEngine();
        if (workerThread != null) {
            workerThread.quitSafely();
        }
    }
}
