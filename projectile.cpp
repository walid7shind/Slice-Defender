#include "Projectile.h"
#include <QOpenGLShaderProgram>
#include <cmath>
#include <QDebug>
#include <QVector3D>
struct Vertex { QVector3D pos; QVector3D normal; QVector2D uv; QVector3D color; };


#include <QFile>
#include <QOpenGLTexture>
#include <QImage>
#include <QDebug>



Projectile::Projectile(const Settings& cfg)
    : m_cfg(cfg)
    , m_shape(cfg.shape)
    , m_vel(cfg.velocity)
    , m_size(cfg.size)
    , m_initialPosition(cfg.initialPosition)
    , m_targetPoint(cfg.targetPoint)
    , m_isFragment(cfg.isFragment)
    , m_active(true)
    , m_visible(true)
    , m_time(0.f)
{

    initializeOpenGLFunctions();


    loadShapeTexture();


    buildGeometry();
    uploadGeometry();


    m_pos = m_initialPosition;
}


Projectile::~Projectile()
{
    m_vao.destroy();
    m_vbo.destroy();
}

const QVector3D Projectile::kAxes[4] = {
    QVector3D(1, 0, 0),
    QVector3D(0, 1, 0),
    QVector3D(0, 0, 1),
    QVector3D(1, 1, 0).normalized()
};
static QString textureForShape(Projectile::Shape s) {
    switch (s) {
    case Projectile::Shape::Apple:   return "C:/Users/khali/dev/sd lakheeer/assets/apple.jpg";
    case Projectile::Shape::bannana: return "C:/Users/khali/dev/sd lakheeer/assets/banana.jpg";
    case Projectile::Shape::Cherry:  return "C:/Users/khali/dev/sd lakheeer/assets/cherry.jpg";
    case Projectile::Shape::IceCube: return "C:/Users/khali/dev/sd lakheeer/assets/ice.jpg";
    }
    return "";
}
void Projectile::loadShapeTexture() {
    m_texture.reset();
    QString path = textureForShape(m_shape);
    if (QFile::exists(path)) {
        QImage img(path);
        img = img.mirrored();
        m_texture.reset(new QOpenGLTexture(img));
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::Repeat);
    } else {
        qWarning() << "Missing projectile texture:" << path;
    }
}
void Projectile::setShape(Shape s)
{
    m_shape = s;

    loadShapeTexture();
    buildGeometry();
    uploadGeometry();
}


static float randomX(float minX, float maxX)
{
    double zeroToOne = QRandomGenerator::global()->generateDouble();
    return minX + static_cast<float>(zeroToOne * (maxX - minX));
}


void Projectile::update(float dt)
{
    if (!m_active) return;

    const QVector3D gravity(0.f, -9.81f, 0.f);
    m_vel += gravity * dt;
    m_pos += m_vel * dt;

    if (m_pos.z() < -5.f) m_active = false;
}

void Projectile::computeProjectilePositionAtTime(
    QVector3D initialPoint,
    QVector3D targetPoint,
    float alphaDeg,
    float t,
    float g )
{
    float alpha = qDegreesToRadians(alphaDeg);

    float dx = targetPoint.x() - initialPoint.x();
    float dz = targetPoint.z() - initialPoint.z();
    float dy = targetPoint.y() - initialPoint.y();

    float d = std::sqrt(dx * dx + dz * dz);

    float V0 = (d / std::cos(alpha)) * std::sqrt(g / (2 * (d * std::tan(alpha) - dy)));

    QVector3D dirXZ(dx, 0, dz);
    dirXZ.normalize();

    QVector3D velocity = V0 * (dirXZ * std::cos(alpha) + QVector3D(0, std::sin(alpha), 0));

    QVector3D acceleration(0, -g, 0);

    QVector3D position = initialPoint + velocity * t + 0.5f * acceleration * t * t;

    m_pos= position;
}
static int randomInt(int min, int max)
{
    return QRandomGenerator::global()->bounded(min, max + 1);
}
Projectile::Shape  Projectile::RandomShape()
{
    int index = randomInt(0, 3);

    Projectile::Shape tab[] = {
        Projectile::Shape :: IceCube,
        Projectile::Shape :: bannana,
        Projectile::Shape :: Apple,
        Projectile::Shape :: Cherry
    };
    return tab[index];
}

