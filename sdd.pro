#-------------------------------------------------
# Merged Qt + OpenCV project file (Qt 5/6 + MinGW)
#-------------------------------------------------

QT       += core gui widgets \
            opengl openglwidgets \
            multimedia multimediawidgets


CONFIG   += c++17 console
CONFIG   -= app_bundle

TEMPLATE = app
TARGET   = PalmDetectorGame

#----- OpenCV setup -----
# adjust to wherever you installed OpenCV
OPENCV_DIR = C:/opencv/opencv-4.10.0/build/install

INCLUDEPATH += $$OPENCV_DIR/include

# Link against the OpenCV .dll.a import libraries
LIBS += -L$$OPENCV_DIR/x64/mingw/lib \
        -lopencv_core4100 \
        -lopencv_highgui4100 \
        -lopencv_imgproc4100 \
        -lopencv_imgcodecs4100 \
        -lopencv_videoio4100 \
        -lopencv_features2d4100 \
        -lopencv_calib3d4100 \
        -lopencv_objdetect4100 \
        -lopencv_video4100 \
        -lopengl32

#----- sources / headers / forms -----
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    gamescene.cpp \
    projectile.cpp \
    sword.cpp \
    test_detectmultiscale.cpp
      # test_detectmultiscale.cpp # <-- your palm-detect demo

HEADERS += \
    camera_window.h \
    mainwindow.h \
    gamescene.h \
    projectile.h \
    sword.h \
    test_detectmultiscale.h

FORMS   += mainwindow.ui

# Optional: disable deprecated Qt 5 APIs
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
