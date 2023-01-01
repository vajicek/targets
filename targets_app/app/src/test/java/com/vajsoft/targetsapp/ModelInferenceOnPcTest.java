package com.vajsoft.targetsapp;

import android.graphics.BitmapFactory;

import org.junit.Ignore;
import org.junit.Test;
import org.tensorflow.lite.Interpreter;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.util.HashMap;
import java.util.Map;

public class ModelInferenceOnPcTest {
    @Test
    @Ignore
    public void testModelInference() throws IOException {
        final var interpreter = new Interpreter(new File("src/main/assets/detect.tflite"));

        byte[] imageBytes = Files.readAllBytes(new File("src/main/assets/testdata.jpg").toPath());

        final var outputs = new Outputs();
        interpreter.runForMultipleInputsOutputs(prepareInputs(imageBytes), prepareOutputs(outputs));
        printOutputs(outputs);
    }

    private void printOutputs(Outputs outputs) {
        System.out.println("numDetections = " + outputs.numDetections[0]);
        for (int i = 0; i < outputs.outputLocations[0].length; i++) {
            final var loc = outputs.outputLocations[0][i];
            final var score = outputs.outputScores[0][i];
            final var outputClass = outputs.outputClasses[0][i];
            System.out.println(score + ", " + outputClass + " : " + loc[0] + ", " + loc[1] + ", " + loc[2] + ", " + loc[3]);
        }
    }
    private static final int NUM_DETECTIONS = 10;

    public static class Outputs {
        public float[][][] outputLocations = new float[1][NUM_DETECTIONS][4];
        public float[][] outputClasses = new float[1][NUM_DETECTIONS];
        public float[][] outputScores = new float[1][NUM_DETECTIONS];
        public float[]  numDetections = new float[1];
    }

    Map<Integer, Object> prepareOutputs(final Outputs outputs) {
        Map<Integer, Object> outputMap = new HashMap<>();
        outputMap.put(0, outputs.outputScores);
        outputMap.put(1, outputs.outputLocations);
        outputMap.put(2, outputs.numDetections);
        outputMap.put(3, outputs.outputClasses);
        return outputMap;
    }

    Object[] prepareInputs(final byte[] imageBytes) {
        int numBytesPerChannel = 4;
        int inputSize = 640;
        ByteBuffer imgData = ByteBuffer.allocateDirect(1 * inputSize * inputSize * 3 * numBytesPerChannel);

        final var bitmap = BitmapFactory.decodeByteArray(imageBytes, 0, imageBytes.length);
        final var intValues = new int[inputSize * inputSize];
        bitmap.getPixels(intValues, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());

        imgData.rewind();
        for (int i = 0; i < inputSize; ++i) {
            for (int j = 0; j < inputSize; ++j) {
                int pixelValue = intValues[i * inputSize + j];
                imgData.putFloat((pixelValue >> 16) & 0xFF);
                imgData.putFloat((pixelValue >> 8) & 0xFF);
                imgData.putFloat(pixelValue & 0xFF);
            }
        }
        return new Object[]{imgData};
    }
}