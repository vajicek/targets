package com.vajsoft.targetsapp;

import android.content.Context;
import android.hardware.camera2.CameraManager;
import android.net.ConnectivityManager;
import android.os.Build;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.EditText;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import com.google.api.client.auth.oauth2.Credential;
import com.google.api.client.googleapis.auth.oauth2.GoogleCredential;
import com.google.api.services.drive.DriveScopes;
import com.google.api.services.drive.model.File;

import java.util.stream.Collectors;

public class MainActivity extends AppCompatActivity {

    EditText textMultiLine;
    SurfaceView surfaceView;
    CameraCapture cameraCapture;
    DriveClient driveClient;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textMultiLine = findViewById(R.id.editTextTextMultiLine2);
        surfaceView = findViewById(R.id.surfaceView);
        cameraCapture = new CameraCapture(
                (CameraManager) this.getSystemService(Context.CAMERA_SERVICE),
                surfaceView);
        driveClient = new DriveClient(
                getCredentials(),
                (ConnectivityManager) this.getSystemService(Context.CONNECTIVITY_SERVICE));

        findViewById(R.id.capture).setOnClickListener(this::onButtonClick);
        findViewById(R.id.getfiles).setOnClickListener(this::onGetFilesClick);
        findViewById(R.id.upload).setOnClickListener(this::onUploadButtonClick);
        findViewById(R.id.deleteall).setOnClickListener(this::onDeleteAll);
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void onButtonClick(final View view) {
        cameraCapture.startCameraStream(this);
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    public void onUploadButtonClick(final View view) {
        cameraCapture.captureImage(this)
                .thenAccept(buffer -> driveClient.uploadJpeg("xyz.jpg", buffer));
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
                            textMultiLine.getText().clear();
                            fileList.forEach(file -> textMultiLine.getText()
                                    .append(file.getName())
                                    .append("\n"));
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
