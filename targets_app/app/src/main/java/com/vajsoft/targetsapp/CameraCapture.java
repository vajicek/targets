package com.vajsoft.targetsapp;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
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

public class CameraCapture {

    static private final Logger LOG = Logger.getLogger(CameraCapture.class.getName());

    final CameraManager cameraManager;

    CameraDevice cameraDevice;

    public CameraCapture(final CameraManager cameraManager) {
        this.cameraManager = cameraManager;
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void stopCameraStream() {
        closeCamera(null, null);
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void startCameraStream(final Context context, final Surface surface) {
        final var outputConfigurations = List.of(new OutputConfiguration(surface));
        final var previewSessionConfiguration = new SessionConfiguration(
                SessionConfiguration.SESSION_REGULAR,
                outputConfigurations,
                Executors.newCachedThreadPool(),
                getPreviewCameraCaptureSessionStateCallback(surface));

        try {
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                LOG.warning("Camera permission not granted");
                return;
            }
            cameraManager.openCamera(getCameraId(), getCameraStateCallback(previewSessionConfiguration), null);
        } catch (CameraAccessException exception) {
            LOG.log(Level.SEVERE, "Error while streaming from camera", exception);
        }
    }

    private CameraCaptureSession.StateCallback getPreviewCameraCaptureSessionStateCallback(final Surface surface) {
        return new CameraCaptureSession.StateCallback() {

            @Override
            public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
                LOG.info("onConfigured");
                previewCaptureRequest(cameraCaptureSession, surface);
            }

            @Override
            public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                LOG.info("onConfigureFailed");
            }
        };
    }

    private void previewCaptureRequest(final CameraCaptureSession cameraCaptureSession,
                                       final Surface surface) {
        try {
            final CaptureRequest.Builder captureRequestBuilder = cameraCaptureSession
                    .getDevice()
                    .createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            captureRequestBuilder.addTarget(surface);
            captureRequestBuilder.set(CaptureRequest.SCALER_CROP_REGION, getSquaredCaptureRegion());
            cameraCaptureSession.setRepeatingRequest(
                    captureRequestBuilder.build(),
                    new CameraCaptureSession.CaptureCallback() {
                    },
                    new Handler(Looper.getMainLooper())
            );
        } catch (CameraAccessException exception) {
            LOG.log(Level.SEVERE, "Error while capturing request", exception);
        }
    }

    private CameraDevice.StateCallback getCameraStateCallback(final SessionConfiguration sessionConfiguration) {
        return new CameraDevice.StateCallback() {

            @Override
            public void onClosed(@NonNull CameraDevice camera) {
                LOG.info("onClosed");
            }

            @RequiresApi(api = Build.VERSION_CODES.R)
            @Override
            public void onOpened(@NonNull CameraDevice newCameraDevice) {
                try {
                    cameraDevice = newCameraDevice;
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
    public CompletableFuture<Bitmap> captureImage(final Size targetSize) {
        final var captureImageResult = new CompletableFuture<Bitmap>();
        final var imageReader = ImageReader.newInstance(
                targetSize.getWidth(),
                targetSize.getHeight(),
                ImageFormat.JPEG,
                1);

        final var outputConfigurations = List.of(new OutputConfiguration(imageReader.getSurface()));
        final var captureSessionConfiguration = new SessionConfiguration(
                SessionConfiguration.SESSION_REGULAR,
                outputConfigurations,
                Executors.newCachedThreadPool(),
                getCaptureCameraCaptureSessionStateCallback(imageReader, captureImageResult));

        try {
            cameraDevice.createCaptureSession(captureSessionConfiguration);
        } catch (CameraAccessException e) {
            LOG.log(Level.SEVERE, "Camera capture error", e);
        }

        return captureImageResult;
    }

    private CameraCaptureSession.StateCallback getCaptureCameraCaptureSessionStateCallback(
            final ImageReader imageReader,
            final CompletableFuture<Bitmap> result) {

        return new CameraCaptureSession.StateCallback() {

            @Override
            public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
                LOG.info("onConfigured");
                singleCaptureRequest(cameraCaptureSession, imageReader, result);
            }

            @Override
            public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                LOG.info("onConfigureFailed");
            }
        };
    }

    private void singleCaptureRequest(final CameraCaptureSession cameraCaptureSession,
                                      final ImageReader imageReader,
                                      final CompletableFuture<Bitmap> captureImageResult) {
        try {
            final CaptureRequest.Builder captureRequestBuilder = cameraCaptureSession
                    .getDevice()
                    .createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            captureRequestBuilder.addTarget(imageReader.getSurface());

            imageReader.setOnImageAvailableListener(
                    reader -> {
                        LOG.info("onImageAvailableListener");
                        captureImageResult.complete(imageToBitmap(imageReader.acquireLatestImage()));
                        closeCamera(imageReader, cameraCaptureSession);
                    },
                    new Handler(Looper.getMainLooper()));

            cameraCaptureSession.captureSingleRequest(
                    captureRequestBuilder.build(),
                    Executors.newCachedThreadPool(),
                    new CameraCaptureSession.CaptureCallback() {
                        public void onCaptureCompleted(
                                final CameraCaptureSession session,
                                final CaptureRequest request,
                                final TotalCaptureResult captureResult) {
                            LOG.info("onCaptureCompleted");
                        }
                    });
        } catch (CameraAccessException exception) {
            LOG.log(Level.SEVERE, "Error while capturing request", exception);
        }
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

    private Bitmap imageToBitmap(final Image image) {
        LOG.info(String.format("img >>>>>>> %d, %d", image.getWidth(), image.getHeight()));

        ByteBuffer buffer = image.getPlanes()[0].getBuffer();
        byte[] bytes = new byte[buffer.capacity()];
        buffer.get(bytes);
        final var bmp = BitmapFactory.decodeByteArray(bytes, 0, bytes.length, null);

        LOG.info(String.format("bmp >>>>>>> %d, %d", bmp.getWidth(), bmp.getHeight()));
        return bmp;
    }
}
