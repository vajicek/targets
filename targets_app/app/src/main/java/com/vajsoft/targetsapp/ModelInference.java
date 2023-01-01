package com.vajsoft.targetsapp;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.util.Size;

import org.tensorflow.lite.Interpreter;
import org.tensorflow.lite.Tensor;
import org.tensorflow.lite.support.common.ops.NormalizeOp;
import org.tensorflow.lite.support.image.ImageProcessor;
import org.tensorflow.lite.support.image.ops.ResizeWithCropOrPadOp;

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;

import io.vavr.control.Try;

public class ModelInference {

    Interpreter interpreter;

    Context applicationContext;

    ImageProcessor imageProcessor;

    public ModelInference(final Context applicationContext, final Size inputImageSize) {
        this.applicationContext = applicationContext;
        this.imageProcessor = preprocessor(inputImageSize);
        Try.of(() -> loadBufferFromFileAsset(applicationContext.getAssets(), "detect.tflite"))
                .andThen(byteBuffer -> {
                    this.interpreter = new Interpreter(byteBuffer);
                    this.interpreter.allocateTensors();
                });
    }

    public CompletableFuture<ModelOutputs> run(final Bitmap bitmap) {
        return CompletableFuture.supplyAsync(() -> {
            final var outputs = new ModelOutputs(bitmap);
            printInterpreter(interpreter);
            interpreter.runForMultipleInputsOutputs(prepareInputs(bitmap), prepareOutputs(outputs));
            printOutputs(outputs);
            return outputs;
        });
    }

    private ImageProcessor preprocessor(final Size inputImageSize) {
        int width = inputImageSize.getWidth();
        int height = inputImageSize.getHeight();
        int cropSize = Math.min(width, height);
        return new ImageProcessor.Builder()
                .add(new ResizeWithCropOrPadOp(cropSize, cropSize))
                .add(new NormalizeOp(0.5f, 0.5f))
                .build();
    }

    private static ByteBuffer
    loadBufferFromFileAsset(final AssetManager assets,
                            final String modelFilename) throws IOException {
        AssetFileDescriptor fileDescriptor = assets.openFd(modelFilename);
        FileInputStream inputStream = new FileInputStream(fileDescriptor.getFileDescriptor());
        FileChannel fileChannel = inputStream.getChannel();
        long startOffset = fileDescriptor.getStartOffset();
        long declaredLength = fileDescriptor.getDeclaredLength();
        return fileChannel.map(FileChannel.MapMode.READ_ONLY, startOffset, declaredLength);
    }

    private Object[] prepareInputs(final Bitmap bitmap) {
        int numBytesPerChannel = 4;

        ByteBuffer imgData = ByteBuffer.allocateDirect(bitmap.getWidth() * bitmap.getHeight() * 3 * numBytesPerChannel);
        imgData.order(ByteOrder.nativeOrder());

        final var intValues = new int[bitmap.getWidth() * bitmap.getHeight()];
        bitmap.getPixels(intValues, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());

        imgData.rewind();
        for (int y = 0; y < bitmap.getHeight(); ++y) {
            for (int x = 0; x < bitmap.getWidth(); ++x) {
                final int pixelValue = intValues[y * bitmap.getWidth() + x];
                final var r = (pixelValue >> 16) & 0xFF;
                final var g = (pixelValue >> 8) & 0xFF;
                final var b = pixelValue & 0xFF;
                imgData.putFloat(r / 255.0f);
                imgData.putFloat(g / 255.0f);
                imgData.putFloat(b / 255.0f);
            }
        }

        return new Object[]{imgData};
    }

    private void printOutputs(ModelOutputs outputs) {
        System.out.println(outputs.toString());
    }

    private Map<Integer, Object> prepareOutputs(final ModelOutputs outputs) {
        Map<Integer, Object> outputMap = new HashMap<>();
        outputMap.put(0, outputs.outputScores);
        outputMap.put(1, outputs.outputLocations);
        outputMap.put(2, outputs.numDetections);
        outputMap.put(3, outputs.outputClasses);
        return outputMap;
    }

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

    private static void printTensor(final Tensor t) {
        System.out.println("dataType= " + t.dataType().toString());
        for (int i = 0; i < t.shape().length; i++) {
            System.out.println("shape[" + i + "]= " + t.shape()[i]);
        }
    }
}
