#!/bin/bash

set -x
set -e

#MODEL=my_ssd_resnet50
#MODEL=my_ssd_resnet101
#MODEL=my_ssd_mobnet
MODEL=my_ssd_mobnet_640

#python3 Tensorflow/models/research/object_detection/exporter_main_v2.py \
#--input_type=image_tensor \
#--pipeline_config_path=Tensorflow/workspace/models/${MODEL}/pipeline.config \
#--trained_checkpoint_dir=Tensorflow/workspace/models/${MODEL} \
#--output_directory=Tensorflow/workspace/models/${MODEL}/export

python3 Tensorflow/models/research/object_detection/export_tflite_graph_tf2.py \
--pipeline_config_path=Tensorflow/workspace/models/${MODEL}/pipeline.config \
--trained_checkpoint_dir=Tensorflow/workspace/models/${MODEL} \
--output_directory=Tensorflow/workspace/models/${MODEL}/tfliteexport

#~/.local/bin/tflite_convert \
#--saved_model_dir=Tensorflow/workspace/models/${MODEL}/tfliteexport/saved_model \
#--output_file=Tensorflow/workspace/models/${MODEL}/tfliteexport/saved_model/detect.tflite
#\
#--input_shapes=1,640,640,3 \
#--input_arrays=normalized_input_image_tensor \
#--output_arrays='TFLite_Detection_PostProcess','TFLite_Detection_PostProcess:1','TFLite_Detection_PostProcess:2','TFLite_Detection_PostProcess:3'  \
#--inference_type=QUANTIZED_UINT8 \
#--mean_values=128 \
#--std_values=128 \
#--default_ranges_min=0 \
#--default_ranges_max=255 \
#--change_concat_input_ranges=false \
#--allow_custom_ops

#python3 Tensorflow/models/research/object_detection/export_inference_graph.py \
#--input_type image_tensor \
#--pipeline_config_path Tensorflow/workspace/models/my_ssd_resnet50/pipeline.config \
#--trained_checkpoint_prefix Tensorflow/workspace/models/my_ssd_resnet50/ckpt-5 \
#--output_directory Tensorflow/workspace/models/my_ssd_resnet50/export
#
#python3 Tensorflow/models/research/object_detection/export_tflite_ssd_graph.py \
#--pipeline_config_path=Tensorflow/workspace/models/my_ssd_resnet50/pipeline.config \
#--trained_checkpoint_prefix=Tensorflow/workspace/models/my_ssd_resnet50/ckpt-3 \
#--output_directory=Tensorflow/workspace/models/my_ssd_resnet50/tfliteexport \
#--add_postprocessing_op=true
#
#~/.local/bin/tflite_convert --saved_model_dir=Tensorflow/workspace/models/my_ssd_resnet50/tfliteexport/saved_model \
#--output_file=Tensorflow/workspace/models/my_ssd_resnet50/tfliteexport/saved_model/detect.tflite \
#--output_format=TFLITE \
#--input_shapes=1,640,640,3 \
#--input_arrays=normalized_input_image_tensor \
#--output_arrays='TFLite_Detection_PostProcess','TFLite_Detection_PostProcess:1','TFLite_Detection_PostProcess:2','TFLite_Detection_PostProcess:3' \
#--inference_type=FLOAT --mean_values=128 --std_dev_values=127 --change_concat_input_ranges=false --allow_custom_ops


# \
#--input_shapes=1,640,640,3 \
#--input_arrays=normalized_input_image_tensor \
#--output_arrays='TFLite_Detection_PostProcess','TFLite_Detection_PostProcess:1','TFLite_Detection_PostProcess:2','TFLite_Detection_PostProcess:3' \
#--inference_type=FLOAT \
#--allow_custom_ops

#--enable_v1_converter \


#python3 Tensorflow/models/research/object_detection/model_main_tf2.py \
#--model_dir=Tensorflow/workspace/models/my_ssd_resnet50 \
#--pipeline_config_path=Tensorflow/workspace/models/my_ssd_resnet50/pipeline.config \
#--num_train_steps=2000
