package com.vajsoft.targetsapp;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.OutputConfiguration;
import android.hardware.camera2.params.SessionConfiguration;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.SurfaceView;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Executors;
import java.util.logging.Level;
import java.util.logging.Logger;

public class CameraCapture {

    static private final Logger LOG = Logger.getLogger(CameraCapture.class.getName());

    final Activity activity;
    final SurfaceView surfaceView;

    public CameraCapture(final MainActivity activity, final SurfaceView surfaceView) {
        this.activity = activity;
        this.surfaceView = surfaceView;
    }

    public void startCameraStream(final Context context) {
        final var cameraManager = (CameraManager) activity.getSystemService(Context.CAMERA_SERVICE);
        try {
            final var cameraIdList = Arrays.asList(cameraManager.getCameraIdList());
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                LOG.warning("Camera permission not granted");
                return;
            }
            cameraManager.openCamera(cameraIdList.get(0), getCameraStateCallback(), null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private CameraDevice.StateCallback getCameraStateCallback() {
        return new CameraDevice.StateCallback() {
            @RequiresApi(api = Build.VERSION_CODES.R)
            @Override
            public void onOpened(@NonNull CameraDevice cameraDevice) {
                try {
                    cameraDevice.createCaptureSession(getSessionConfiguration());
                } catch (CameraAccessException e) {
                    LOG.log(Level.SEVERE, "Camera capture error", e);
                }
            }

            @Override
            public void onDisconnected(@NonNull CameraDevice cameraDevice) {
            }

            @Override
            public void onError(@NonNull CameraDevice cameraDevice, int i) {
            }
        };
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    private SessionConfiguration getSessionConfiguration() {
        return new SessionConfiguration(
                SessionConfiguration.SESSION_REGULAR,
                List.of(new OutputConfiguration(surfaceView.getHolder().getSurface())),
                Executors.newCachedThreadPool(),
                new CameraCaptureSession.StateCallback() {

                    @Override
                    public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
                        LOG.info("onConfigured");
                        captureRequest(cameraCaptureSession);
                    }

                    @Override
                    public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                        LOG.info("onConfigureFailed");
                    }
                });
    }

    private void captureRequest(final CameraCaptureSession cameraCaptureSession) {
        try {
            final CaptureRequest.Builder builder = cameraCaptureSession
                    .getDevice()
                    .createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            builder.addTarget(surfaceView.getHolder().getSurface());
            captureSingle(cameraCaptureSession, builder);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void captureSingle(final CameraCaptureSession cameraCaptureSession,
                               final CaptureRequest.Builder builder) throws CameraAccessException {
        cameraCaptureSession.captureSingleRequest(
                builder.build(),
                Executors.newCachedThreadPool(),
                new CameraCaptureSession.CaptureCallback() {
                    public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request, TotalCaptureResult result) {
                        LOG.info("onCaptureCompleted");
                        captureRequest(cameraCaptureSession);
                    }
                });
    }

    private void captureContinuous(final CameraCaptureSession cameraCaptureSession,
                                   final CaptureRequest.Builder builder) throws CameraAccessException {
        cameraCaptureSession.setRepeatingRequest(
                builder.build(),
                new CameraCaptureSession.CaptureCallback() {
                    public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request, TotalCaptureResult result) {
                        LOG.info("onCaptureCompleted");
                    }
                },
                new Handler(Looper.getMainLooper()) {
                    @Override
                    public void handleMessage(Message message) {
                        // This is where you do your work in the UI thread.
                        // Your worker tells you in the message what to do.
                    }
                }
        );
    }
}
