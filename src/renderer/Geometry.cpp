#include "Geometry.h"
#include "innermath.h"
#include <cstring>

NAMESPACE_BEGIN


Attribute::Attribute(DataType type, int tupleSize, int offset, bool normalize)
    : m_normalize(normalize)
    , m_type(type)
    , m_tupleSize(tupleSize)
    , m_offset(offset)
{}

Attribute::Attribute(const std::string& name, DataType type, int tupleSize, int offset, bool normalize)
    : m_normalize(normalize)
    , m_type(type)
    , m_tupleSize(tupleSize)
    , m_offset(offset)
    , m_name(name)
{}
//
//#ifndef QT_NO_DEBUG_STREAM
//	QDebug operator<<(QDebug dbg, const Attribute &a) {
//		dbg.nospace() << "attribute: " << a.name();
//		dbg.nospace() << ", offset " << a.offset();
//		dbg.nospace() << ", tupleSize " << a.tupleSize();
//		dbg.nospace() << ", dataType " << a.type();
//		dbg.nospace() << ", normalize " << a.normalize();
//		return dbg.space();
//	}
//#endif

Geometry::Geometry(int vertexCount, int indexCount, DataType indexType)
    : m_primitive(TriangleStrip)
    , m_itype(indexType)
    , m_vcount(vertexCount)
    , m_icount(indexCount)
{}

int Geometry::indexDataSize() const
{
    switch (indexType()) {
    case TypeU16: return indexCount() * 2;
    case TypeU32: return indexCount() * 4;
    default: return indexCount();
    }
}

void Geometry::setIndexValue(int index, int value)
{
    switch (indexType()) {
    case TypeU8: {
        unsigned char* d = (unsigned char*)m_idata.data();
        *(d + index) = value;
    }
        break;
    case TypeU16: {
        unsigned short* d = (unsigned short*)m_idata.data();
        *(d + index) = value;
    }
        break;
    case TypeU32: {
        unsigned int* d = (unsigned int*)m_idata.data();
        *(d + index) = value;
    }
        break;
    default:
        break;
    }
}

void Geometry::setIndexValue(int index, int v1, int v2, int v3)
{
    // TODO: *(d + 3*index), *(d + 3*index + 1), *(d + 3*index + 2)
    switch (indexType()) {
    case TypeU8: {
        unsigned char* d = (unsigned char*)m_idata.data();
        *(d + index++) = v1;
        *(d + index++) = v2;
        *(d + index++) = v2;
    }
        break;
    case TypeU16: {
        unsigned short* d = (unsigned short*)m_idata.data();
        *(d + index++) = v1;
        *(d + index++) = v2;
        *(d + index++) = v3;
    }
        break;
    case TypeU32: {
        unsigned int* d = (unsigned int*)m_idata.data();
        *(d + index++) = v1;
        *(d + index++) = v2;
        *(d + index++) = v3;
    }
        break;
    default:
        break;
    }
}

void Geometry::dumpVertexData()
{
    printf("vertex %p: ", m_vdata.data());
    const int n = stride() / sizeof(float);
    for (int i = 0; i < m_vcount; ++i) {
        const float* f = (const float*)(m_vdata.data() + i * stride());
        for (int j = 0; j < n; ++j) {
            printf("%f, ", *(f + j));
        }
        printf(";");
    }
    printf("\n"); fflush(0);
}

void Geometry::dumpIndexData()
{
    switch (indexType()) {
    case TypeU8: {
        unsigned char* d = (unsigned char*)m_idata.data();
        for (int i = 0; i < m_icount; ++i) printf("%u, ", *(d + i));
    }
        break;
    case TypeU16: {
        unsigned short* d = (unsigned short*)m_idata.data();
        for (int i = 0; i < m_icount; ++i) printf("%u, ", *(d + i));
    }
        break;
    case TypeU32: {
        unsigned int* d = (unsigned int*)m_idata.data();
        for (int i = 0; i < m_icount; ++i) printf("%u, ", *(d + i));
    }
        break;
    default:
        break;
    }
    printf("\n"); fflush(0);
}

