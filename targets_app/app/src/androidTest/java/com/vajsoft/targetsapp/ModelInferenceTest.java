package com.vajsoft.targetsapp;

import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.graphics.BitmapFactory;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.tensorflow.lite.Interpreter;
import org.tensorflow.lite.Tensor;

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.util.HashMap;
import java.util.Map;

@RunWith(AndroidJUnit4.class)
public class ModelInferenceTest {
    @Test
    public void testModelInference() throws IOException {
        final var appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        final var byteBuffer = loadBufferFromFileAsset(appContext.getAssets(), "detect.tflite");
        final var imageByteBuffer = loadBufferFromFileAsset(appContext.getAssets(), "testdata2.jpg");

        final var interpreter = new Interpreter(byteBuffer);
        printInterpreter(interpreter);
        final var outputs = new Outputs();
        interpreter.runForMultipleInputsOutputs(prepareInputs(imageByteBuffer), prepareOutputs(outputs));
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

    Object[] prepareInputs(ByteBuffer imageByteBuffer) {
        int numBytesPerChannel = 4;
        int inputSize = 640;
        ByteBuffer imgData = ByteBuffer.allocateDirect(1 * inputSize * inputSize * 3 * numBytesPerChannel);

        byte[] bytes = new byte[imageByteBuffer.capacity()];
        imageByteBuffer.get(bytes);

        final var bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.length);
        final var intValues = new int[inputSize * inputSize];
        bitmap.getPixels(intValues, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());

        imgData.rewind();
        for (int i = 0; i < inputSize; ++i) {
            for (int j = 0; j < inputSize; ++j) {
                int pixelValue = intValues[i * inputSize + j];
                imgData.putFloat((((pixelValue >> 16) & 0xFF) - IMAGE_MEAN) / IMAGE_STD);
                imgData.putFloat((((pixelValue >> 8) & 0xFF) - IMAGE_MEAN) / IMAGE_STD);
                imgData.putFloat(((pixelValue & 0xFF) - IMAGE_MEAN) / IMAGE_STD);
            }
        }
        return new Object[]{imgData};
    }

    private static final int NUM_DETECTIONS = 10;
    private static final float IMAGE_MEAN = 127.5f;
    private static final float IMAGE_STD = 127.5f;

    private static void printInterpreter(Interpreter interpreter) {
        System.out.println("interpreter.getInputTensorCount() = " + interpreter.getInputTensorCount());
        for (int i = 0; i < interpreter.getInputTensorCount(); i++) {
            System.out.println("getInputTensor(" + i + ")");
            printTensor(interpreter.getInputTensor(i));
        }
        System.out.println("interpreter.getOutputTensorCount() = " + interpreter.getOutputTensorCount());
        for (int i = 0; i < interpreter.getOutputTensorCount(); i++) {
            System.out.println("getOutputTensor(" + i + ")");
            printTensor(interpreter.getOutputTensor(i));
        }
    }

    private static void printTensor(Tensor t) {
        System.out.println("dataType= " + t.dataType().toString());
        for(int i = 0; i < t.shape().length;i++) {
            System.out.println("shape[" + i +"]= " + t.shape()[i]);
        }
    }

    private static MappedByteBuffer loadBufferFromFileAsset(AssetManager assets, String modelFilename)
            throws IOException {
        AssetFileDescriptor fileDescriptor = assets.openFd(modelFilename);
        FileInputStream inputStream = new FileInputStream(fileDescriptor.getFileDescriptor());
        FileChannel fileChannel = inputStream.getChannel();
        long startOffset = fileDescriptor.getStartOffset();
        long declaredLength = fileDescriptor.getDeclaredLength();
        return fileChannel.map(FileChannel.MapMode.READ_ONLY, startOffset, declaredLength);
    }
}
