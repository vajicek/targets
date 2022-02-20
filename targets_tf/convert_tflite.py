#!/usr/bin/python3

import argparse
import tensorflow as tf


def _get_saved_model(model):
    return "Tensorflow/workspace/models/%s/tfliteexport/saved_model" % model


def _get_tflite_model(model):
    return "Tensorflow/workspace/models/%s/tfliteexport/saved_model/detect.tflite" % model


def _convert(model):
    converter = tf.lite.TFLiteConverter.from_saved_model(_get_saved_model(model))
    converter.experimental_new_converter = True
    tflite_model = converter.convert()
    with open(_get_tflite_model(model), 'wb') as f:
        f.write(tflite_model)


def main():
    parser = argparse.ArgumentParser(description='Convert saved model to tflite.')
    parser.add_argument('model', type=str, help='Model name')
    args = parser.parse_args()
    _convert(args.model)


if __name__ == "__main__":
    main()