void Geometry::allocate(int nbVertex, int nbIndex)
{
    m_icount = nbIndex;
    m_vcount = nbVertex;
    m_vdata.resize(nbVertex*stride());
    memset(m_vdata.data(), 0, m_vdata.size());
    if (nbIndex <= 0) {
        m_idata.clear(); // required?
        return;
    }
    switch (indexType()) {
    case TypeU8:
        m_idata.resize(nbIndex * sizeof(unsigned char));
        break;
    case TypeU16:
        m_idata.resize(nbIndex * sizeof(unsigned short));
        break;
    case TypeU32:
        m_idata.resize(nbIndex * sizeof(unsigned int));
        break;
    default:
        break;
    }
    memset((void*)m_idata.data(), 0, m_idata.size());
}

bool Geometry::compare(const Geometry *other) const
{
    // this == other: attributes and stride can be different
    if (!other)
        return false;
    if (stride() != other->stride())
        return false;
    return attributes() == other->attributes();
}

TexturedGeometry::TexturedGeometry()
    : Geometry()
    , nb_tex(0)
    , geo_rect(-1, 1, 2, -2) // (-1, -1, 2, 2) flip y
{
    setVertexCount(4);
    a.push_back(Attribute(TypeF32, 2, 0));
    a.push_back(Attribute(TypeF32, 2, 2 * sizeof(float)));
    setTextureCount(1);
}

void TexturedGeometry::setTextureCount(int value)
{
    if (value == nb_tex)
        return;
    texRect.resize(value);
    nb_tex = value;
}

int TexturedGeometry::textureCount() const
{
    return nb_tex;
}

void TexturedGeometry::setPoint(int index, const PointF &p, const PointF &tp, int texIndex)
{
    setGeometryPoint(index, p);
    setTexturePoint(index, tp, texIndex);
}

void TexturedGeometry::setGeometryPoint(int index, const PointF &p)
{
    float *v = (float*)(m_vdata.data() + index * stride());
    *v = p.x();
    *(v + 1) = p.y();
}

void TexturedGeometry::setTexturePoint(int index, const PointF &tp, int texIndex)
{
    float *v = (float*)(m_vdata.data() + index * stride() + (texIndex + 1) * 2 * sizeof(float));
    *v = tp.x();
    *(v + 1) = tp.y();
}

void TexturedGeometry::setRect(const RectF &r, const RectF &tr, int texIndex)
{
    setPoint(0, r.topLeft(), tr.topLeft(), texIndex);
    setPoint(1, r.bottomLeft(), tr.bottomLeft(), texIndex);
    switch (primitive()) {
    case TriangleStrip:
        setPoint(2, r.topRight(), tr.topRight(), texIndex);
        setPoint(3, r.bottomRight(), tr.bottomRight(), texIndex);
        break;
    case TriangleFan:
        setPoint(3, r.topRight(), tr.topRight(), texIndex);
        setPoint(2, r.bottomRight(), tr.bottomRight(), texIndex);
        break;
    case Triangles:
        break;
    default:
        break;
    }
}

void TexturedGeometry::setGeometryRect(const RectF &r)
{
    geo_rect = r;
}

void TexturedGeometry::setTextureRect(const RectF &tr, int texIndex)
{
    if (texRect.size() <= texIndex)
        texRect.resize(texIndex + 1);
    texRect[texIndex] = tr;
}

const std::vector<Attribute>& TexturedGeometry::attributes() const
{
    return a;
}

