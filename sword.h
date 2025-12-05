#pragma once
#include <QObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QVector2D>
#include <QOpenGLShaderProgram>
#include <vector>

/*
 * Classe Sword :
 * Gère la création, la géométrie et le rendu OpenGL d’un sabre.
 */
class Sword : public QObject, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    //--- Structure de vertex utilisée pour le sabre ---
    struct Vertex {
        QVector3D pos;     // Position dans l’espace
        QVector3D normal;  // Normale pour l’éclairage
        QVector2D uv;      // Coordonnées de texture
        QVector3D color;   // Couleur si pas de texture
    };

    explicit Sword(QObject* parent = nullptr);  // Init QObject et contexte OpenGL
    ~Sword() override;                          // Nettoyage des ressources

    //--- Initialisation et rendu ---
    void initialize();  // Configure VAO, VBO et construit la géométrie
    void render(QOpenGLShaderProgram& shader,
                const QMatrix4x4& view,
                const QMatrix4x4& proj);  // Dessine le sabre selon les matrices

    //--- Transformation dans le monde ---
    // Définit la position globale du sabre
    void setPosition(const QVector3D& p) { m_position = p; }
    QVector3D position() const { return m_position; }

private:
    //--- Construction de la géométrie ---
    void buildGeometry();  // Remplit m_verts et m_vertexCount

    //--- Ressources OpenGL ---
    QOpenGLVertexArrayObject m_vao;           // Handle VAO
    QOpenGLBuffer            m_vbo{QOpenGLBuffer::VertexBuffer}; // Buffer de vertex
    int                      m_vertexCount = 0;  // Nombre de sommets
    std::vector<Vertex>      m_verts;            // Données de vertex

    //--- Position du sabre dans la scène ---
    QVector3D m_position{0.f,1.f,6.5f};
};
