#include "GameScene.h"
#include "Projectile.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QtMath>
#include <cstdlib>
#include <QRandomGenerator>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QFont>


static const char* vShaderSrc = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vUV;
out vec3 vColor;

void main() {
    vNormal    = mat3(transpose(inverse(uModel))) * aNormal;
    vWorldPos  = vec3(uModel * vec4(aPos, 1.0));
    vUV        = aUV;
    vColor     = aColor;
    gl_Position = uProj * uView * vec4(vWorldPos, 1.0);
})";

static const char* fShaderSrc = R"(#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;
in vec3 vColor;

uniform vec3   uLightDir;
uniform vec3   uViewPos;
uniform int    uHasTex;
uniform sampler2D uTexture;
uniform float  uShininess;

uniform vec3   uEmissionColor;
uniform float  uEmissionPower;

out vec4 fragColor;

void main()
{
    // When textured, we ignore vColor; otherwise take the vertex color
    vec3 base = (uHasTex == 1)
              ? texture(uTexture, vUV).rgb
              : vColor;

    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.15);                    // diffuse + ambient
    float spec = pow(max(dot(N, H), 0.0), uShininess);    // specular

    vec3 emission = uEmissionColor * uEmissionPower;
    vec3 color    = base * diff + vec3(spec) + emission;

    fragColor = vec4(color, 1.0);
})";


float randomX(float min, float max) {
    return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
}
GameScene::GameScene(QWidget* parent)
    : QOpenGLWidget(parent)
{
    m_elapsed.start();

    m_frameTimer = new QTimer(this);
    connect(m_frameTimer, &QTimer::timeout, this, &GameScene::tick);

    m_startButton = new QPushButton("Start Game", this);
    m_startButton->setFixedSize(200, 60);
    connect(m_startButton, &QPushButton::clicked, this, [this]() {
        m_gameStarted = true;
        m_gameTimer.restart();
        emit elapsedTimeChanged(0.0f);
        m_startButton->hide();
        m_frameTimer->start(16);
        update();
    });

    m_restartButton = new QPushButton("Restart Game", this);
    m_restartButton->setFixedSize(200, 60);
    m_restartButton->hide();

    m_gameTimer.restart();
    emit elapsedTimeChanged(0.0f);

    connect(m_restartButton, &QPushButton::clicked, this, [this]() {
        m_score = 0;
        emit scoreChanged(0);
        m_gameOver = false;
        m_gameStarted = true;
        m_gameTimer.restart();
        emit elapsedTimeChanged(0.0f);
        m_restartButton->hide();

        for (auto* p : m_projectiles) {
            float rx = randomX(-5.f, 5.f);
            p->setShape(Projectile::RandomShape());
            p->reset({rx, 0.f, -5.f}, {0.f, 0.f, 12.f});
            p->setVisible(true);
            p->setActive(true);
        }

        update();
    });

}
void GameScene::resizeEvent(QResizeEvent* evt) {
    QOpenGLWidget::resizeEvent(evt);
    if (m_startButton) {
        int x = (width()  - m_startButton->width())  / 2;
        int y = (height() - m_startButton->height()) / 2;
        m_startButton->move(x, y);
    }
    if (m_restartButton) {
        int x = (width()  - m_restartButton->width())  / 2;
        int y = (height() - m_restartButton->height()) / 2;
        m_restartButton->move(x, y);
    }
}


