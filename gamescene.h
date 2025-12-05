#pragma once

// GameScene.h
// Gère la boucle de jeu, l’init OpenGL, le rendu et la logique de la scène (meh, c’est le cœur du game)

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QMatrix4x4>
#include <QElapsedTimer>
#include <QTimer>
#include <QPushButton>
#include "Projectile.h"
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPaintEvent>
#include <QResizeEvent>
#include "Sword.h"

// Décrit un effet d’explosion à un point donné
struct Explosion {
    QVector3D position;
};

class QOpenGLShaderProgram;

class GameScene : public QOpenGLWidget,
                  protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

signals:
    // Signaux Qt pour notifier les changements
    void scoreChanged(int newScore);        // Quand le score évolue
    void elapsedTimeChanged(float seconds); // Quand le chrono update

public:
    //=== Constructeur / Destructeur ===
    explicit GameScene(QWidget* parent = nullptr); // Prépare le widget
    ~GameScene() override;                         // Clean up GL & objets

protected:
    //=== Overrides Qt / OpenGL ===
    void initializeGL() override;                  // Init contexte GL + shaders + assets
    void resizeGL(int w, int h) override;          // Ajuste la projection si la fenêtre change
    void paintGL() override;                       // Rendu frame par frame
    void paintEvent(QPaintEvent *event) override;  // Dessin Qt (GAME OVER overlay)
    void resizeEvent(QResizeEvent* event) override;// Gère le repositionnement UI

    //=== Scène / Rendu complémentaires ===
    void setupRoom();          // Génère sol, murs, plafond
    void drawRoom();           // Dessine la room texturée
    void setupCylinderGrid();  // Génère le grille cylindre (lines)
    void drawCylinderGrid();   // Affiche la grille

private:
    //=== État du jeu ===
    bool        m_gameStarted = false;
    bool        m_gameOver    = false;
    int         m_score       = 0;
    bool        m_hideInTunnel= false;  // si on doit planquer les projectiles dans le tunnel

    QPushButton* m_startButton   = nullptr; // Bouton “Start Game”
    QPushButton* m_restartButton = nullptr; // Bouton “Restart Game”
    QPushButton* m_fireBtn       = nullptr; // (potentiellement unused)

    QElapsedTimer m_gameTimer;   // Chrono pour mesurer temps de jeu
    QElapsedTimer m_elapsed;     // Delta time entre frames
    QTimer*       m_frameTimer;  // Timer Qt pour boucle à ~60fps

    //=== Son ===
    QMediaPlayer* m_musicPlayer = nullptr; // Musique de fond
    QAudioOutput* m_audioOutput = nullptr;
    QMediaPlayer* m_sfxPlayer   = nullptr; // Effets sonores
    QAudioOutput* m_sfxOutput   = nullptr;

    //=== Projetiles & explosions ===
    QVector<Projectile*> m_projectiles; // Liste des projectiles actifs
    QVector<Explosion>   m_explosions;  // Positions des explosions

    //=== Ressources OpenGL générales ===
    QOpenGLShaderProgram*      m_shader   = nullptr; // Shader unique
    QMatrix4x4                 m_proj;               // Matrice de projection
    QMatrix4x4                 m_view;               // Matrice de vue (caméra)

    //=== Room buffers ===
    QOpenGLVertexArrayObject   m_roomVao;
    QOpenGLBuffer              m_roomVbo{QOpenGLBuffer::VertexBuffer};
    int                        m_roomVertexCount = 0;
    QScopedPointer<QOpenGLTexture> m_groundTexture;
    QScopedPointer<QOpenGLTexture> m_wallTexture;
    QScopedPointer<QOpenGLTexture> m_ceillingTexture;
    QScopedPointer<QOpenGLTexture> m_frontTexture;

    //=== Cylinder grid buffers ===
    QOpenGLVertexArrayObject   m_cylinderVao;
    QOpenGLBuffer              m_cylinderVbo{QOpenGLBuffer::VertexBuffer};
    int                        m_cylinderVertexCount = 0;

    //=== Particules explosion ===
    QOpenGLBuffer              m_particleVbo{QOpenGLBuffer::VertexBuffer};

    //=== Éclairage ===
    QVector3D                  m_lightDir = { 1.f, 1.f, 1.f }; // Direction light

    //=== Sabre du joueur ===
    Sword*                     m_sword = nullptr; // Objet sabre

    // Gravité utilisée pour les trajectoires (constante)
    const float g = 9.81f;

    //=== Fonctions utilitaires privées ===
    void tick();                          // Update loop: game logic + collisions
    bool initShader();                    // Compile/link shaders
    void uploadSceneLight();              // Envoie params light au shader
    void update();                        // Force update OpenGLWidget
    void drawExplosionParticles(const Explosion& ex); // Render points explosion

public slots:
    // Slot pour bouger le sabre depuis UI externe
    void updateSwordPosition(const QVector3D& pos);
};
