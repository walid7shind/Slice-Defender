#include "mainwindow.h"
#include "camera_window.h"
#include "gamescene.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include <opencv2/imgproc.hpp>
#include <QtMath>
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , scene(nullptr)
    , sidePanel(nullptr)
    , sideLayout(nullptr)
    , cameraWindow(nullptr)
    , scoreLabel(nullptr)
    , detector(nullptr)
    , timer(nullptr)
{
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout* mainLay = new QHBoxLayout(central);
    mainLay->setContentsMargins(0, 0, 0, 0);

    scene = new GameScene(this);
    mainLay->addWidget(scene, /*stretch*/ 4);

    sidePanel = new QWidget(central);
    sidePanel->setStyleSheet("background-color: black;");
    sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(5, 5, 5, 5);


    cameraWindow = new CameraWindow(this);
    sideLayout->addWidget(cameraWindow->label(), /*stretch*/ 1);


    scoreLabel = new QLabel("Score: 0", sidePanel);
    scoreLabel->setAlignment(Qt::AlignCenter);
    scoreLabel->setStyleSheet("color: white; font: 16pt;");
    sideLayout->addWidget(scoreLabel, /*stretch*/ 1);

    timeLabel = new QLabel("Temps: 0.00 s", sidePanel);
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setStyleSheet("color: white; font: 14pt;");
    sideLayout->addWidget(timeLabel, /*stretch*/ 0);

    mainLay->addWidget(sidePanel, /*stretch*/ 1);


    try {
        detector = new PalmDetector(
            1,
            "C:/Users/khali/dev/sd lakheeer/palm.xml"
            );
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }



    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateFrame);
    timer->start(33);


    connect(scene, &GameScene::scoreChanged,
            this, [this](int s){ scoreLabel->setText(QString("Score: %1").arg(s)); });

    connect(scene, &GameScene::elapsedTimeChanged,
            this, [this](float t){
                int minutes = int(t) / 60;
                int seconds = int(t) % 60;
                int centis  = int((t - floor(t)) * 100);
                timeLabel->setText(
                    QString("Temps : %1:%2.%3")
                        .arg(minutes, 2, 10, QChar('0'))
                        .arg(seconds, 2, 10, QChar('0'))
                        .arg(centis,  2, 10, QChar('0'))
                    );
            });

}

MainWindow::~MainWindow() {
    if (timer) timer->stop();
    delete detector;
    delete scene;

}

void MainWindow::updateFrame() {
    cv::Mat frame;
    std::vector<cv::Point> centers;
    if (!detector || !detector->getAnnotatedFrame(frame, centers))
        return;

    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows,
               static_cast<int>(rgb.step),
               QImage::Format_RGB888);

    QImage disp = img.mirrored(true, false);
    cameraWindow->label()->setPixmap(
        QPixmap::fromImage(disp)
            .scaled(cameraWindow->label()->size(),
                    Qt::KeepAspectRatio));

    if (!centers.empty()) {
        const auto& c = centers[0];

        // raw normalized coords
        float nx = float(c.x) / frame.cols;
        float ny = float(c.y) / frame.rows;

        float nx_mirror = 1.0f - nx;
        float x_world  = (nx_mirror - 0.5f) * 10.0f;
        float y_world  = (0.5f - ny)     * 10.0f;

        const float radius = 4.0f;
        float disc     = radius*radius - x_world*x_world;
        float z_offset = (disc > 0.0f ? qSqrt(disc) : 0.0f);
        float z_world  = 11.0f - z_offset;

        scene->updateSwordPosition({ x_world, y_world, z_world });
    }
}
