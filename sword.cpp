#include "Sword.h"
#include <QDebug>

Sword::Sword(QObject* parent)
    : QObject(parent)
{
    initializeOpenGLFunctions();
}

Sword::~Sword() {
    m_vao.destroy();
    m_vbo.destroy();
}

void Sword::buildGeometry()
{
    m_verts.clear();

    const float handleLen = 0.25f;
    const float bladeLen  = 1.0f;
    const float tipLen    = 0.2f;
    const float w         = 0.05f;
    const float t         = 0.02f;

    const QVector3D handleColor(0.4f, 0.2f, 0.1f);
    const QVector3D bladeColor(0.8f, 0.8f, 0.8f);

    auto pushFace = [&](QVector3D a, QVector3D b,
                        QVector3D c, QVector3D d,
                        const QVector3D& col)
    {
        QVector3D n = QVector3D::normal(a, b, c);
        QVector2D uv00{0,0}, uv10{1,0}, uv11{1,1}, uv01{0,1};

        m_verts.push_back({ a, n, uv00, col });
        m_verts.push_back({ b, n, uv10, col });
        m_verts.push_back({ c, n, uv11, col });

        m_verts.push_back({ a, n, uv00, col });
        m_verts.push_back({ c, n, uv11, col });
        m_verts.push_back({ d, n, uv01, col });
    };

    auto pushTri = [&](QVector3D a, QVector3D b,
                       QVector3D c, const QVector3D& col)
    {
        QVector3D n = QVector3D::normal(a, b, c);
        QVector2D uv0{0,0}, uv1{1,0}, uv2{0.5f,1.0f};
        m_verts.push_back({ a, n, uv0, col });
        m_verts.push_back({ b, n, uv1, col });
        m_verts.push_back({ c, n, uv2, col });
    };

    QVector3D h0{-w,      0, -t}, h1{ w,      0, -t},
        h2{ w,      0,  t}, h3{-w,      0,  t},
        h4{-w,handleLen,-t},h5{ w,handleLen,-t},
        h6{ w,handleLen, t},h7{-w,handleLen, t};

    pushFace(h0,h1,h2,h3, handleColor);
    pushFace(h7,h6,h5,h4, handleColor);
    pushFace(h1,h5,h6,h2, handleColor);
    pushFace(h4,h0,h3,h7, handleColor);
    pushFace(h2,h6,h7,h3, handleColor);
    pushFace(h5,h1,h0,h4, handleColor);

    float by0 = handleLen;
    float by1 = handleLen + bladeLen;
    QVector3D b0{-w,by0,-t}, b1{ w,by0,-t},
        b2{ w,by0, t}, b3{-w,by0, t},
        b4{-w,by1,-t}, b5{ w,by1,-t},
        b6{ w,by1, t}, b7{-w,by1, t};

    pushFace(b0,b1,b2,b3, bladeColor);
    pushFace(b7,b6,b5,b4, bladeColor);
    pushFace(b1,b5,b6,b2, bladeColor);
    pushFace(b4,b0,b3,b7, bladeColor);
    pushFace(b2,b6,b7,b3, bladeColor);
    pushFace(b5,b1,b0,b4, bladeColor);

    float ty0 = by1;
    float ty1 = by1 + tipLen;
    float sw  = w * 0.1f;
    float st  = t * 0.1f;

    QVector3D t0{-sw,ty0,-st},
        t1{ sw,ty0,-st},
        t2{  0,ty0, st};

    QVector3D tA{  0,ty1, 0};

    pushTri(t0, t1, tA, bladeColor);
    pushTri(t1, t2, tA, bladeColor);
    pushTri(t2, t0, tA, bladeColor);

    pushFace(b4, b5, t1, t0, bladeColor);
    pushFace(b5, b6, t2, t1, bladeColor);
    pushFace(b6, b7, t0, t2, bladeColor);

    m_vertexCount = static_cast<int>(m_verts.size());
}

void Sword::initialize() {
    buildGeometry();

    if (!m_vao.isCreated()) m_vao.create();
    m_vao.bind();

    if (!m_vbo.isCreated()) m_vbo.create();
    m_vbo.bind();
    m_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);


    m_vbo.allocate(m_verts.data(), m_vertexCount * sizeof(Vertex));


    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, pos))
        );


    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, normal))
        );


    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, uv))
        );


    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, color))
        );

    m_vao.release();
    m_vbo.release();
}

void Sword::render(QOpenGLShaderProgram& shader,
                   const QMatrix4x4& view,
                   const QMatrix4x4& proj)
{
    shader.bind();
    QMatrix4x4 model;

    model.translate(m_position);
    shader.setUniformValue("uModel", model);
    shader.setUniformValue("uView",  view);
    shader.setUniformValue("uProj",  proj);
    shader.setUniformValue("uHasTex", 0);

    m_vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    m_vao.release();
    shader.release();
}
