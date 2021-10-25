#!/usr/bin/python3

import tensorflow as tf
from tensorflow.python.client import device_lib
a=tf.config.experimental.list_physical_devices(device_type="GPU")
print(a)
print(tf. __version__)
