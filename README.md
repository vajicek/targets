# Targets - AI driven archery scoring

## Idea - a picture is worth a thousand words

![idea](doc/img/idea.svg)


## How to do it

* Use object detection api in TensorFlow (v2.0+)
  * Train model on given annotated examples
  * Deploy model either on mobile device (TensorFlow Lite) or online (as service)
* Implement modile (Android) application
  * Capture image from camera
  * Process image, send it to model for classification


## Preliminary results

### Detection using MobileNet SSD model with Tensorflow (2.7), running on PC.
![idea](doc/img/preliminary_tf.png)

### Detection using MobileNet SSD model with Tensorflow Lite (2.7), still running on PC.
![idea](doc/img/preliminary_tflite.png)
