# AI-Based Real-Time Object Detection LED Alert System

## Project Overview

This project is an AI-based real-time object detection and LED alert system using ESP32-CAM and TinyML.

The system uses an Edge Impulse image-classification model to detect two classes:

- Person
- Background

When a person is detected with sufficient confidence, the ESP32-CAM turns ON an LED and optional buzzer.

## Hardware

- ESP32-CAM (AI-Thinker)
- External LED
- Optional buzzer
- 5V/2A external power supply

## AI / Software

- Edge Impulse
- TensorFlow Lite Micro
- Arduino IDE
- C++
- ESP32-CAM

## Working

Camera captures an image
↓
Image preprocessing
↓
AI classification
↓
Person detected?
↓
YES → LED and buzzer ON
NO → LED and buzzer OFF

## Detection Threshold

The current confidence threshold is:

0.75

## Repository Contents

- `AI_Object_Detection_LED_Alert.ino` - Main ESP32-CAM code
- `Activity2_Report.docx` - Project Management Activity 2 report

## QA

Five hardware/software integration issues were identified and documented using GitHub Issues with labels, severity and root-cause analysis.