void GameScene::setupCylinderGrid()
{
    struct Vertex { QVector3D pos; };
    QVector<Vertex> verts;

    const int radialSegments  = 42;
    const int heightSegments  = 15;
    const float radius        = 3.0f;
    const float zCenter       = 11.0f;
    const float height        = 10.0f;

    for (int i = 0; i <= heightSegments; ++i) {
        float y = (height * i) / heightSegments;
        for (int j = 0; j < radialSegments; ++j) {
            float theta     = (2.0f * M_PI * j) / radialSegments;
            float nextTheta = (2.0f * M_PI * (j + 1)) / radialSegments;
            verts.append({{ radius * qCos(theta),     y, radius * qSin(theta) + zCenter }});
            verts.append({{ radius * qCos(nextTheta), y, radius * qSin(nextTheta) + zCenter }});
        }
    }

    for (int j = 0; j < radialSegments; ++j) {
        float theta = (2.0f * M_PI * j) / radialSegments;
        float x     = radius * qCos(theta);
        float z     = radius * qSin(theta) + zCenter;
        verts.append({{ x, 0.0f,    z }});
        verts.append({{ x, height,  z }});
    }

    m_cylinderVertexCount = verts.size();
    m_cylinderVao.create();
    m_cylinderVbo.create();
    m_cylinderVao.bind();
    m_cylinderVbo.bind();
    m_cylinderVbo.allocate(verts.constData(), verts.size() * sizeof(Vertex));

    m_shader->bind();
    m_shader->enableAttributeArray(0);
    m_shader->setAttributeBuffer(
        0, GL_FLOAT, offsetof(Vertex, pos), 3, sizeof(Vertex)
        );
    m_cylinderVao.release();
    m_cylinderVbo.release();
    m_shader->release();
}

