# Arrow detection using Tensorflow Object Detection API

## Notebooks

* setup_object_detection.ipynb - Setup object detection API and install prerequisites
* train_arrow_detection.ipynb - Train TF model
* detect_arrows.ipynb - Run detection using TF and TF lite models

## TF lite conversion

1. Convert TF model to saved model using `export_saved_tflite.sh`.
2. Convert saved model to TF lite using 'convert_tflite.py'

## Test detection via scripts

* testmodel_tflite.py - Test Tensorflow Lite model
* testmodel_tf.py - Test Tensorflow model

## Support

* test_gpu.py - Check if GPU is available on system.
* restartnvidia.sh - If nvidia driver is not found by Tensorflow, this will reinsert nvidia_uvm module.
* runjupyter.sh - Run jupyter notebook.
* Makefile - run jupyter, annotation, cleanup jupyter notebooks