void TexturedGeometry::create()
{
    allocate(vertexCount());
    if (a.size() - 1 < textureCount()) { // the first is position
        for (int i = a.size() - 1; i < textureCount(); ++i)
            a.push_back(Attribute(TypeF32, 2, int((i + 1) * 2 * sizeof(float))));
    }
    else {
        a.resize(textureCount() + 1);
    }

    setGeometryPoint(0, geo_rect.topLeft());
    setGeometryPoint(1, geo_rect.bottomLeft());
    switch (primitive()) {
    case TriangleStrip:
        setGeometryPoint(2, geo_rect.topRight());
        setGeometryPoint(3, geo_rect.bottomRight());
        break;
    case TriangleFan:
        setGeometryPoint(3, geo_rect.topRight());
        setGeometryPoint(2, geo_rect.bottomRight());
        break;
    case Triangles:
        break;
    default:
        break;
    }

    for (int i = 0; i < texRect.size(); ++i) {
        const RectF tr = texRect[i];
        setTexturePoint(0, tr.topLeft(), i);
        setTexturePoint(1, tr.bottomLeft(), i);
        switch (primitive()) {
        case TriangleStrip:
            setTexturePoint(2, tr.topRight(), i);
            setTexturePoint(3, tr.bottomRight(), i);
            break;
        case TriangleFan:
            setTexturePoint(3, tr.topRight(), i);
            setTexturePoint(2, tr.bottomRight(), i);
            break;
        case Triangles:
            break;
        default:
            break;
        }
    }
}


Sphere::Sphere()
    : TexturedGeometry()
    , r(1)
{
    setPrimitive(Triangles);
    setResolution(128, 128);
    a.push_back(Attribute(TypeF32, 3, 0));
    a.push_back(Attribute(TypeF32, 2, 3 * sizeof(float)));
}

void Sphere::setResolution(int w, int h)
{
    ru = w;
    rv = h;
    setVertexCount((ru + 1)*(rv + 1));
}

void Sphere::setRadius(float value)
{
    r = value;
}

float Sphere::radius() const
{
    return r;
}

void Sphere::create()
{
    allocate(vertexCount(), ru*rv * 3 * 2); // quads * 2 triangles,
    if (a.size() - 1 < nb_tex) { // the first is position
        for (int i = a.size() - 1; i < nb_tex; ++i)
            a.push_back(Attribute(TypeF32, 2, 3 * sizeof(float) + int(i * 2 * sizeof(float))));
    }
    else {
        a.resize(nb_tex + 1);
    }

    // TODO: use geo_rect?
    float *vd = (float*)m_vdata.data();
    const float dTheta = M_PI * 2.0 / float(ru);
    const float dPhi = M_PI / float(rv);
    //const float du = 1.0f/float(ru);
    //const float dv = 1.0f/float(rv);
    for (int lat = 0; lat <= rv; ++lat) {
        const float phi = M_PI_2 - float(lat)*dPhi;
        const float cosPhi = std::cos(phi);
        const float sinPhi = std::sin(phi);
        //const float v = 1.0f - float(lat)*dv; // flip y?
        for (int lon = 0; lon <= ru; ++lon) {
            const float theta = float(lon)*dTheta;
            const float cosTheta = std::cos(theta);
            const float sinTheta = std::sin(theta);
            //const float u = float(lon) * du;
            *vd++ = r * cosPhi*cosTheta;//2.0*float(lon)/float(ru) -1.0;//
            *vd++ = r * sinPhi;//2.0*float(lat)/float(rv)-1.0;//
            *vd++ = r * cosPhi*sinTheta;
            for (int i = 0; i < nb_tex; ++i) {
                *vd++ = texRect[i].x() + texRect[i].width() / float(ru) * float(lon);
                *vd++ = texRect[i].y() + texRect[i].height() / float(rv) * float(lat);
            }
        }
    }
    // create index data
    if (m_icount > 0) {
        int idx = 0;
        for (int lat = 0; lat < rv; ++lat) {
            for (int lon = 0; lon < ru; ++lon) {
                const int ring = lat * (ru + 1) + lon;
                const int ringNext = ring + ru + 1;
                setIndexValue(idx, ring, ringNext, ring + 1);
                setIndexValue(idx + 3, ringNext, ringNext + 1, ring + 1);
                idx += 6;
            }
        }
    }
}

NAMESPACE_END
