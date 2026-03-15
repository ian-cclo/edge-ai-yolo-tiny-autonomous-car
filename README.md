# Edge AI Autonomous Driving using YOLO Tiny

This project demonstrates an Edge AI autonomous driving system based on:  

- YOLOv7-tiny neural network  
- Realtek Ameba Pro2 AI processor  
- Robot car platform  

The system performs real-time object detection and driving decision on embedded hardware. 


## System Architecture

```
Camera
   │
   ▼
Ameba Pro2 (Edge AI)
   │
   ├─ YOLO Tiny inference
   │
   └─ driving decision
         │
         ▼
Motor control
``` 

## Features
- Real-time object detection on edge device
- YOLOv7-tiny neural network
- Embedded AI inference
- Autonomous driving decision logic
- Low-power edge computing

## Model Training

The object detection model is trained using YOLOv7-tiny.

Dataset:
- traffic signs
- obstacles
- lane markers

Training environment:
- PyTorch
- CUDA GPU

Export:
.pt → ONNX → Ameba Pro2 NN model


## Hardware Platform

Main controller:
Realtek Ameba Pro2

Robot platform:
Matrix R4 autonomous car

Sensors:
- Camera
- Motor driver
