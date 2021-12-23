package com.vajsoft.targetsapp;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.OutputConfiguration;
import android.hardware.camera2.params.SessionConfiguration;
import android.media.ImageReader;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Size;
import android.view.SurfaceView;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executors;
import java.util.logging.Level;
import java.util.logging.Logger;

import io.vavr.CheckedFunction2;

public class CameraCapture {

    static private final Logger LOG = Logger.getLogger(CameraCapture.class.getName());

    final CameraManager cameraManager;
    final SurfaceView surfaceView;

    public CameraCapture(final CameraManager cameraManager, final SurfaceView surfaceView) {
        this.cameraManager = cameraManager;
        this.surfaceView = surfaceView;
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void startCameraStream(final Context context) {
        openCamera(context, getCameraStateCallback(getSessionConfiguration(
                List.of(new OutputConfiguration(surfaceView.getHolder().getSurface())),
                this::captureContinuous)));
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public CompletableFuture<ByteBuffer> captureImage(final Context context) {
        return getCameraImageSize()
                .map(size -> {
                    final var result = new CompletableFuture<ByteBuffer>();
                    final var imageReader = ImageReader.newInstance(size.getWidth(),
                            size.getHeight(),
                            ImageFormat.JPEG,
                            1);

                    openCamera(context, getCameraStateCallback(getSessionConfiguration(
                            List.of(new OutputConfiguration(surfaceView.getHolder().getSurface()),
                                    new OutputConfiguration(imageReader.getSurface())),
                            (cameraCaptureSession, captureRequestBuilder) -> {
                                captureRequestBuilder.addTarget(imageReader.getSurface());
                                return this.captureSingle(cameraCaptureSession, captureRequestBuilder,
                                        new CameraCaptureSession.CaptureCallback() {
                                            public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request, TotalCaptureResult captureResult) {
                                                ByteBuffer byteBuffer = imageReader.acquireLatestImage().getPlanes()[0].getBuffer();
                                                result.complete(byteBuffer);
                                                LOG.info("onCaptureCompleted");
                                            }
                                        });
                            })));
                    return result;
                })
                .orElseGet(() -> {
                    final var dummyResult = new CompletableFuture<ByteBuffer>();
                    dummyResult.completeExceptionally(new RuntimeException("Failed to capture image"));
                    return dummyResult;
                });
    }

    private Optional<Size> getCameraImageSize() {
        try {
            final var cameraIdList = Arrays.asList(cameraManager.getCameraIdList());
            final var cameraCharacteristics = cameraManager.getCameraCharacteristics(cameraIdList.get(0));
            final var map = cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            final var largestImageSize = Collections.max(
                    Arrays.asList(map.getOutputSizes(ImageFormat.JPEG)),
                    new CompareSizeByArea());
            return Optional.of(largestImageSize);
        } catch (CameraAccessException exception) {
            exception.printStackTrace();
        }
        return Optional.empty();
    }

    private static class CompareSizeByArea implements Comparator<Size> {
        @Override
        public int compare(Size lhs, Size rhs) {
            return Long.signum((long) lhs.getWidth() * lhs.getHeight() -
                    (long) rhs.getWidth() * rhs.getHeight());
        }
    }

    private void openCamera(final Context context, final CameraDevice.StateCallback stateCallback) {
        try {
            final var cameraIdList = Arrays.asList(cameraManager.getCameraIdList());
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                LOG.warning("Camera permission not granted");
                return;
            }
            cameraManager.openCamera(cameraIdList.get(0), stateCallback, null);
        } catch (CameraAccessException exception) {
            LOG.log(Level.SEVERE, "Error while streaming from camera", exception);
        }
    }

    private CameraDevice.StateCallback getCameraStateCallback(SessionConfiguration sessionConfiguration) {
        return new CameraDevice.StateCallback() {
            @RequiresApi(api = Build.VERSION_CODES.R)
            @Override
            public void onOpened(@NonNull CameraDevice cameraDevice) {
                try {
                    cameraDevice.createCaptureSession(sessionConfiguration);
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
    private SessionConfiguration getSessionConfiguration(
            final List<OutputConfiguration> outputConfigurationList,
            final CheckedFunction2<CameraCaptureSession, CaptureRequest.Builder, Integer> submitCaptureRequest) {
        return new SessionConfiguration(
                SessionConfiguration.SESSION_REGULAR,
                outputConfigurationList,
                Executors.newCachedThreadPool(),
                new CameraCaptureSession.StateCallback() {

                    @Override
                    public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
                        LOG.info("onConfigured");
                        captureRequest(cameraCaptureSession, submitCaptureRequest);
                    }

                    @Override
                    public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                        LOG.info("onConfigureFailed");
                    }
                });
    }

    private void captureRequest(final CameraCaptureSession cameraCaptureSession,
                                final CheckedFunction2<CameraCaptureSession, CaptureRequest.Builder, Integer> submitCaptureRequest) {
        try {
            final CaptureRequest.Builder captureRequestBuilder = cameraCaptureSession
                    .getDevice()
                    .createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            captureRequestBuilder.addTarget(surfaceView.getHolder().getSurface());
            submitCaptureRequest.apply(cameraCaptureSession, captureRequestBuilder);
        } catch (Throwable throwable) {
            LOG.log(Level.SEVERE, "Error while capturing request", throwable);
        }
    }

    private Integer captureSingle(final CameraCaptureSession cameraCaptureSession,
                                  final CaptureRequest.Builder captureRequestBuilder,
                                  final CameraCaptureSession.CaptureCallback callback) throws CameraAccessException {
        return cameraCaptureSession.captureSingleRequest(
                captureRequestBuilder.build(),
                Executors.newCachedThreadPool(),
                callback);
    }

    private Integer captureContinuous(final CameraCaptureSession cameraCaptureSession,
                                      final CaptureRequest.Builder captureRequestBuilder) throws CameraAccessException {
        return cameraCaptureSession.setRepeatingRequest(
                captureRequestBuilder.build(),
                new CameraCaptureSession.CaptureCallback() {
                },
                new Handler(Looper.getMainLooper())
        );
    }
}
