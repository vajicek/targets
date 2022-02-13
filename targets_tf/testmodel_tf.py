#!/usr/bin/python3

import cv2
import numpy as np
import os
import tensorflow as tf
from object_detection.builders import model_builder
from object_detection.utils import config_util

#CHECKPOINT_PATH = "Tensorflow/workspace/models/my_ssd_resnet101"
CHECKPOINT_PATH = "Tensorflow/workspace/models/my_ssd_mobnet_640"
PIPELINE_CONFIG = CHECKPOINT_PATH + "/pipeline.config"

configs = config_util.get_configs_from_pipeline_file(PIPELINE_CONFIG)
detection_model = model_builder.build(model_config=configs['model'], is_training=False)

ckpt = tf.compat.v2.train.Checkpoint(model=detection_model)
ckpt.restore(os.path.join(CHECKPOINT_PATH, 'ckpt-3')).expect_partial()

@tf.function
def detect_fn(image):
    image, shapes = detection_model.preprocess(image)
    prediction_dict = detection_model.predict(image, shapes)
    detections = detection_model.postprocess(prediction_dict, shapes)
    return detections

img = cv2.imread('Tensorflow/workspace/images/test/IMG_20210706_093257.jpg')
image_np = np.array(img)

input_tensor = tf.convert_to_tensor(np.expand_dims(image_np, 0), dtype=tf.float32)
detections = detect_fn(input_tensor)

print(detections['detection_boxes'][:,:10,:])