void GameScene::drawCylinderGrid()
{
    glLineWidth(3.0f);

    m_shader->bind();
    m_cylinderVao.bind();

    QMatrix4x4 model;
    model.setToIdentity();

    m_shader->setUniformValue("uModel", model);
    m_shader->setUniformValue("uView",  m_view);
    m_shader->setUniformValue("uProj",  m_proj);
    m_shader->setUniformValue("uHasTex",        0);
    m_shader->setUniformValue("uEmissionColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_shader->setUniformValue("uEmissionPower", 0.0f);

    glDrawArrays(GL_LINES, 0, m_cylinderVertexCount);

    m_cylinderVao.release();
    m_shader->release();
}




void GameScene::setupRoom()
{
    struct Vertex { QVector3D pos, normal; QVector2D uv; };
    QVector<Vertex> vertices;

    vertices << Vertex{{-5,5,-5}, {0,-1,0}, {0,0}}
             << Vertex{{ 5,5,-5}, {0,-1,0}, {1,0}}
             << Vertex{{ 5,5, 12}, {0,-1,0}, {1,1}}

             << Vertex{{-5,5,-5}, {0,-1,0}, {0,0}}
             << Vertex{{ 5,5, 12}, {0,-1,0}, {1,1}}
             << Vertex{{-5,5, 12}, {0,-1,0}, {0,1}};

    vertices << Vertex{{-5,0,-5},{0,1,0},{0,0}}
             << Vertex{{ 5,0, 12},{0,1,0},{1,1}}
             << Vertex{{ 5,0,-5},{0,1,0},{1,0}}

             << Vertex{{-5,0,-5},{0,1,0},{0,0}}
             << Vertex{{-5,0, 12},{0,1,0},{0,1}}
             << Vertex{{ 5,0, 12},{0,1,0},{1,1}};

    vertices << Vertex{{-5,0,-5},{0,0,1},{0,0}}
             << Vertex{{ 5,0,-5},{0,0,1},{1,0}}
             << Vertex{{ 5,5,-5},{0,0,1},{1,1}}

             << Vertex{{-5,0,-5},{0,0,1},{0,0}}
             << Vertex{{ 5,5,-5},{0,0,1},{1,1}}
             << Vertex{{-5,5,-5},{0,0,1},{0,1}};

    vertices << Vertex{{-5,0, 10},{1,0,0},{1,0}}
             << Vertex{{-5,0,-5},{1,0,0},{0,0}}
             << Vertex{{-5,5,-5},{1,0,0},{0,1}}

             << Vertex{{-5,5, 10},{1,0,0},{1,1}}
             << Vertex{{-5,0, 10},{1,0,0},{1,0}}
             << Vertex{{-5,5,-5},{1,0,0},{0,1}};

    vertices << Vertex{{5,0,-5},{-1,0,0},{0,0}}
             << Vertex{{5,0, 10},{-1,0,0},{1,0}}
             << Vertex{{5,5, 10},{-1,0,0},{1,1}}

             << Vertex{{5,0,-5},{-1,0,0},{0,0}}
             << Vertex{{5,5, 10},{-1,0,0},{1,1}}
             << Vertex{{5,5,-5},{-1,0,0},{0,1}};

    m_roomVertexCount = vertices.size();

    m_roomVao.create();
    m_roomVbo.create();
    m_roomVao.bind();
    m_roomVbo.bind();
    m_roomVbo.allocate(vertices.constData(), vertices.size() * sizeof(Vertex));

    m_shader->bind();
    m_shader->enableAttributeArray(0);
    m_shader->enableAttributeArray(1);
    m_shader->enableAttributeArray(2);
    m_shader->setAttributeBuffer(0, GL_FLOAT, offsetof(Vertex, pos),    3, sizeof(Vertex));
    m_shader->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, normal),3, sizeof(Vertex));
    m_shader->setAttributeBuffer(2, GL_FLOAT, offsetof(Vertex, uv),    2, sizeof(Vertex));
    m_roomVao.release();
    m_roomVbo.release();
    m_shader->release();
    QImage groundImg("C:/Users/khali/dev/sd lakheeer/assets/ground.jpg");
    if (groundImg.isNull()) {
        qWarning() << "Failed to load assets/ground.jpg";
    } else {
        m_groundTexture.reset(new QOpenGLTexture(groundImg.mirrored()));
    }

    QImage sideWallImg("C:/Users/khali/dev/sd lakheeer/assets/wall_jp.jpg");
    if (sideWallImg.isNull()) {
        qWarning() << "Failed to load assets/wall_jp.jpg";
    } else {
        m_wallTexture.reset(new QOpenGLTexture(sideWallImg.mirrored()));
    }

    QImage ceilingImg("C:/Users/khali/dev/sd lakheeer/assets/ceiling.jpg");
    if (ceilingImg.isNull()) {
        qWarning() << "Failed to load assets/ceiling.jpg";
    } else {
        m_ceillingTexture.reset(new QOpenGLTexture(ceilingImg.mirrored()));
    }

    QImage frontImg("C:/Users/khali/dev/sd lakheeer/assets/temple.jpg");
    if (frontImg.isNull()) {
        qWarning() << "Failed to load assets/temple.jpg";
    } else {
        m_frontTexture.reset(new QOpenGLTexture(frontImg.mirrored()));
    }


    m_groundTexture.reset(new QOpenGLTexture(groundImg.mirrored()));
    m_wallTexture.reset(new QOpenGLTexture(sideWallImg.mirrored()));
    m_ceillingTexture.reset(new QOpenGLTexture(ceilingImg.mirrored()));
    m_frontTexture.reset(new QOpenGLTexture(frontImg.mirrored()));

}
void GameScene::drawExplosionParticles(const Explosion& ex) {

    static QVector<QVector3D> offsets;
    if (offsets.isEmpty()) {
        constexpr int   PARTICLE_COUNT = 500;
        constexpr float SPREAD         = 0.8f;

        offsets.reserve(PARTICLE_COUNT);
        for (int i = 0; i < PARTICLE_COUNT; ++i) {
            float rx = (QRandomGenerator::global()->bounded(-100,100) / 100.0f) * SPREAD;
            float ry = (QRandomGenerator::global()->bounded(-100,100) / 100.0f) * SPREAD;
            float rz = (QRandomGenerator::global()->bounded(-100,100) / 100.0f) * SPREAD;
            offsets.append({ rx, ry, rz });
        }
    }

    QVector<QVector3D> points;
    points.reserve(offsets.size());
    for (auto &o : offsets)
        points.append(ex.position + o);

    m_particleVbo.bind();
    m_particleVbo.allocate(points.constData(), points.size() * sizeof(QVector3D));

    m_shader->bind();
    m_shader->setUniformValue("uHasTex",         0);
    m_shader->setUniformValue("uEmissionColor",  QVector3D(1.0f, 0.8f, 0.0f));
    m_shader->setUniformValue("uEmissionPower",  1.5f);

    m_shader->enableAttributeArray(0);
    m_shader->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(QVector3D));

    glEnable(GL_PROGRAM_POINT_SIZE);
    glPointSize(8.0f);
    glDrawArrays(GL_POINTS, 0, points.size());
    glDisable(GL_PROGRAM_POINT_SIZE);

    m_shader->disableAttributeArray(0);
    m_shader->release();
    m_particleVbo.release();
}
void GameScene::updateSwordPosition(const QVector3D& pos)
{
    if (m_sword) {
        m_sword->setPosition(pos);
        update();
    }
}

