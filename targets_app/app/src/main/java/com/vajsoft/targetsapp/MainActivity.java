package com.vajsoft.targetsapp;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.hardware.camera2.CameraManager;
import android.media.Image;
import android.net.ConnectivityManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Size;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.google.api.client.auth.oauth2.Credential;
import com.google.api.client.googleapis.auth.oauth2.GoogleCredential;
import com.google.api.services.drive.DriveScopes;
import com.google.api.services.drive.model.File;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.logging.Logger;
import java.util.stream.Collectors;

public class MainActivity extends AppCompatActivity {
    static private final Logger LOG = Logger.getLogger(MainActivity.class.getName());

    TextView textView;
    SurfaceView surfaceView;

    CameraCapture cameraCapture;
    DriveClient driveClient;
    ModelInference modelInference;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textView = findViewById(R.id.textView);
        surfaceView = findViewById(R.id.surfaceView);

        findViewById(R.id.detect).setOnClickListener(this::onDetectClick);
        findViewById(R.id.capture).setOnClickListener(this::onCaptureClick);
        findViewById(R.id.snap).setOnClickListener(this::onSnapClick);
        //findViewById(R.id.getfiles).setOnClickListener(this::onGetFilesClick);
        //findViewById(R.id.upload).setOnClickListener(this::onUploadButtonClick);
        //findViewById(R.id.deleteall).setOnClickListener(this::onDeleteAll);

        checkAndRequestPermissions(Manifest.permission.CAMERA);

        cameraCapture = new CameraCapture(
                (CameraManager) this.getSystemService(Context.CAMERA_SERVICE));
        driveClient = new DriveClient(
                getCredentials(),
                (ConnectivityManager) this.getSystemService(Context.CONNECTIVITY_SERVICE));
        modelInference = new ModelInference(getApplicationContext(),
                cameraCapture.getCameraImageSize().orElse(null));
    }

    void checkAndRequestPermissions(final String permissionString) {
        final var result = ContextCompat.checkSelfPermission(this, permissionString);
        if (result != PackageManager.PERMISSION_GRANTED) {
            LOG.info(String.format("%s permissions not granted, requesting from user interactively", permissionString));
            ActivityCompat.requestPermissions(
                    this,
                    new String[]{permissionString},
                    50);
        } else {
            LOG.info(String.format("%s permissions granted", permissionString));
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void onDetectClick(final View view) {
        try {
            final var inputStream = getAssets().open("testdata.jpg");
            final var bitmap = BitmapFactory.decodeStream(inputStream);
            modelInference.run(bitmap)
                    .thenAccept(this::drawResults);
        } catch (IOException ex) {
            LOG.info("Failed to load image");
        }
    }

    void drawResults(final ModelOutputs outputs) {
        runOnUiThread(() -> {
            Canvas canvas = surfaceView.getHolder().lockCanvas();
            canvas.drawBitmap(outputs.inputImage, new Matrix(), null);
            final var paint = new Paint();
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(Color.BLACK);
            for (int i = 0; i < outputs.outputLocations[0].length; i++) {
                if (outputs.outputScores[0][i] < 0.1) {
                    continue;
                }
                float[] floats = outputs.outputLocations[0][i];
                float radius = (float) Math.sqrt(
                        (floats[2] - floats[0]) * (floats[2] - floats[0]) +
                                (floats[3] - floats[1]) * (floats[3] - floats[1]));
                canvas.drawCircle(
                        640 * (floats[1] + floats[3]) / 2,
                        640 * (floats[0] + floats[2]) / 2,
                        0.5f * 640 * radius,
                        paint);
            }
            surfaceView.getHolder().unlockCanvasAndPost(canvas);
            textView.setText(outputs.toString());
        });
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void onCaptureClick(final View view) {
        cameraCapture.startCameraStream(this, surfaceView.getHolder().getSurface());
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void onSnapClick(final View view) {
        cameraCapture.captureImage(this,
                //surfaceView.getHolder().getSurface(),
                null,
                cameraCapture.getCameraImageSize().orElseThrow(() -> new RuntimeException("xyz"))
                //new Size(640, 640)
        )
                .thenAccept(image -> {
                    runOnUiThread(() -> {
                        final var inputImage = imageToBitmap(image);
                        Canvas canvas = surfaceView.getHolder().lockCanvas();

                        final var m = new Matrix();
                        m.preRotate(90);
                        m.postTranslate(960, 0);
                        //m.postTranslate(canvas.getWidth(), 0);
                        final var scale = 0.5f;//surfaceView.getWidth()/640.0f;
                        m.postScale(scale, scale);

                        canvas.drawBitmap(inputImage, m, null);
                        surfaceView.getHolder().unlockCanvasAndPost(canvas);
                    });
//                    modelInference.run(imageToBitmap(image))
//                            .thenAccept(this::drawResults);
                });
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

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void onUploadButtonClick(final View view) {
//        cameraCapture.captureImage(this, surfaceView.getHolder().getSurface())
//                .thenAccept(buffer -> driveClient.uploadJpeg("xyz.jpg", buffer));
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void onDeleteAll(final View view) {
        driveClient.getFileList().thenAccept(fileList->{
            driveClient.deleteFile(fileList.stream()
                    .map(File::getId)
                    .collect(Collectors.toList()));
        });
    }

    public void onGetFilesClick(final View view) {
        driveClient.getFileList()
                .thenAccept(fileList ->
                        this.runOnUiThread(() -> {
                            List<String> fileNames = fileList.stream()
                                    .map(File::getName).collect(Collectors.toList());
                            textView.setText(String.join("\n", fileNames));
                        })
                );
    }

    private Credential getCredentials() {
        try {
            return GoogleCredential
                    .fromStream(this.getResources().openRawResource(R.raw.targetsappa51b7605d0c8))
                    .createScoped(DriveScopes.all());
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
}
