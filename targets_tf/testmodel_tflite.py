#!/usr/bin/python3

import argparse
import numpy as np
import tensorflow as tf
from tflite_runtime.interpreter import Interpreter


def _get_tflite_model(model):
    return "Tensorflow/workspace/models/%s/tfliteexport/saved_model/detect.tflite" % model


def _preprocess_image(image_path):
    img = tf.io.read_file(image_path)
    img = tf.io.decode_image(img, channels=3)
    img = tf.image.convert_image_dtype(img, tf.float32)
    original_image = img
    resized_img = tf.image.resize(img, (640, 640))
    resized_img = resized_img[tf.newaxis, :]
    return resized_img, original_image


def _get_output_tensor(interpreter, index):
    output_details = interpreter.get_output_details()[index]
    tensor = np.squeeze(interpreter.get_tensor(output_details['index']))
    return tensor


def _detect_objects(interpreter, image, threshold):
    interpreter.set_tensor(interpreter.get_input_details()[0]['index'], image)
    interpreter.invoke()

    scores = _get_output_tensor(interpreter, 0)
    boxes = _get_output_tensor(interpreter, 1)
    count = int(get_output_tensor(interpreter, 2))
    classes = get_output_tensor(interpreter, 3)
    results = []
    for i in range(count):
        if scores[i] >= threshold:
            result = {
                'bounding_box': boxes[i],
                'class_id': classes[i],
                'score': scores[i]
            }
            results.append(result)
    return results


def _detect(image_file, model):
    interpreter = Interpreter(_get_tflite_model(model))
    interpreter.allocate_tensors()
    _, input_height, input_width, _ = interpreter.get_input_details()[0]['shape']

    resized_img, original_image = _preprocess_image(image_file)
    res = _detect_objects(interpreter, resized_img, 0.001)
    for r in res:
        print(r)


def main():
    parser = argparse.ArgumentParser(description='Convert saved model to tflite.')
    parser.add_argument('model', type=str, help='Model name')
    parser.add_argument('image', type=str, help='Input image where to run detection')
    args = parser.parse_args()
    _detect(args.image, args.model)


if __name__ == "__main__":
    main()
