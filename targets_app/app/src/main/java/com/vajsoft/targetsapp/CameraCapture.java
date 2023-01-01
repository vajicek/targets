package com.vajsoft.targetsapp;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.OutputConfiguration;
import android.hardware.camera2.params.SessionConfiguration;
import android.media.Image;
import android.media.ImageReader;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Size;
import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;

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

    public CameraCapture(final CameraManager cameraManager) {
        this.cameraManager = cameraManager;
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void startCameraStream(final Context context, final Surface surface) {
        final var outputConfigurations = List.of(new OutputConfiguration(surface));
        final var sessionConfiguration = getSessionConfiguration(
                outputConfigurations,
                surface,
                this::captureContinuous);
        openCamera(context, getCameraStateCallback(sessionConfiguration));
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void stopCameraStream(final Context context, final Surface surface) {
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public CompletableFuture<Image> captureImage(final Context context, final Surface surface, final Size targetSize) {
        final var result = new CompletableFuture<Image>();
        final var imageReader = ImageReader.newInstance(
                targetSize.getWidth(),
                targetSize.getHeight(),
                ImageFormat.JPEG,
                1);

        final var outputConfigurations = surface == null ?
                List.of(new OutputConfiguration(imageReader.getSurface())) :
                List.of(new OutputConfiguration(surface), new OutputConfiguration(imageReader.getSurface()));
        final CheckedFunction2<CameraCaptureSession, CaptureRequest.Builder, Integer> submitCaptureRequest = (
                final CameraCaptureSession cameraCaptureSession,
                final CaptureRequest.Builder captureRequestBuilder) ->
                this.captureSingle(cameraCaptureSession, captureRequestBuilder, imageReader, result);

        final var sessionConfiguration =
                getSessionConfiguration(outputConfigurations, surface, submitCaptureRequest);

        openCamera(context, getCameraStateCallback(sessionConfiguration));

        return result;
    }

    public Optional<Size> getCameraImageSize() {
        try {
            final CameraCharacteristics cameraCharacteristics = cameraManager.getCameraCharacteristics(getCameraId());
            final var map = cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            final var largestImageSize = Collections.max(
                    Arrays.asList(map.getOutputSizes(ImageFormat.JPEG)),
                    new CompareSizeByArea());
            return Optional.of(largestImageSize);
        } catch (CameraAccessException exception) {
            return Optional.empty();
        }
    }

    private static class CompareSizeByArea implements Comparator<Size> {
        @Override
        public int compare(Size lhs, Size rhs) {
            return Long.signum((long) lhs.getWidth() * lhs.getHeight() -
                    (long) rhs.getWidth() * rhs.getHeight());
        }
    }

    private Rect getSquaredCaptureRegion() {
        final Size size = getCameraImageSize()
                .orElseThrow(() -> new RuntimeException("Failed to capture image"));
        final var w = size.getWidth();
        final var h = size.getHeight();
        final var s = (Math.max(w, h) - Math.min(w, h)) / 2;
        if (w > h) {
            return new Rect(s, 0, w - s, h);
        } else {
            return new Rect(0, s, w, h - s);
        }
    }

    private String getCameraId() throws CameraAccessException {
        final var cameraIdList = Arrays.asList(cameraManager.getCameraIdList());
        return cameraIdList.get(0);
    }

    private void openCamera(final Context context, final CameraDevice.StateCallback stateCallback) {
        try {
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                LOG.warning("Camera permission not granted");
                return;
            }
            cameraManager.openCamera(getCameraId(), stateCallback, null);
        } catch (CameraAccessException exception) {
            LOG.log(Level.SEVERE, "Error while streaming from camera", exception);
        }
    }

    private CameraDevice.StateCallback getCameraStateCallback(final SessionConfiguration sessionConfiguration) {
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
                LOG.log(Level.SEVERE, "onDisconnected");
            }

            @Override
            public void onError(@NonNull CameraDevice cameraDevice, int i) {
                LOG.log(Level.SEVERE, "onError");
            }
        };
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    private SessionConfiguration getSessionConfiguration(
            final List<OutputConfiguration> outputConfigurationList,
            final Surface surface,
            final CheckedFunction2<CameraCaptureSession, CaptureRequest.Builder, Integer> submitCaptureRequest) {
        return new SessionConfiguration(
                SessionConfiguration.SESSION_REGULAR,
                outputConfigurationList,
                Executors.newCachedThreadPool(),
                getCameraCaptureSessionStateCallback(surface, submitCaptureRequest));
    }

    private CameraCaptureSession.StateCallback getCameraCaptureSessionStateCallback(
            final Surface surface,
            final CheckedFunction2<CameraCaptureSession, CaptureRequest.Builder, Integer> submitCaptureRequest) {
        return new CameraCaptureSession.StateCallback() {

            @Override
            public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
                LOG.info("onConfigured");
                captureRequest(cameraCaptureSession, surface, submitCaptureRequest);
            }

            @Override
            public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                LOG.info("onConfigureFailed");
            }
        };
    }

    private void captureRequest(final CameraCaptureSession cameraCaptureSession,
                                final Surface surface,
                                final CheckedFunction2<CameraCaptureSession, CaptureRequest.Builder, Integer> submitCaptureRequest) {
        try {
            final CaptureRequest.Builder captureRequestBuilder = cameraCaptureSession
                    .getDevice()
                    .createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            if (surface != null) {
                captureRequestBuilder.addTarget(surface);
                //captureRequestBuilder.set(CaptureRequest.SCALER_CROP_REGION, getSquaredCaptureRegion());
            }
            submitCaptureRequest.apply(cameraCaptureSession, captureRequestBuilder);
        } catch (Throwable throwable) {
            LOG.log(Level.SEVERE, "Error while capturing request", throwable);
        }
    }

    private Integer captureSingle(final CameraCaptureSession cameraCaptureSession,
                                  final CaptureRequest.Builder captureRequestBuilder,
                                  final ImageReader imageReader,
                                  final CompletableFuture<Image> result) throws CameraAccessException {
        captureRequestBuilder.addTarget(imageReader.getSurface());

        imageReader.setOnImageAvailableListener(
                reader -> result.complete(imageReader.acquireLatestImage()),
                new Handler(Looper.getMainLooper()));

        return cameraCaptureSession.captureSingleRequest(
                captureRequestBuilder.build(),
                Executors.newCachedThreadPool(),
                new CameraCaptureSession.CaptureCallback() {
                    public void onCaptureCompleted(
                            final CameraCaptureSession session,
                            final CaptureRequest request,
                            final TotalCaptureResult captureResult) {
                        closeCamera(imageReader, cameraCaptureSession);
                        LOG.info("onCaptureCompleted");
                    }
                });
    }

    private void closeCamera(final ImageReader imageReader,
                             final CameraCaptureSession cameraCaptureSession) {
        LOG.info("closeCamera");
        if (null != cameraCaptureSession) {
            final CameraDevice cameraDevice = cameraCaptureSession.getDevice();
            cameraCaptureSession.close();

            if (null != cameraDevice) {
                cameraDevice.close();
            }
        }
        if (null != imageReader) {
            imageReader.close();
        }
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
