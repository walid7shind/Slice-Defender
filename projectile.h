#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <QVector3D>
#include <QMatrix4x4>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QRandomGenerator>

class QOpenGLShaderProgram;

/*
 * Classe Projectile
 * → Gère la physique, la géométrie et le rendu OpenGL de projectiles variés.
 */
class Projectile : protected QOpenGLFunctions_3_3_Core
{
public:
    //=== Types et configuration =================================================
    enum class Shape { Apple, Cherry, IceCube, bannana };  // Formes dispo

    struct Settings {
        Shape     shape           = Shape::Apple;      // Type de modèle
        QVector3D position        = {0.f, 0.f, 0.f};   // Point de départ
        QVector3D velocity        = {0.f, 0.f, 0.f};   // Vitesse initiale
        QVector3D initialPosition = {0.f, 0.f, 0.f};   // Override position
        QVector3D targetPoint     = {0.f, 0.f, 10.f};  // Cible par défaut
        float     size            = 1.f;               // Échelle du modèle
        QString   textureImg      = "";                // Chemin de la texture
        bool      isFragment      = false;             // Indique un fragment
    };

    //=== Constructeur / Destructeur ==============================================
    explicit Projectile(const Settings& cfg);  // Init OpenGL, géom. & textures
    ~Projectile();                             // Nettoyage VAO/VBO

    //=== Fonctions statiques utilitaires ========================================
    static Shape             RandomShape();   // Forme tirée aléatoirement
    static const QVector3D   kAxes[4];        // Axes possibles pour rotation

    //=== Accesseurs et setters basiques ========================================
    QVector3D     position()    const { return m_pos; }
    QVector3D     velocity()    const { return m_vel; }
    Settings      settings()    const { return m_cfg; }
    bool          isFragment()  const { return m_isFragment; }
    bool          isActive()    const { return m_active; }
    bool          isVisible()   const { return m_visible; }
    float         size()        const { return m_size; }
    int           vertexCount() const { return m_vertexCount; }

    void setSize(float s)             { m_size = s; }
    void setActive(bool a)            { m_active = a; }
    void setVisible(bool v)           { m_visible = v; }
    void setHideInTunnel(bool hide)   { m_hideInTunnel = hide; }

    //=== Simulation / Physique =================================================
    virtual void advanceTime(float dt);  // Met à jour rotation + trajectoire
    void update(float dt);               // Applique gravité & translate
    void computeProjectilePositionAtTime(
        QVector3D initialPoint,
        QVector3D targetPoint,
        float     alphaDeg,
        float     t,
        float     g = 9.81f            // Gravité par défaut
        );

    //=== Rendu OpenGL ===========================================================
    /**
     * Dessine le projectile.
     * @param shader Programme shader OpenGL actif.
     * @param view   Matrice de vue caméra.
     * @param proj   Matrice de projection.
     */
    void render(QOpenGLShaderProgram& shader,
                const QMatrix4x4& view,
                const QMatrix4x4& proj);

    //=== Gestion des fragments =================================================
    /**
     * En cas d’impact, génère deux nouveaux fragments.
     * @return Liste de pointeurs vers nouveaux Projectile.
     */
    QVector<Projectile*> split();

    //=== Réinitialisation & utilitaires divers =================================
    void setInitialPosition(const QVector3D& p) { m_initialPosition = p; m_pos = p; }
    void setTargetPoint   (const QVector3D& t) { m_targetPoint     = t; }
    void resetTimeAndActive()                   { m_time = 0.f; m_active = true; m_visible = true; }
    void reset(const QVector3D& start, const QVector3D& target); // Reset complet
    void setShape(Shape s);                  // Change forme + reload ressources
    void loadShapeTexture();                 // Charge texture selon forme

private:
    //=== Construction & upload de la géométrie =================================
    void buildGeometry();                    // Choix de la routine adaptée
    void buildGeometryApple();               // Génère la pomme
    void buildGeometryCherry();              // Génère la cerise
    void buildGeometryBanana();              // Génère la banane
    void buildGeometryIceCube();             // Génère le cube de glace
    void uploadGeometry();                   // VBO → VAO

    //=== Attributs internes ====================================================
    Settings                    m_cfg;
    float                       m_time         = 0.f;
    float                       m_timeStep     = 0.016f;
    QVector3D                   m_initialPosition;
    QVector3D                   m_targetPoint;
    QVector3D                   m_pos;
    QVector3D                   m_vel;
    Shape                       m_shape;
    float                       m_size         = 0.6f;
    bool                        m_active       = true;
    bool                        m_isFragment   = false;
    bool                        m_visible      = true;
    QVector3D                   m_rotAxis;
    float                       m_rotAngle     = 0.f;
    float                       m_rotSpeed     = 360.f;  // °/s
    int                         m_axisIndex    = 0;
    int                         m_vertexCount  = 0;
    QOpenGLVertexArrayObject    m_vao;
    QOpenGLBuffer               m_vbo{QOpenGLBuffer::VertexBuffer};
    QScopedPointer<QOpenGLTexture> m_texture;
    bool                        m_hideInTunnel = false;  // Masque en tunnel

    // Interdiction de copie et affectation
    Projectile(const Projectile&)            = delete;
    Projectile& operator=(const Projectile&) = delete;

public slots:
    void hideProjectile();  // Slot Qt pour masquer instantané
};

#endif // PROJECTILE_H
