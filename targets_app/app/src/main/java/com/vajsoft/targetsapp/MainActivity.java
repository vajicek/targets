package com.vajsoft.targetsapp;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.hardware.camera2.CameraManager;
import android.net.ConnectivityManager;
import android.os.Build;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
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

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.CompletableFuture;
import java.util.logging.Logger;
import java.util.stream.Collectors;

public class MainActivity extends AppCompatActivity {
    static private final Logger LOG = Logger.getLogger(MainActivity.class.getName());

    TextView textView;
    SurfaceView cameraPreview;
    SurfaceView resultView;

    CameraCapture cameraCapture;
    DriveClient driveClient;
    ModelInference modelInference;

    Bitmap detectionInput;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textView = findViewById(R.id.textView);
        textView.setMovementMethod(new ScrollingMovementMethod());

        cameraPreview = findViewById(R.id.cameraPreview);
        resultView = findViewById(R.id.resultView);

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
        final var start = Instant.now();
        printLine("Running detection");
        modelInference.run(detectionInput)
                .thenCompose(this::drawResults)
                .thenAccept(aVoid -> printLine(String.format(
                        Locale.getDefault(),
                        "Finished in: %.3f seconds",
                        Duration.between(start, Instant.now()).toMillis() / 1000.f)));
    }

    private CompletableFuture<Void> drawResults(final ModelOutputs outputs) {
        final var result = new CompletableFuture<Void>();
        runOnUiThread(() -> {
            printLine("Drawing results");
            Canvas canvas = resultView.getHolder().lockCanvas();
            canvas.scale(resultView.getWidth() / 640.0f, resultView.getWidth() / 640.0f);
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
            resultView.getHolder().unlockCanvasAndPost(canvas);
            printLine(outputs.toString());
            result.complete(null);
        });
        return result;
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void onCaptureClick(final View view) {
        resultView.setVisibility(View.INVISIBLE);
        cameraPreview.setVisibility(View.VISIBLE);
        cameraCapture.startCameraStream(this, cameraPreview.getHolder().getSurface());
        printLine("Start streaming camera");
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void onSnapClick(final View view) {
        cameraCapture.captureImage(cameraCapture.getCameraImageSize().orElseThrow(() -> new RuntimeException("xyz")))
                .thenAccept(inputImage -> runOnUiThread(() -> {
                    cameraPreview.setVisibility(View.INVISIBLE);
                    resultView.setVisibility(View.VISIBLE);
                    printLine(String.format(Locale.getDefault(), "Captured input, %d x %d", inputImage.getWidth(), inputImage.getHeight()));
                    detectionInput = clipRectImage(640, inputImage);
                    printLine("Clipped and scaled input to 640 x 640");
                    showInput(detectionInput);
                }));
    }

    private void printLine(final String str) {
        textView.setText(String.format("%s\n%s", textView.getText(), str));
    }

    private Bitmap clipRectImage(final int resolution, final Bitmap inputImage) {
        final var rotateAndTranslate = new Matrix();
        rotateAndTranslate.preRotate(90);
        rotateAndTranslate.postTranslate(inputImage.getHeight(), 0);

        final var scaleInputToResultView = resolution / (float)inputImage.getHeight();
        rotateAndTranslate.postScale(scaleInputToResultView, scaleInputToResultView);

        return Bitmap.createBitmap(inputImage,
                (inputImage.getWidth() - inputImage.getHeight()), 0,
                inputImage.getHeight(), inputImage.getHeight(), rotateAndTranslate, true);
    }

    private void showInput(final Bitmap croppedRotatedBitmap) {
        Canvas canvas = resultView.getHolder().lockCanvas();

        final var m = new Matrix();
        final var scaleInputToResultView = resultView.getHeight() / (float)croppedRotatedBitmap.getHeight();
        m.postScale(scaleInputToResultView, scaleInputToResultView);
        canvas.drawBitmap(croppedRotatedBitmap, m, null);

        resultView.getHolder().unlockCanvasAndPost(canvas);
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