void Projectile::reset(const QVector3D& start, const QVector3D& target)
{
    m_initialPosition = start;
    m_targetPoint     = target;
    m_pos             = start;
    m_time            = 0.f;
    m_active          = true;
    m_visible         = true;

    m_axisIndex = (m_axisIndex + 1) % 4;
    m_rotAxis   = kAxes[m_axisIndex];

    buildGeometry();
    uploadGeometry();
    loadShapeTexture();
}

void Projectile::advanceTime(float dt)
{
    if (!m_active) return;
    m_time += dt;
    m_rotAngle += m_rotSpeed * m_timeStep;
    buildGeometry();
    uploadGeometry();
    m_rotAngle += m_rotSpeed * dt;
    if (m_rotAngle >= 360.f) m_rotAngle -= 360.f;
    computeProjectilePositionAtTime(
        m_initialPosition,
        m_targetPoint,
        40.f,
        m_time,
        9.8f
        );

    if (m_pos.z() >= m_targetPoint.z() ) {
        m_active = false;

    }
}




void Projectile::render(QOpenGLShaderProgram& shader,
                        const QMatrix4x4& view,
                        const QMatrix4x4& proj)
{
    if (!m_active || !m_visible)
        return;

    m_rotAngle += m_rotSpeed * m_timeStep;
    if (m_rotAngle >= 360.f)
        m_rotAngle -= 360.f;

    shader.bind();
    QMatrix4x4 model;
    model.translate(m_pos);
    model.rotate(m_rotAngle, m_rotAxis);
    model.scale(m_size);
    shader.setUniformValue("uModel", model);
    shader.setUniformValue("uView",  view);
    shader.setUniformValue("uProj",  proj);

    if (m_texture) {
        qDebug()<<"texture shi";
        shader.setUniformValue("uHasTex", 1);
        shader.setUniformValue("uTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        m_texture->bind();
    } else {
        shader.setUniformValue("uHasTex", 0);
    }

    m_vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    m_vao.release();

    if (m_texture)
        m_texture->release();
    shader.release();
}



QVector<Projectile*> Projectile::split()
{
    QVector<Projectile*> fragments;

    if (!m_active) return fragments;
    m_active = false;

    Settings fragSet;
    fragSet.shape  = m_shape;
    fragSet.size   = m_size * 0.5f;
    fragSet.textureImg = "";

    fragSet.position = m_pos + QVector3D(+0.1f, 0.f, 0.f);
    fragSet.velocity = m_vel + QVector3D(+1.f, +2.f, 0.f);
    fragments.append(new Projectile(fragSet));

    fragSet.position = m_pos + QVector3D(-0.1f, 0.f, 0.f);
    fragSet.velocity = m_vel + QVector3D(-1.f, +2.f, 0.f);
    fragments.append(new Projectile(fragSet));

    return fragments;
}

void Projectile::buildGeometryApple()
{
    struct Vertex { QVector3D pos; QVector3D normal; QVector2D uv; QVector3D color; };

    const QVector3D bodyColor(1.0f, 0.1f, 0.1f);
    const QVector3D leafColor(0.2f, 0.8f, 0.2f);

    std::vector<Vertex> verts;
    verts.reserve(40 * 40 * 6 + 12);


    auto appleRadius = [](float theta) {

        return 0.5f * (1.0f - 0.2f * std::cos(theta));
    };

    const int stacks = 40;
    const int slices = 40;

    for (int i = 0; i < stacks; ++i) {
        float v0 = float(i) / stacks;
        float v1 = float(i + 1) / stacks;
        float theta0 = v0 * M_PI;
        float theta1 = v1 * M_PI;

        float r0 = appleRadius(theta0);
        float r1 = appleRadius(theta1);

        for (int j = 0; j < slices; ++j) {
            float u0 = float(j) / slices;
            float u1 = float(j + 1) / slices;
            float phi0 = u0 * 2.0f * M_PI;
            float phi1 = u1 * 2.0f * M_PI;

            QVector3D p00{ r0 * std::sin(theta0) * std::cos(phi0),
                          r0 * std::cos(theta0),
                          r0 * std::sin(theta0) * std::sin(phi0) };
            QVector3D p10{ r1 * std::sin(theta1) * std::cos(phi0),
                          r1 * std::cos(theta1),
                          r1 * std::sin(theta1) * std::sin(phi0) };
            QVector3D p11{ r1 * std::sin(theta1) * std::cos(phi1),
                          r1 * std::cos(theta1),
                          r1 * std::sin(theta1) * std::sin(phi1) };
            QVector3D p01{ r0 * std::sin(theta0) * std::cos(phi1),
                          r0 * std::cos(theta0),
                          r0 * std::sin(theta0) * std::sin(phi1) };

            QVector3D n00 = p00.normalized();
            QVector3D n10 = p10.normalized();
            QVector3D n11 = p11.normalized();
            QVector3D n01 = p01.normalized();

            auto computeUV = [&](const QVector3D& P) {
                float u = (std::atan2(P.z(), P.x()) + M_PI) / (2.0f * M_PI);
                float v = (std::asin(P.y() / 0.5f) + M_PI * 0.5f) / M_PI;
                return QVector2D(u, v);
            };

            QVector2D uv00 = computeUV(p00);
            QVector2D uv10 = computeUV(p10);
            QVector2D uv11 = computeUV(p11);
            QVector2D uv01 = computeUV(p01);

            verts.push_back({ p00, n00, uv00, bodyColor });
            verts.push_back({ p10, n10, uv10, bodyColor });
            verts.push_back({ p11, n11, uv11, bodyColor });

            verts.push_back({ p00, n00, uv00, bodyColor });
            verts.push_back({ p11, n11, uv11, bodyColor });
            verts.push_back({ p01, n01, uv01, bodyColor });
        }
    }

    QVector3D leafCenter{ 0.0f, 0.55f, 0.0f };
    float leafW = 0.3f;
    float leafL = 0.5f;
    QVector3D leafNormal{ 0.0f, 1.0f, 0.0f };
    QVector2D leafUv0{ 0.5f, 0.5f };
    QVector2D leafUv1{ 1.0f, 0.0f };
    QVector2D leafUv2{ 0.0f, 0.0f };
    QVector2D leafUv3{ 0.5f, 1.0f };

    {
        QVector3D A = leafCenter + QVector3D( leafW, 0.0f,  leafL );
        QVector3D B = leafCenter + QVector3D(-leafW, 0.0f,  leafL );
        QVector3D C = leafCenter + QVector3D( 0.0f,    0.0f, -leafL );
        verts.push_back({ leafCenter, leafNormal, leafUv0, leafColor });
        verts.push_back({ A,          leafNormal, leafUv1, leafColor });
        verts.push_back({ B,          leafNormal, leafUv2, leafColor });

        verts.push_back({ leafCenter, leafNormal, leafUv0, leafColor });
        verts.push_back({ B,          leafNormal, leafUv2, leafColor });
        verts.push_back({ C,          leafNormal, leafUv3, leafColor });
    }

    {
        float ang = M_PI / 4.0f;
        QVector3D dirA = QVector3D(std::cos(ang)*leafW, 0.0f, std::sin(ang)*leafW);
        QVector3D dirB = QVector3D(std::cos(ang)*-leafW, 0.0f, std::sin(ang)*-leafW);
        QVector3D A2   = leafCenter + dirA + QVector3D(0.0f, 0.0f, leafL);
        QVector3D B2   = leafCenter + dirB + QVector3D(0.0f, 0.0f, leafL);
        QVector3D C2   = leafCenter + QVector3D(0.0f, 0.0f, -leafL);
        verts.push_back({ leafCenter, leafNormal, leafUv0, leafColor });
        verts.push_back({ A2,         leafNormal, leafUv1, leafColor });
        verts.push_back({ B2,         leafNormal, leafUv2, leafColor });

        verts.push_back({ leafCenter, leafNormal, leafUv0, leafColor });
        verts.push_back({ B2,         leafNormal, leafUv2, leafColor });
        verts.push_back({ C2,         leafNormal, leafUv3, leafColor });
    }

    m_vertexCount = static_cast<int>(verts.size());
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(verts.data(), m_vertexCount * sizeof(Vertex));
    m_vbo.release();
}



void Projectile::buildGeometryCherry()
{
    struct Vertex { QVector3D pos; QVector3D normal; QVector2D uv; QVector3D color; };

    const QVector3D cherryRed(1.0f, 0.0f, 0.0f);
    const QVector3D stemGreen(0.0f, 0.8f, 0.0f);
    const QVector3D leafColor(0.2f, 0.6f, 0.2f);

    std::vector<Vertex> verts;
    verts.reserve(2 * 16 * 32 * 6 + 20 + 6);

    const int latSteps = 16, lonSteps = 32;
    const float radius = 0.2f;
    QVector3D centers[2] = {{-0.25f, 0.0f, 0.0f}, {+0.25f, 0.0f, 0.0f}};
    for (int c = 0; c < 2; ++c) {
        const QVector3D& C = centers[c];
        for (int i = 0; i < latSteps; ++i) {
            float v0 = float(i) / latSteps;
            float v1 = float(i + 1) / latSteps;
            float lat0 = M_PI * (v0 - 0.5f);
            float lat1 = M_PI * (v1 - 0.5f);
            float z0 = std::sin(lat0), zr0 = std::cos(lat0);
            float z1 = std::sin(lat1), zr1 = std::cos(lat1);
            for (int j = 0; j < lonSteps; ++j) {
                float u0 = float(j) / lonSteps;
                float u1 = float(j + 1) / lonSteps;
                float ang0 = 2.0f * M_PI * u0;
                float ang1 = 2.0f * M_PI * u1;
                float x0 = std::cos(ang0), y0 = std::sin(ang0);
                float x1 = std::cos(ang1), y1 = std::sin(ang1);

                QVector3D p00 = C + QVector3D(x0*zr0, z0, y0*zr0) * radius;
                QVector3D p10 = C + QVector3D(x0*zr1, z1, y0*zr1) * radius;
                QVector3D p11 = C + QVector3D(x1*zr1, z1, y1*zr1) * radius;
                QVector3D p01 = C + QVector3D(x1*zr0, z0, y1*zr0) * radius;

                QVector3D n00 = (p00 - C).normalized();
                QVector3D n10 = (p10 - C).normalized();
                QVector3D n11 = (p11 - C).normalized();
                QVector3D n01 = (p01 - C).normalized();

                QVector2D uv00{u0, v0}, uv10{u0, v1}, uv11{u1, v1}, uv01{u1, v0};

                verts.push_back({p00, n00, uv00, cherryRed});
                verts.push_back({p10, n10, uv10, cherryRed});
                verts.push_back({p11, n11, uv11, cherryRed});
                verts.push_back({p00, n00, uv00, cherryRed});
                verts.push_back({p11, n11, uv11, cherryRed});
                verts.push_back({p01, n01, uv01, cherryRed});
            }
        }
    }

    QVector3D lb = centers[0] + QVector3D(0, radius, 0);
    QVector3D rb = centers[1] + QVector3D(0, radius, 0);
    QVector3D mt = (lb + rb) * 0.5f + QVector3D(0, 0.2f, 0);
    float stemW = 0.02f;
    QVector3D dir = (rb - lb).normalized();
    QVector3D widthVec = QVector3D::crossProduct(dir, {0,1,0}).normalized() * stemW;
    QVector2D suv0{0,0}, suv1{1,0}, suv2{1,1}, suv3{0,1};

    QVector3D lb0 = lb - widthVec, lb1 = lb + widthVec;
    QVector3D mt0 = mt - widthVec, mt1 = mt + widthVec;
    verts.push_back({lb0, dir, suv0, stemGreen});
    verts.push_back({mt0, dir, suv3, stemGreen});
    verts.push_back({mt1, dir, suv2, stemGreen});
    verts.push_back({lb0, dir, suv0, stemGreen});
    verts.push_back({mt1, dir, suv2, stemGreen});
    verts.push_back({lb1, dir, suv1, stemGreen});

    QVector3D rb0 = rb - widthVec, rb1 = rb + widthVec;
    verts.push_back({rb1, dir, suv1, stemGreen});
    verts.push_back({mt1, dir, suv2, stemGreen});
    verts.push_back({mt0, dir, suv3, stemGreen});
    verts.push_back({rb1, dir, suv1, stemGreen});
    verts.push_back({mt0, dir, suv3, stemGreen});
    verts.push_back({rb0, dir, suv0, stemGreen});

    QVector3D leafCenter = mt + dir * 0.1f;
    QVector3D leafUp = QVector3D(0,1,0);
    QVector3D leafDir = dir;
    QVector3D L0 = leafCenter;
    QVector3D L1 = leafCenter + leafDir * 0.15f + leafUp * 0.05f;
    QVector3D L2 = leafCenter - leafDir * 0.15f + leafUp * 0.05f;
    QVector3D nL = leafUp;
    QVector2D luv0{0.5f,0.5f}, luv1{1,0}, luv2{0,0};
    verts.push_back({L0, nL, luv0, leafColor});
    verts.push_back({L1, nL, luv1, leafColor});
    verts.push_back({L2, nL, luv2, leafColor});

    m_vertexCount = int(verts.size());
    m_vbo.create(); m_vbo.bind();
    m_vbo.allocate(verts.data(), m_vertexCount * sizeof(Vertex));
    m_vbo.release();
}



void Projectile::buildGeometryBanana()
{
    struct Vertex {
        QVector3D pos;
        QVector3D normal;
        QVector2D uv;
        QVector3D color;
    };

    const QVector3D bananaYellow(1.0f, 0.9f, 0.1f);
    constexpr int   curveSteps = 20;
    constexpr int   sliceSteps = 16;
    const   float   R          = 1.0f;
    const   float   baseR      = 0.12f;
    const   float   arc        = 2.0f * M_PI / 3.0f;

    std::vector<Vertex> verts;
    verts.reserve(curveSteps * sliceSteps * 6 + sliceSteps * 6);

    for (int i = 0; i < curveSteps; ++i) {
        float t0     = float(i)     / curveSteps;
        float t1     = float(i + 1) / curveSteps;
        float theta0 = t0 * arc;
        float theta1 = t1 * arc;

        float s0 = 0.5f + 0.5f * std::sin(theta0);
        float s1 = 0.5f + 0.5f * std::sin(theta1);
        float r0 = baseR * s0;
        float r1 = baseR * s1;

        QVector3D C0{ R * std::sin(theta0),
                     R - R * std::cos(theta0),
                     0.f };
        QVector3D C1{ R * std::sin(theta1),
                     R - R * std::cos(theta1),
                     0.f };

        QVector3D N0{ std::sin(theta0), -std::cos(theta0), 0.f };
        QVector3D N1{ std::sin(theta1), -std::cos(theta1), 0.f };
        constexpr QVector3D B{ 0.f, 0.f, -1.f };

        for (int j = 0; j < sliceSteps; ++j) {
            float u0   = float(j)     / sliceSteps;
            float u1   = float(j + 1) / sliceSteps;
            float phi0 = u0 * 2.f * M_PI;
            float phi1 = u1 * 2.f * M_PI;

            QVector3D R0a = (N0 * std::cos(phi0) + B * std::sin(phi0)) * r0;
            QVector3D R0b = (N0 * std::cos(phi1) + B * std::sin(phi1)) * r0;
            QVector3D R1a = (N1 * std::cos(phi0) + B * std::sin(phi0)) * r1;
            QVector3D R1b = (N1 * std::cos(phi1) + B * std::sin(phi1)) * r1;

            QVector3D p00 = C0 + R0a;
            QVector3D p01 = C0 + R0b;
            QVector3D p10 = C1 + R1a;
            QVector3D p11 = C1 + R1b;

            QVector3D n00 = R0a.normalized();
            QVector3D n01 = R0b.normalized();
            QVector3D n10 = R1a.normalized();
            QVector3D n11 = R1b.normalized();

            QVector2D uv00{ t0, u0 }, uv01{ t0, u1 };
            QVector2D uv10{ t1, u0 }, uv11{ t1, u1 };

            verts.push_back({ p00, n00, uv00, bananaYellow });
            verts.push_back({ p10, n10, uv10, bananaYellow });
            verts.push_back({ p11, n11, uv11, bananaYellow });

            verts.push_back({ p00, n00, uv00, bananaYellow });
            verts.push_back({ p11, n11, uv11, bananaYellow });
            verts.push_back({ p01, n01, uv01, bananaYellow });
        }
    }

    for (int end = 0; end < 2; ++end) {
        float thetaE = (end == 0 ? 0.f : arc);
        QVector3D C{ R * std::sin(thetaE),
                    R - R * std::cos(thetaE),
                    0.f };
        QVector3D N = (end == 0 ? QVector3D(-1,0,0)
                                : QVector3D(+1,0,0));
        float capR = baseR * (0.5f + 0.5f * std::sin(thetaE));

        for (int j = 0; j < sliceSteps; ++j) {
            float u0   = float(j)     / sliceSteps;
            float u1   = float(j + 1) / sliceSteps;
            float phi0 = u0 * 2.f * M_PI;
            float phi1 = u1 * 2.f * M_PI;

            QVector3D p0 = C + QVector3D(std::cos(phi0), 0, std::sin(phi0)) * capR;
            QVector3D p1 = C + QVector3D(std::cos(phi1), 0, std::sin(phi1)) * capR;
            QVector2D uv0{ 0.5f + 0.5f*std::cos(phi0), 0.5f + 0.5f*std::sin(phi0) };
            QVector2D uv1{ 0.5f + 0.5f*std::cos(phi1), 0.5f + 0.5f*std::sin(phi1) };
            QVector2D uvC{ 0.5f, 0.5f };

            if (end == 0) {
                verts.push_back({ C, N, uvC, bananaYellow });
                verts.push_back({ p0, N, uv0, bananaYellow });
                verts.push_back({ p1, N, uv1, bananaYellow });
            } else {
                verts.push_back({ C, N, uvC, bananaYellow });
                verts.push_back({ p1, N, uv1, bananaYellow });
                verts.push_back({ p0, N, uv0, bananaYellow });
            }
        }
    }

    m_vertexCount = static_cast<int>(verts.size());
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(verts.data(), m_vertexCount * sizeof(Vertex));
    m_vbo.release();
}


void Projectile::buildGeometryIceCube()
{
    struct Vertex { QVector3D pos; QVector3D normal; QVector2D uv; QVector3D color; };

    const QVector3D iceColor(0.6f, 0.8f, 1.0f);
    const float h = 0.5f;

    QVector3D corners[8] = {
        { +h, +h, +h }, { +h, +h, -h }, { -h, +h, -h }, { -h, +h, +h },
        { +h, -h, +h }, { +h, -h, -h }, { -h, -h, -h }, { -h, -h, +h }
    };

    QVector3D normals[6] = {
        {  0, +1,  0 },
        {  0, -1,  0 },
        { +1,  0,  0 },
        { -1,  0,  0 },
        {  0,  0, +1 },
        {  0,  0, -1 }
    };

    int faces[6][4] = {
        { 0, 1, 2, 3 },
        { 4, 7, 6, 5 },
        { 0, 4, 5, 1 },
        { 3, 2, 6, 7 },
        { 0, 3, 7, 4 },
        { 1, 5, 6, 2 }
    };

    QVector2D uv[4] = { {0,0}, {1,0}, {1,1}, {0,1} };

    std::vector<Vertex> verts;
    verts.reserve(6 * 6);

    for (int f = 0; f < 6; ++f) {
        const QVector3D& n = normals[f];
        int i0 = faces[f][0],
            i1 = faces[f][1],
            i2 = faces[f][2],
            i3 = faces[f][3];

        verts.push_back({ corners[i0], n, uv[0], iceColor });
        verts.push_back({ corners[i1], n, uv[1], iceColor });
        verts.push_back({ corners[i2], n, uv[2], iceColor });

        verts.push_back({ corners[i0], n, uv[0], iceColor });
        verts.push_back({ corners[i2], n, uv[2], iceColor });
        verts.push_back({ corners[i3], n, uv[3], iceColor });
    }

    m_vertexCount = static_cast<int>(verts.size());
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(verts.data(), m_vertexCount * sizeof(Vertex));
    m_vbo.release();
}

void Projectile::buildGeometry()
{
    switch (m_shape)
    {
    case Shape::Apple:     buildGeometryApple();     break;
    case Shape::Cherry:  buildGeometryCherry();  break;
    case Shape::bannana: buildGeometryBanana(); break;
    case Shape::IceCube:   buildGeometryIceCube(); break;
    }
}

void Projectile::uploadGeometry()
{
    struct Vertex {
        QVector3D pos;
        QVector3D normal;
        QVector2D uv;
        QVector3D color;
    };

    constexpr int stride = sizeof(Vertex);

    m_vao.create();
    m_vao.bind();

    m_vbo.bind();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, pos)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, normal)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, uv)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, color)));

    m_vbo.release();
    m_vao.release();
}

