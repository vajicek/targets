#!/usr/bin/python3
"""Generate tfrecord from annotated data."""

import argparse
import glob
import io
import json
import os
import xml.etree.ElementTree as ET

from collections import namedtuple

import pandas as pd

import tensorflow.compat.v1 as tf
from PIL import Image
from object_detection.utils import dataset_util, label_map_util


def xml_to_dataframe(path):
	row_list = []
	for xml_file in glob.glob(path + '/*.xml'):
		tree = ET.parse(xml_file)
		root = tree.getroot()
		for member in root.findall('object'):
			bndbox=member.find('bndbox')
			value = (root.find('filename').text,
					 int(root.find('size')[0].text),
					 int(root.find('size')[1].text),
					 member.find('name').text,
					 int(bndbox.find('xmin').text),
					 int(bndbox.find('ymin').text),
					 int(bndbox.find('xmax').text),
					 int(bndbox.find('ymax').text)
					 )
			row_list.append(value)
	column_name = ['filename', 'width', 'height', 'class', 'xmin', 'ymin', 'xmax', 'ymax']
	return pd.DataFrame(row_list, columns=column_name)


def json_to_dataframe(path, point_to_rect=15):
	row_list = []
	for json_file in glob.glob(path + '/*.json'):
		with open(json_file, 'r') as read_file:
			json_data = json.load(read_file)
			for shape in json_data['shapes']:
				label = shape['label']
				if shape['shape_type'] == 'point':
					coords = [float(number) for number in shape['points'][0]]
					row = (json_data['imagePath'],
						int(json_data['imageWidth']),
						int(json_data['imageHeight']),
						label,
						int(coords[0]),
						int(coords[1]),
						int(coords[0]) - point_to_rect,
						int(coords[1]) - point_to_rect,
						int(coords[0]) + point_to_rect,
						int(coords[1]) + point_to_rect)
				if shape['shape_type'] == 'rectangle':
					coords0 = [float(number) for number in shape['points'][0]]
					coords1 = [float(number) for number in shape['points'][1]]
					row = (json_data['imagePath'],
						int(json_data['imageWidth']),
						int(json_data['imageHeight']),
						label,
						int(sum([coords0[0], coords1[0]]) / 2),
						int(sum([coords0[1], coords1[1]]) / 2),
						int(min(coords0[0], coords1[0])),
						int(min(coords0[1], coords1[1])),
						int(max(coords0[0], coords1[0])),
						int(max(coords0[1], coords1[1])))
				row_list.append(row)

	column_name = ['filename', 'width', 'height', 'class', 'x', 'y', 'xmin', 'ymin', 'xmax', 'ymax']
	return pd.DataFrame(row_list, columns=column_name)


def class_text_to_int(row_label, label_map_dict):
	return label_map_dict[row_label]


def split(examples, group):
	data = namedtuple('data', ['filename', 'object'])
	groupedby = examples.groupby(group)
	zipped = zip(groupedby.groups.keys(), groupedby.groups)
	return [data(filename, groupedby.get_group(x)) for filename, x in zipped]


def create_tf_example(group, path, label_map_dict):
	with tf.gfile.GFile(os.path.join(path, '{}'.format(group.filename)), 'rb') as fid:
		encoded_jpg = fid.read()
	encoded_jpg_io = io.BytesIO(encoded_jpg)
	image = Image.open(encoded_jpg_io)
	width, height = image.size

	filename = group.filename.encode('utf8')
	image_format = b'png'
	xmins = []
	xmaxs = []
	ymins = []
	ymaxs = []
	classes_text = []
	classes = []

	for _, row in group.object.iterrows():
		xmins.append(row['xmin'] / width)
		xmaxs.append(row['xmax'] / width)
		ymins.append(row['ymin'] / height)
		ymaxs.append(row['ymax'] / height)
		classes_text.append(row['class'].encode('utf8'))
		classes.append(class_text_to_int(row['class'], label_map_dict))

	tf_example = tf.train.Example(features=tf.train.Features(feature={
		'image/height': dataset_util.int64_feature(height),
		'image/width': dataset_util.int64_feature(width),
		'image/filename': dataset_util.bytes_feature(filename),
		'image/source_id': dataset_util.bytes_feature(filename),
		'image/encoded': dataset_util.bytes_feature(encoded_jpg),
		'image/format': dataset_util.bytes_feature(image_format),
		'image/object/bbox/xmin': dataset_util.float_list_feature(xmins),
		'image/object/bbox/xmax': dataset_util.float_list_feature(xmaxs),
		'image/object/bbox/ymin': dataset_util.float_list_feature(ymins),
		'image/object/bbox/ymax': dataset_util.float_list_feature(ymaxs),
		'image/object/class/text': dataset_util.bytes_list_feature(classes_text),
		'image/object/class/label': dataset_util.int64_list_feature(classes),
	}))
	return tf_example


def get_args():
	parser = argparse.ArgumentParser(
		description="Sample TensorFlow XML-to-TFRecord converter")
	input_dir = parser.add_mutually_exclusive_group(required=True)
	input_dir.add_argument("-x", "--xml_dir", type=str,
		help="Path to the folder where the input .xml files are stored.")
	input_dir.add_argument("-j", "--json_dir", type=str,
		help="Path to the folder where the input .xml files are stored.")
	parser.add_argument("-l", "--labels_path", type=str, required=True,
		help="Path to the labels (.pbtxt) file.")
	parser.add_argument("-o", "--output_path", type=str, required=True,
		help="Path of output TFRecord (.record) file.")
	parser.add_argument("-i", "--image_dir", type=str, default=None,
		help="Path to the folder where the input image files are stored. Defaults to the same directory as XML_DIR.")
	parser.add_argument("-c", "--csv_path", type=str, default=None,
		help="Path of output .csv file. If none provided, then no file will be written.")

	args = parser.parse_args()

	if args.image_dir is None:
		args.image_dir = args.xml_dir or args.json_dir

	return args


def write_tfrecord(output_path, examples, image_dir, label_map_dict):
	grouped = split(examples, 'filename')
	writer = tf.python_io.TFRecordWriter(output_path)
	for group in grouped:
		tf_example = create_tf_example(group, image_dir, label_map_dict)
		writer.write(tf_example.SerializeToString())
	writer.close()
	print('Successfully created the TFRecord file: {}'.format(output_path))


def write_csv(csv_path, examples):
	examples.to_csv(csv_path, index=None)
	print('Successfully created the CSV file: {}'.format(csv_path))


def get_label_map_dict(label_path):
	label_map = label_map_util.load_labelmap(label_path)
	return label_map_util.get_label_map_dict(label_map)


def json_to_tfrecord(image_dir, label_filename, input_filename, output_filename, point_to_rect):
	label_map_dict = get_label_map_dict(label_filename)
	examples = json_to_dataframe(input_filename, point_to_rect)
	write_tfrecord(output_filename, examples, image_dir, label_map_dict)


def xml_to_tfrecord(image_dir, label_filename, input_filename, output_filename):
	label_map_dict = get_label_map_dict(label_filename)
	examples = xml_to_dataframe(input_filename)
	write_tfrecord(output_filename, examples, image_dir, label_map_dict)


def main():
	args = get_args()

	label_map_dict = get_label_map_dict(args.labels_path)
	if args.xml_dir:
		examples = xml_to_dataframe(args.xml_dir)
	elif args.json_dir:
		examples = json_to_dataframe(args.json_dir)

	if args.output_path:
		write_tfrecord(args.output_path, examples, args.image_dir, label_map_dict)
	if args.csv_path:
		write_csv(args.csv_path, examples)


if __name__ == '__main__':
	main()
