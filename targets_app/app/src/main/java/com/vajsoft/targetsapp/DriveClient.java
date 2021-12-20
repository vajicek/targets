package com.vajsoft.targetsapp;

import android.net.ConnectivityManager;
import android.net.NetworkCapabilities;

import com.google.api.client.auth.oauth2.Credential;
import com.google.api.client.http.javanet.NetHttpTransport;
import com.google.api.client.json.jackson2.JacksonFactory;
import com.google.api.services.drive.Drive;
import com.google.api.services.drive.model.File;
import com.google.api.services.drive.model.FileList;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Collectors;

import io.vavr.CheckedRunnable;

public class DriveClient {

    static private final Logger LOG = Logger.getLogger(DriveClient.class.getName());

    private final Credential credentials;
    private final ConnectivityManager connectivityManager;
    private final Executor executor = Executors.newSingleThreadExecutor();
    private final Drive service;

    public DriveClient(final Credential credentials, final ConnectivityManager connectivityManager) {
        this.credentials = credentials;
        this.connectivityManager = connectivityManager;
        this.service = getDriveService();
    }

    public void download() {
        getDataFromApiSafe(this::getFileList);
    }

    public void upload() {
        getDataFromApiSafe(this::uploadImage);
    }

    private boolean isDeviceOnline() {
        final var activeNetwork = connectivityManager.getActiveNetwork();
        if (activeNetwork == null) {
            return false;
        }
        final var activeNetworkCapabilities = connectivityManager.getNetworkCapabilities(activeNetwork);
        return activeNetworkCapabilities != null && (
                activeNetworkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) ||
                        activeNetworkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) ||
                        activeNetworkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET) ||
                        activeNetworkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_BLUETOOTH));
    }

    private void getDataFromApiSafe(CheckedRunnable runnable) {
        if (!isDeviceOnline()) {
            LOG.warning("No network connection available.");
        } else {
            executor.execute(() -> {
                try {
                    runnable.run();
                } catch (Throwable throwable) {
                    LOG.log(Level.SEVERE, "Error while running command", throwable);
                }
            });
        }
    }

    private Void uploadImage() throws IOException {
        return null;
    }

    private List<String> getFileList() throws IOException {
        final var result = new ArrayList<String>();
        final var request = service.files().list();
        do {
            FileList files = request
                    .execute();
            result.addAll(files.getFiles()
                    .stream()
                    .peek(f -> LOG.info(f.getName()))
                    .map(File::getName)
                    .collect(Collectors.toList()));
            request.setPageToken(files.getNextPageToken());
        } while (request.getPageToken() != null &&
                request.getPageToken().length() > 0);
        return result;
    }

    private Drive getDriveService() {
        final var transport = new NetHttpTransport();
        final var jsonFactory = JacksonFactory.getDefaultInstance();
        return new Drive.Builder(transport, jsonFactory, credentials)
                .setApplicationName("TargetsApp")
                .build();
    }
}