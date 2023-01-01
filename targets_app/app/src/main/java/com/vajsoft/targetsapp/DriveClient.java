package com.vajsoft.targetsapp;

import android.media.Image;
import android.net.ConnectivityManager;
import android.net.NetworkCapabilities;

import com.google.api.client.auth.oauth2.Credential;
import com.google.api.client.googleapis.batch.json.JsonBatchCallback;
import com.google.api.client.googleapis.json.GoogleJsonError;
import com.google.api.client.http.HttpHeaders;
import com.google.api.client.http.InputStreamContent;
import com.google.api.client.http.javanet.NetHttpTransport;
import com.google.api.client.json.jackson2.JacksonFactory;
import com.google.api.services.drive.Drive;
import com.google.api.services.drive.model.File;
import com.google.api.services.drive.model.FileList;
import com.google.api.services.drive.model.Permission;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Collectors;

import io.vavr.CheckedRunnable;
import io.vavr.control.Try;

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

    public CompletableFuture<List<File>> getFileList() {
        final var result = new CompletableFuture<List<File>>();
        getDataFromApiSafe(() -> result.complete(this.getFileListInternal()));
        return result;
    }

    public CompletableFuture<File> uploadJpeg(final String filename, final Image image) {
        final var result = new CompletableFuture<File>();
        getDataFromApiSafe(() -> result.complete(uploadJpegInternal(filename, image)));
        return result;
    }

    public CompletableFuture<Void> deleteFile(final List<String> fileIds) {
        final var result = new CompletableFuture<Void>();
        getDataFromApiSafe(() -> {
            deleteFileInternal(fileIds);
            result.complete(null);
        });
        return result;
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

    private File uploadJpegInternal(final String filename, final Image image) throws IOException {
        final var fileMetadata = new File();
        fileMetadata.setName(filename);

        final var imageByteBuffer = image.getPlanes()[0].getBuffer();

        byte[] bytes = new byte[imageByteBuffer.remaining()];
        imageByteBuffer.get(bytes);

        final var mediaContent = new InputStreamContent("image/jpeg", new ByteArrayInputStream(bytes));
        return service.files()
                .create(fileMetadata, mediaContent)
                .setFields("id")
                .execute();
    }

    private void setUserWritePermissions(final String fileId, final String email) throws IOException {
        Permission userPermission = new Permission()
                .setEmailAddress(email)
                .setType("user")
                .setRole("writer");
        service.permissions()
                .create(fileId, userPermission)
                .setFields("id")
                .execute();
    }

    private List<File> getFileListInternal() throws IOException {
        final var result = new ArrayList<File>();
        final var request = service.files().list();
        do {
            FileList files = request
                    .execute();
            result.addAll(files.getFiles()
                    .stream()
                    .peek(file -> LOG.info(String.format("%s %s", file.getName(), file.getId())))
                    .collect(Collectors.toList()));
            request.setPageToken(files.getNextPageToken());
        } while (request.getPageToken() != null &&
                request.getPageToken().length() > 0);
        return result;
    }

    private void deleteFileInternal(final List<String> fileIds) throws IOException {
        final var callback = new JsonBatchCallback<Void>() {
            @Override
            public void onSuccess(Void aVoid, HttpHeaders responseHeaders) throws IOException {
            }

            @Override
            public void onFailure(GoogleJsonError e, HttpHeaders responseHeaders) throws IOException {
            }
        };

        final var batch = service.batch();
        fileIds.forEach(fileId -> Try.of(() -> {
            service.files()
                    .delete(fileId)
                    .queue(batch, callback);
            return null;
        }));
        batch.execute();
    }

    private Drive getDriveService() {
        final var transport = new NetHttpTransport();
        final var jsonFactory = JacksonFactory.getDefaultInstance();
        return new Drive.Builder(transport, jsonFactory, credentials)
                .setApplicationName("TargetsApp")
                .build();
    }
}