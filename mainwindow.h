#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include "gamescene.h"
#include "camera_window.h"
#include "test_detectmultiscale.h"  // Pour la détection de la main (PalmDetector)

/*
 * Classe MainWindow :
 * Gère la fenêtre principale, intègre la scène 3D, le flux caméra et le panel UI.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr); // Configure layout & initialisation
    ~MainWindow();                                  // Nettoyage des ressources

private slots:
    void updateFrame();  // Slot appelé périodiquement pour récupérer et afficher la frame

private:
    //--- Scène de jeu 3D ---
    GameScene*    scene;         // Widget OpenGL de la scène

    //--- Panel latéral (score, timer, camera feed) ---
    QWidget*      sidePanel;     // Conteneur droit
    QVBoxLayout*  sideLayout;    // Layout vertical pour les contrôles

    //--- Flux vidéo de la caméra ---
    CameraWindow* cameraWindow;  // Widget affichant l’image proces­sée

    //--- Indicateurs de joueur ---
    QLabel*       scoreLabel;    // Affiche le score actuel
    QLabel*       timeLabel;     // Affiche le temps écoulé

    //--- Détection de paume ---
    PalmDetector* detector;      // Détecte la main via OpenCV

    //--- Boucle de mise à jour ---
    QTimer*       timer;         // Timer Qt (~30 FPS) pour updateFrame()
};

#endif // MAINWINDOW_H