void GameScene::tick()
{
    if (m_gameStarted && !m_gameOver) {
        float t = m_gameTimer.elapsed() * 1e-3f;
        emit elapsedTimeChanged(t);
    }

    if (!m_gameStarted || m_gameOver)
        return;

    m_explosions.clear();

    float dt = m_elapsed.restart() * 1e-3f;
    QVector3D swordPos = m_sword->position();

    for (auto* p : m_projectiles) {
        if (!p->isActive())
            continue;

        p->advanceTime(dt);
        float z = p->position().z();

        if (z >= 11.f) {
            m_gameOver = true;
            m_restartButton->show();
            return;
        }

        float dist = (p->position() - swordPos).length();
        if (dist < p->size() - 0.2f) {
            ++m_score;
            emit scoreChanged(m_score);

            p->setActive(false);
            p->setVisible(false);

            Explosion ex;
            ex.position = p->position();
            m_explosions.append(ex);

            if (m_sfxPlayer) {
                m_sfxPlayer->stop();
                m_sfxPlayer->play();
            }
        }

        else if (m_hideInTunnel && z >= 7.5f && z <= 12.f) {
            p->setVisible(false);
        }
    }

    for (auto* p : m_projectiles) {
        if (!p->isActive()) {
            float rx = randomX(-5.f, 5.f);
            p->setShape(Projectile::RandomShape());
            p->reset({rx, 0.f, -5.f}, {0.f, 0.f, 12.f});
            p->setActive(true);
            p->setVisible(true);
        }
    }

    update();
}


void GameScene::update()
{
    float dt = m_elapsed.restart() * 1e-3f;

    for (auto* p : m_projectiles)
    {
        p->advanceTime(dt);
    }

    QOpenGLWidget::update();
}


GameScene::~GameScene()
{
    makeCurrent();
    m_groundTexture.reset();
    m_ceillingTexture.reset();
    m_frontTexture.reset();
    qDeleteAll(m_projectiles);
    delete m_shader;
    doneCurrent();
}







void GameScene::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);

    initShader();
    uploadSceneLight();

    m_particleVbo.create();
    m_particleVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    setupRoom();
    m_sword = new Sword(this);
    m_sword->initialize();
    setupCylinderGrid();

    m_view.lookAt({0.f, 2.5f, 15.f},
                  {0.f, 1.f, 0.f},
                  {0.f, 2.f, 0.f});



    m_audioOutput = new QAudioOutput(this);
    m_musicPlayer = new QMediaPlayer(this);
    m_musicPlayer->setAudioOutput(m_audioOutput);
    m_musicPlayer->setSource(QUrl::fromLocalFile("C:/Users/khali/dev/sd lakheeer/assets/music.mp3"));
    m_audioOutput->setVolume(0.3);
    m_musicPlayer->play();

    m_sfxOutput = new QAudioOutput(this);
    m_sfxPlayer = new QMediaPlayer(this);
    m_sfxPlayer->setAudioOutput(m_sfxOutput);
    m_sfxPlayer->setSource(QUrl::fromLocalFile("C:/Users/khali/dev/sd lakheeer/assets/cut.mp3"));
    m_sfxOutput->setVolume(1.0);



    float rx = randomX(-5.f, 5.f);
    Projectile::Settings cfg;
    cfg.shape           = Projectile::RandomShape();
    cfg.initialPosition = { rx, 0.f, -5.f };
    cfg.targetPoint     = { 0.f, 0.f, 12.f };
    cfg.size            = 1.f;
    cfg.velocity        = {};


    m_projectiles.append( new Projectile(cfg));


}



