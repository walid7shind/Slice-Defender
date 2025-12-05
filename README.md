🎮 Overview

This project demonstrates how Computer Vision can be used to control a video game in real time using only a webcam.
The player’s palm position is tracked through the video stream (OpenCV), mapped to in-game coordinates (Qt + OpenGL), and used to move a virtual sword inside the game scene.

It is a complete pipeline including:

Camera capture (OpenCV VideoCapture)

Skin segmentation + contour analysis

Fallback Haar Cascade palm detection

3D game scene rendering (Qt/OpenGL)

Real-time interaction (Qt event loop)

👉 A short gameplay video is included in the /video folder.
View it to see how the palm controls the sword in real time.

🚀 Features

Real-time palm detection

YCrCb skin segmentation

Morphology + contour filtering

Distance transform to find palm center

Haar cascade fallback (palm.xml)

Camera feed displayed inside the Qt UI

3D Interactive Game Scene

Sword position updated from palm coordinates

Smooth mirrored tracking

Qt-based GUI (side panel, score, timer, camera preview)

~30 FPS frame processing loop using QTimer

🛠️ Technologies Used
Component	Technology
Computer Vision	OpenCV (VideoCapture, CLAHE, YCrCb segmentation, Haar Cascade)
UI / Event Loop	Qt 6 (QMainWindow, QWidget, QHBoxLayout, QVBoxLayout)
3D Rendering	OpenGL (via Qt)
Build System	qmake / .pro
Language	C++17
📂 Project Structure (Main Files)
/src
 ├── main.cpp                // Starts the Qt application
 ├── mainwindow.h/.cpp       // Main window, UI, and game integration
 ├── gamescene.h/.cpp        // OpenGL 3D scene (sword movement)
 ├── camera_window.h         // Embedded camera feed widget
 ├── test_detectmultiscale.h/.cpp // PalmDetector implementation
 ├── assets/                 // Textures, sprites, models
 ├── palm.xml                // Haar cascade for fallback palm detection
 └── video/                  // Gameplay demonstration video

📸 Palm Detection Pipeline

Capture frame from webcam

Convert to YCrCb

Apply CLAHE on Cr/Cb channels

Threshold for skin mask

Clean with morphological operations

Contour filtering to isolate palm

Use distance transform to locate palm center

If no palm is found → fallback to Haar Cascade (detectMultiScale)

🔧 How to Build
Prerequisites

Qt 6 or Qt 5

OpenCV 4.x

C++17 compiler

qmake

Build Instructions
qmake sdd.pro
make
./sdd

🎮 How to Play

Stand in front of your webcam

Move your palm in the camera area

The sword in the game will follow your hand

Strike the objects on screen to increase your score

Timer shows elapsed time

🎥 Gameplay Video

A demonstration video is included:

video/gameplay.mp4


👉 Open it to see a real capture of the game being played with the palm.

👤 Credits

Developed using:

Qt

OpenCV

C++
