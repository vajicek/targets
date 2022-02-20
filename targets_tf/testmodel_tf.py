#!/usr/bin/python3

import argparse
import glob
import logging
import numpy as np
import re

import cv2
import tensorflow as tf
from object_detection.builders import model_builder
from object_detection.utils import config_util

LOGGER = logging.getLogger(__file__)


@tf.function
def detect_fn(image, detection_model):
    image, shapes = detection_model.preprocess(image)
    prediction_dict = detection_model.predict(image, shapes)
    detections = detection_model.postprocess(prediction_dict, shapes)
    return detections


def _get_pipeline_config(model):
    return "Tensorflow/workspace/models/%s/pipeline.config" % model


def _get_latest_checkpoint(model):
    pattern = re.compile(r'(.*-(.*)).index$')
    checkpoint_path = "Tensorflow/workspace/models/%s" % model
    files = glob.glob(checkpoint_path + "/ckpt-*.index")
    files.sort(key=lambda path: int(re.match(pattern, path).group(2)))
    return re.match(pattern, files[-1]).group(1)


def _detect(image_file, model):

    config_path = _get_pipeline_config(model)
    last_checkpoint_path = _get_latest_checkpoint(model)

    LOGGER.info("Detecting using config_path=%s, last_checkpoint_path=%s",
                config_path, last_checkpoint_path)

    configs = config_util.get_configs_from_pipeline_file(config_path)
    detection_model = model_builder.build(model_config=configs['model'], is_training=False)

    ckpt = tf.compat.v2.train.Checkpoint(model=detection_model)
    ckpt.restore(last_checkpoint_path).expect_partial()

    img = cv2.imread(image_file)
    image_np = np.array(img)

    input_tensor = tf.convert_to_tensor(np.expand_dims(image_np, 0), dtype=tf.float32)
    detections = detect_fn(input_tensor, detection_model)

    print(detections.keys())
    print(detections['detection_boxes'][:,:10,:])
    print(detections['detection_scores'][0,:10])


def main():
    logging.basicConfig(level=logging.INFO)

    parser = argparse.ArgumentParser(description='Convert saved model to tflite.')
    parser.add_argument('model', type=str, help='Model name')
    parser.add_argument('image', type=str, help='Input image where to run detection')
    args = parser.parse_args()
    _detect(args.image, args.model)


if __name__ == "__main__":
    main()
