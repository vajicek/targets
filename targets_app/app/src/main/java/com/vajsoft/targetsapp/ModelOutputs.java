package com.vajsoft.targetsapp;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;

import androidx.annotation.NonNull;

public class ModelOutputs {
    private static final int NUM_DETECTIONS = 10;

    public Bitmap inputImage;
    public float[][][] outputLocations = new float[1][NUM_DETECTIONS][4];
    public float[][] outputClasses = new float[1][NUM_DETECTIONS];
    public float[][] outputScores = new float[1][NUM_DETECTIONS];
    public float[] numDetections = new float[1];

    public ModelOutputs(Bitmap inputImage) {
        this.inputImage = inputImage;
    }

    @NonNull
    @SuppressLint("DefaultLocale")
    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder();
        sb.append(String.format("numDetections = %f", numDetections[0]));
        for (int i = 0; i < outputLocations[0].length; i++) {
            final var loc = outputLocations[0][i];
            final var score = outputScores[0][i];
            final var outputClass = outputClasses[0][i];
            sb.append(String.format("%.3f, %.3f: %.3f, %.3f, %.3f, %.3f\n",
                    score,
                    outputClass,
                    loc[0] * 640,
                    loc[1] * 640,
                    loc[2] * 640,
                    loc[3] * 640));
        }
        return sb.toString();
    }
}
