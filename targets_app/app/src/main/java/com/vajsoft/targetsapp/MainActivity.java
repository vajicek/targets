package com.vajsoft.targetsapp;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.OutputConfiguration;
import android.hardware.camera2.params.SessionConfiguration;
import android.os.Build;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceView;
import android.view.View;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.snackbar.Snackbar;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

public class MainActivity extends AppCompatActivity {

    EditText textMultiLine;
    SurfaceView surfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        textMultiLine = findViewById(R.id.editTextTextMultiLine2);
        surfaceView = findViewById(R.id.surfaceView);

        FloatingActionButton fab = findViewById(R.id.fab);
        fab.setOnClickListener(view -> this.onButtonClick(view));
    }

    @RequiresApi(api = Build.VERSION_CODES.P)
    public void onButtonClick(final View view) {
        CameraManager manager = (CameraManager) this.getSystemService(Context.CAMERA_SERVICE);
        try {
            final var cameraIdList = Arrays.asList(manager.getCameraIdList());
//            addText(String.join(",", cameraIdList));
//            addText(String.join("\n\n---------------------\n", cameraIdList.stream()
//                    .map(id -> {
//                        try {
//                            return toString(manager.getCameraCharacteristics(id));
//                        } catch (CameraAccessException e) {
//                            e.printStackTrace();
//                        }
//                        return "";
//                    })
//                    .collect(Collectors.toList())));

            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                addText("permission not granted");
                // TODO: Consider calling
                //    ActivityCompat#requestPermissions
                // here to request the missing permissions, and then overriding
                //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                //                                          int[] grantResults)
                // to handle the case where the user grants the permission. See the documentation
                // for ActivityCompat#requestPermissions for more details.
                return;
            }

            addText("manager.openCamera");
            manager.openCamera(cameraIdList.get(0),
                    new CameraDevice.StateCallback() {
                        @RequiresApi(api = Build.VERSION_CODES.R)
                        @Override
                        public void onOpened(@NonNull CameraDevice cameraDevice) {
                            addText("ON OPENED");
                            try {
                                cameraDevice.createCaptureSession(new SessionConfiguration(
                                        SessionConfiguration.SESSION_REGULAR,
                                        List.of(new OutputConfiguration(surfaceView.getHolder().getSurface())),
                                        Executors.newCachedThreadPool(),
                                        new CameraCaptureSession.StateCallback() {

                                            @Override
                                            public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
                                                addText("onConfigured");

                                                try {
                                                    final CaptureRequest.Builder builder = cameraCaptureSession
                                                            .getDevice()
                                                            .createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
                                                    builder.addTarget(surfaceView.getHolder().getSurface());

                                                    cameraCaptureSession.captureSingleRequest(
                                                            builder.build(),
                                                            Executors.newCachedThreadPool(),
                                                            new CameraCaptureSession.CaptureCallback() {
                                                                public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request, TotalCaptureResult result) {
                                                                    addText("onCaptureCompleted");
                                                                }
                                                    });
                                                } catch (CameraAccessException e) {
                                                    e.printStackTrace();
                                                }
                                            }

                                            @Override
                                            public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                                                addText("onConfigureFailed");
                                            }
                                        })
                                );
                            } catch (CameraAccessException e) {
                                e.printStackTrace();
                            }
                        }

                        @Override
                        public void onDisconnected(@NonNull CameraDevice cameraDevice) {
                            addText("onDisconnected");
                        }

                        @Override
                        public void onError(@NonNull CameraDevice cameraDevice, int i) {
                            addText("onError");
                        }
                    }, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    static private String toString(CameraCharacteristics ch) {
        return String.join("\n",
                ch.getKeys()
                        .stream()
                        .map(key-> key.getName() + ":" + ch.get(key).toString())
                        .collect(Collectors.toList()));
    }

    public void addText(String line) {
        this.runOnUiThread(()->{
            textMultiLine.getText().append(line + "\n");
        });
    }

    public void showMessage(final View view, final String message) {
        Snackbar.make(view, message, Snackbar.LENGTH_LONG)
                .setAction("Action", null).show();
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
