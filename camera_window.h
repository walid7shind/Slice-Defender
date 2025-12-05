#ifndef CAMERAWINDOW_H
#define CAMERAWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

/*
 * Classe CameraWindow :
 * → Widget Qt embarqué pour afficher le flux vidéo de la caméra.
 */
class CameraWindow : public QWidget {
    Q_OBJECT
public:
    explicit CameraWindow(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_label(new QLabel(this))
    {
        // Pas de titre de fenêtre quand intégré dans un layout
        // setWindowTitle("Camera Feed");

        // Taille fixe pour le placeholder du flux (320×240 px)
        m_label->setFixedSize(320, 240);
        // Bordure verte pour délimiter la zone de la vidéo
        m_label->setStyleSheet("border:2px solid green;");

        // Layout vertical sans marges pour un embedding serré
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_label);
        setLayout(layout);
    }

    // Expose le QLabel pour que MainWindow puisse y afficher la frame caméra
    QLabel* label() const { return m_label; }

private:
    QLabel* m_label;  // Affiche le feed vidéo
};

#endif // CAMERAWINDOW_H
