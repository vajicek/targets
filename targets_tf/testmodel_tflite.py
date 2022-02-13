#!/usr/bin/python3

import glob
import numpy as np
import tensorflow as tf
from tflite_runtime.interpreter import Interpreter

def preprocess_image(image_path):
    img = tf.io.read_file(image_path)
    img = tf.io.decode_image(img, channels=3)
    img = tf.image.convert_image_dtype(img, tf.float32)
    original_image = img
    resized_img = tf.image.resize(img, (640, 640))
    resized_img = resized_img[tf.newaxis, :]
    return resized_img, original_image

def get_output_tensor(interpreter, index):
    output_details = interpreter.get_output_details()[index]
    tensor = np.squeeze(interpreter.get_tensor(output_details['index']))
    return tensor

def detect_objects(interpreter, image, threshold):
    interpreter.set_tensor(interpreter.get_input_details()[0]['index'], image)
    interpreter.invoke()

    scores = get_output_tensor(interpreter, 0)
    boxes = get_output_tensor(interpreter, 1)
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

def convert(model='my_ssd_mobnet_640'):
    converter = tf.lite.TFLiteConverter.from_saved_model("Tensorflow/workspace/models/" + model + "/tfliteexport/saved_model")
    converter.experimental_new_converter = True
    tflite_model = converter.convert()
    with open('Tensorflow/workspace/models/' + model + '/tfliteexport/saved_model/detect.tflite', 'wb') as f:
      f.write(tflite_model)

def detect(image_file, model='my_ssd_mobnet_640'):
    interpreter = Interpreter('Tensorflow/workspace/models/' + model + '/tfliteexport/saved_model/detect.tflite')
    interpreter.allocate_tensors()
    _, input_height, input_width, _ = interpreter.get_input_details()[0]['shape']

    resized_img, original_image = preprocess_image(image_file)
    res = detect_objects(interpreter, resized_img, 0.001)
    for r in res:
        print(r)

if __name__ == "__main__":
    model = 'my_ssd_mobnet_640'
    #convert(model)

    IMAGE_PATH=glob.glob('Tensorflow/workspace/images/test/*.jpg')[0]
    print(IMAGE_PATH)

    detect(model, IMAGE_PATH)
