#!/bin/bash

set -x
set -e

MODEL=${1:-my_ssd_mobnet_640}

python3 Tensorflow/models/research/object_detection/export_tflite_graph_tf2.py \
--pipeline_config_path=Tensorflow/workspace/models/${MODEL}/pipeline.config \
--trained_checkpoint_dir=Tensorflow/workspace/models/${MODEL} \
--output_directory=Tensorflow/workspace/models/${MODEL}/tfliteexport
