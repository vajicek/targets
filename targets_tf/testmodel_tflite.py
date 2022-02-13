#!/usr/bin/python3

import numpy as np
import tensorflow as tf
from tflite_runtime.interpreter import Interpreter


def set_input_tensor(interpreter, image):
  tensor_index = interpreter.get_input_details()[0]['index']
  input_tensor = interpreter.tensor(tensor_index)()[0]
  input_tensor[:, :] = np.expand_dims((image - 255) / 255, axis=0)

def preprocess_image(image_path):
    img = tf.io.read_file(image_path)
    img = tf.io.decode_image(img, channels=3)
    img = tf.image.convert_image_dtype(img, tf.float32)
    original_image = img
    resized_img = tf.image.resize(img, (640, 640))
    resized_img = resized_img[tf.newaxis, :]
    return resized_img, original_image

# def set_input_tensor(interpreter, image):
#   tensor_index = interpreter.get_input_details()[0]['index']
#   #input_tensor = interpreter.tensor(tensor_index)()[0]
#   #input_tensor[:, :] = np.expand_dims((image-255)/255, axis=0)
#   img = np.expand_dims((image - 255) / 255, axis=0).astype(np.float32)
#   #input_details = interpreter.get_input_details()
#   interpreter.set_tensor(tensor_index, img)

def get_output_tensor(interpreter, index):
  output_details = interpreter.get_output_details()[index]
  tensor = np.squeeze(interpreter.get_tensor(output_details['index']))
  return tensor

def detect_objects(interpreter, image, threshold):
  set_input_tensor(interpreter, image)
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


IMAGE_PATH='Tensorflow/workspace/images/test/IMG_20210706_093257.jpg'

def convert():
    converter = tf.lite.TFLiteConverter.from_saved_model("Tensorflow/workspace/models/my_ssd_resnet50/export/saved_model")
    converter.experimental_new_converter = True
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS,
                                           tf.lite.OpsSet.SELECT_TF_OPS]
    tflite_model = converter.convert()
    with open('Tensorflow/workspace/models/my_ssd_resnet50/tfliteexport/saved_model/detect.tflite', 'wb') as f:
      f.write(tflite_model)

def main():
    interpreter = Interpreter('Tensorflow/workspace/models/my_ssd_mobnet_640/tfliteexport/saved_model/detect.tflite')
    #interpreter = Interpreter('Tensorflow/workspace/models/my_ssd_mobnet/tfliteexport/saved_model/detect.tflite')
    #interpreter = Interpreter('Tensorflow/workspace/models/my_ssd_resnet101/tfliteexport/saved_model/detect.tflite')
    #interpreter = Interpreter('Tensorflow/workspace/models/my_ssd_resnet50/tfliteexport/saved_model/detect.tflite')
    interpreter.allocate_tensors()
    _, input_height, input_width, _ = interpreter.get_input_details()[0]['shape']

    print(input_height, input_width)

    resized_img, original_image = preprocess_image(IMAGE_PATH)
    #img = cv2.resize(cv2.cvtColor(cv2.imread(IMAGE_PATH), cv2.COLOR_BGR2RGB), (input_height, input_width))
    #img = cv2.resize(cv2.imread(IMAGE_PATH), (input_height, input_width))
    #print(img.shape)

    res = detect_objects(interpreter, resized_img, 0.001)
    for r in res:
        print(r)

if __name__ == "__main__":
    #convert()
    main()