void GameScene::resizeGL(int w, int h)
{
    m_proj.setToIdentity();
    m_proj.perspective(45.f, float(w)/float(h>0?h:1), 0.1f, 100.f);
}
void GameScene::drawRoom()
{
    m_shader->bind();
    m_roomVao.bind();


    m_shader->setUniformValue("uEmissionColor", QVector3D(0.f, 0.f, 0.f));
    m_shader->setUniformValue("uEmissionPower", 0.0f);


    QMatrix4x4 I;  I.setToIdentity();
    m_shader->setUniformValue("uModel", I);
    m_shader->setUniformValue("uView",  m_view);
    m_shader->setUniformValue("uProj",  m_proj);
    m_shader->setUniformValue("uHasTex", 1);
    m_shader->setUniformValue("uTexture", 0);

    if (m_ceillingTexture) m_ceillingTexture->bind(0);
    glDrawArrays(GL_TRIANGLES, 0, 6);


    if (m_groundTexture) m_groundTexture->bind(0);
    glDrawArrays(GL_TRIANGLES, 6, 6);


    if (m_frontTexture) m_frontTexture->bind(0);
    glDrawArrays(GL_TRIANGLES, 12, 6);


    if (m_wallTexture) m_wallTexture->bind(0);
    glDrawArrays(GL_TRIANGLES, 18, 6);


    if (m_wallTexture) m_wallTexture->bind(0);
    glDrawArrays(GL_TRIANGLES, 24, 6);


    m_roomVao.release();
    m_shader->release();
}

void GameScene::paintEvent(QPaintEvent* event)
{

    QOpenGLWidget::paintEvent(event);


    if (m_gameOver) {
        QPainter p(this);
        p.setPen(Qt::blue);
        p.setFont(QFont("Arial", 100, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "GAME OVER");
    }
}




void GameScene::paintGL()
{
    if (m_gameOver) {
        glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        QOpenGLPaintDevice pd(width(), height());
        QPainter p(&pd);

        p.setPen(Qt::red);
        p.setFont(QFont("Arial", 48, QFont::Bold));
        p.drawText(rect().adjusted(0, 50, 0, 0),
                   Qt::AlignTop | Qt::AlignHCenter, "GAME OVER");

        QString scoreText = QString("Score: %1").arg(m_score);
        QFont font("Arial", 24, QFont::Bold);
        p.setFont(font);
        QFontMetrics fm(font);
        int w = fm.horizontalAdvance(scoreText);
        int h = fm.height();
        QRect textRect((width() - w) / 2, 120, w, h);
        p.fillRect(textRect.adjusted(-10, -5, 10, 5),
                   QColor(0, 0, 0, 160));
        p.setPen(Qt::white);
        p.drawText(textRect, Qt::AlignCenter, scoreText);
        return;
    }


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    static float camTime = 0.f;
    camTime += 0.016f;
    float a = 0.f;

    QVector3D eye(a, 2.f, 13.f);
    QVector3D center(0.f, 1.f, 0.f);
    m_view.setToIdentity();
    m_view.lookAt(eye, center, {0.f, 1.f, 0.f});

    m_shader->bind();
    m_shader->setUniformValue("uViewPos", eye);
    m_shader->release();

    drawRoom();
    drawCylinderGrid();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);

    m_shader->bind();
    for (const Explosion& boom : m_explosions)
        drawExplosionParticles(boom);

    m_sword->render(*m_shader, m_view, m_proj);
    m_shader->release();

    for (auto* p : m_projectiles) {
        if (!p->isVisible()) continue;
        p->render(*m_shader, m_view, m_proj);
    }
}

bool GameScene::initShader()
{
    m_shader = new QOpenGLShaderProgram(this);
    if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex,   vShaderSrc)) return false;
    if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fShaderSrc)) return false;
    return m_shader->link();
}

void GameScene::uploadSceneLight()
{
    m_shader->bind();
    m_shader->setUniformValue("uLightDir", QVector3D(1,1,1).normalized());
    m_shader->setUniformValue("uShininess", 64.0f);
    m_shader->release();

}
