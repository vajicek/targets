package com.vajsoft.targetsapp;

import android.content.Context;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import androidx.appcompat.app.AppCompatActivity;

import com.google.api.client.auth.oauth2.Credential;
import com.google.api.client.googleapis.auth.oauth2.GoogleCredential;
import com.google.api.services.drive.DriveScopes;

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
        cameraCapture = new CameraCapture(this, surfaceView);
        driveClient = new DriveClient(getCredentials(),
                (ConnectivityManager) this.getSystemService(Context.CONNECTIVITY_SERVICE));

        findViewById(R.id.capture).setOnClickListener(this::onButtonClick);
        findViewById(R.id.download).setOnClickListener(this::onDownloadButtonClick);
        findViewById(R.id.upload).setOnClickListener(this::onUploadButtonClick);
    }

    public void onButtonClick(final View view) {
        cameraCapture.startCameraStream(this);
    }

    public void onUploadButtonClick(final View view) {
        //cameraCapture.capture();
        //driveClient.upload();
    }

    public void onDownloadButtonClick(final View view) {
        driveClient.download();
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
