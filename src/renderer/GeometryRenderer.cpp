#include "GeometryRenderer.h"
#include "glpackage.h"
#include "AVLog.h"

NAMESPACE_BEGIN

class GeometryRendererPrivate
{
public:
	// rendering features. Use all possible features as the default.
	static const int kVBO = 0x01;
	static const int kIBO = 0x02;
	static const int kVAO = 0x04;
	static const int kMapBuffer = 1 << 16;
	GeometryRendererPrivate()
		: g(NULL)
		, features_(kVBO | kIBO | kVAO | kMapBuffer)
		, vbo_size(0)
		, ibo_size(0)
		, ibo(GLBuffer::IndexBuffer)
		, stride(0)
	{

	}

	Geometry *g;
	int features_;
	int vbo_size, ibo_size; // QOpenGLBuffer.size() may get error 0x501
	GLBuffer vbo; //VertexBuffer
	GLArray vao;
	GLBuffer ibo;

	// geometry characteristic
	int stride;
	std::vector<Attribute> attrib;
};


GeometryRenderer::GeometryRenderer():
	d_ptr(new GeometryRendererPrivate)
{
	//static bool disable_ibo = qgetenv("QTAV_NO_IBO").toInt() > 0;
	//setFeature(kIBO, !disable_ibo);
	//static bool disable_vbo = qgetenv("QTAV_NO_VBO").toInt() > 0;
	//setFeature(kVBO, !disable_vbo);
	//static bool disable_vao = qgetenv("QTAV_NO_VAO").toInt() > 0;
	//setFeature(kVAO, !disable_vao);
}

void GeometryRenderer::setFeature(int f, bool on)
{
	DPTR_D(GeometryRenderer);
	if (on)
		d->features_ |= f;
	else
		d->features_ ^= f;
}

void GeometryRenderer::setFeatures(int value)
{
	DPTR_D(GeometryRenderer);
	d->features_ = value;
}

int GeometryRenderer::features() const
{
	return d_func()->features_;
}

int GeometryRenderer::actualFeatures() const
{
	DPTR_D(const GeometryRenderer);
	int f = 0;
	if (d->vbo.isCreated())
		f |= d->kVBO;
	if (d->ibo.isCreated())
		f |= d->kIBO;
	if (d->vao.isCreated())
		f |= d->kVAO;
	return f;
}

bool GeometryRenderer::testFeatures(int value) const
{
	return !!(features() & value);
}

void GeometryRenderer::updateGeometry(Geometry *geo)
{
	DPTR_D(GeometryRenderer);
	d->g = geo;
	if (!d->g) {
		d->ibo.destroy();
		d->vbo.destroy();
		d->vao.destroy();
		d->vbo_size = 0;
		d->ibo_size = 0;
		return;
	}
	static int support_map = -1;
	if (support_map < 0) {
		support_map = 1;
	}
	if (testFeatures(d->kIBO) && !d->ibo.isCreated()) {
		if (d->g->indexCount() > 0) {
			AVDebug("creating IBO...\n");
			if (!d->ibo.create())
				AVDebug("IBO create error\n");
		}
	}
	if (d->ibo.isCreated()) {
		d->ibo.bind();
		const int bs = d->g->indexDataSize();
		if (bs == d->ibo_size) {
			void * p = NULL;
			if (support_map && testFeatures(d->kMapBuffer))
				p = d->ibo.map(GLBuffer::WriteOnly);
			if (p) {
				memcpy(p, d->g->constIndexData(), bs);
				d->ibo.unmap();
			}
			else {
				d->ibo.write(0, d->g->constIndexData(), bs);
			}
		}
		else {
			d->ibo.allocate(d->g->indexData(), bs); // TODO: allocate NULL and then map or BufferSubData?
			d->ibo_size = bs;
		}
		d->ibo.unbind();
	}
	if (testFeatures(d->kVBO) && !d->vbo.isCreated()) {
		AVDebug("creating VBO...\n");
		if (!d->vbo.create())
			AVWarning("VBO create error\n");
	}
	if (d->vbo.isCreated()) {
		d->vbo.bind();
		const int bs = d->g->vertexCount()*d->g->stride();
		/* Notes from https://www.opengl.org/sdk/docs/man/html/glBufferSubData.xhtml
		   When replacing the entire data store, consider using glBufferSubData rather than completely recreating the data store with glBufferData. This avoids the cost of reallocating the data store.
		 */
		if (bs == d->vbo_size) { // vbo.size() error 0x501 on rpi, and query gl value can be slow
			void* p = NULL;
			if (support_map && testFeatures(d->kMapBuffer))
				p = d->vbo.map(GLBuffer::WriteOnly);
			if (p) {
				memcpy(p, d->g->constVertexData(), bs);
				d->vbo.unmap();
			}
			else {
				d->vbo.write(0, d->g->constVertexData(), bs);
				d->vbo_size = bs;
			}
		}
		else {
			d->vbo.allocate(d->g->vertexData(), bs);
		}
		d->vbo.unbind();
	}
	if (d->stride == d->g->stride() && d->attrib == d->g->attributes())
		return;
	d->stride = d->g->stride();
	d->attrib = d->g->attributes();

	if (testFeatures(d->kVAO) && !d->vao.isCreated()) {
		AVDebug("creating VAO...\n");
		if (!d->vao.create())
			AVDebug("VAO create error\n");
	}
	AVDebug("vao updated\n");
	if (d->vao.isCreated()) // can not use vao binder because it will create a vao if necessary
		d->vao.bind();
	// can set data before vao bind
	if (!d->vao.isCreated())
		return;
	AVDebug("geometry attributes changed, rebind vao...\n");
	// call once is enough if no feature and no geometry attribute is changed
	if (d->vbo.isCreated()) {
		d->vbo.bind();
		for (int an = 0; an < d->g->attributes().size(); ++an) {
			// FIXME: assume bind order is 0,1,2...
			const Attribute& a = d->g->attributes().at(an);
			glVertexAttribPointer(an, a.tupleSize(), a.type(), a.normalize(), d->g->stride(), 
				reinterpret_cast<const void *>(/*qptrdiff*/(a.offset()))); //TODO: in setActiveShader
			glEnableVertexAttribArray(an);
		}
		d->vbo.unbind(); // unbind after vao unbind? http://www.zwqxin.com/archives/opengl/vao-and-vbo-stuff.html
	} // TODO: bind pointers if vbo is disabled
	// bind ibo to vao thus no bind is required later
	if (d->ibo.isCreated())// if not bind here, glDrawElements(...,NULL) crashes and must use ibo data ptr, why?
		d->ibo.bind();
	d->vao.unbind();
	if (d->ibo.isCreated())
		d->ibo.unbind();
	AVDebug("geometry updated\n");
}

void GeometryRenderer::bindBuffers()
{
	DPTR_D(GeometryRenderer);
	bool bind_vbo = d->vbo.isCreated();
	bool bind_ibo = d->ibo.isCreated();
	bool setv_skip = false;

	if (d->vao.isCreated()) {
		d->vao.bind(); // vbo, ibo is ok now
		setv_skip = bind_vbo;
		bind_vbo = false;
		bind_ibo = false;
	}

	//qDebug("bind ibo: %d vbo: %d; set v: %d", bind_ibo, bind_vbo, !setv_skip);
	if (bind_ibo)
		d->ibo.bind();
	// no vbo: set vertex attributes
	// has vbo, no vao: bind vbo & set vertex attributes
	// has vbo, has vao: skip
	if (setv_skip)
		return;
	if (!d->g)
		return;
	const char* vdata = static_cast<const char*>(d->g->vertexData());
	if (bind_vbo) {
		d->vbo.bind();
		vdata = NULL;
	}
	for (int an = 0; an < d->g->attributes().size(); ++an) {
		const Attribute& a = d->g->attributes().at(an);
		glVertexAttribPointer(an, a.tupleSize(), a.type(), a.normalize(), d->g->stride(), vdata + a.offset());
		glEnableVertexAttribArray(an); //TODO: in setActiveShader
	}
}

void GeometryRenderer::unbindBuffers()
{
	DPTR_D(GeometryRenderer);
	bool unbind_vbo = d->vbo.isCreated();
	bool unbind_ibo = d->ibo.isCreated();
	bool unsetv_skip = false;

	if (d->vao.isCreated()) {
		d->vao.unbind();
		unsetv_skip = unbind_vbo;
		unbind_vbo = false;
		unbind_ibo = false;
	}

	//qDebug("unbind ibo: %d vbo: %d; unset v: %d", unbind_ibo, unbind_vbo, !unsetv_skip);
	if (unbind_ibo)
		d->ibo.unbind();
	// unbind vbo. qpainter is affected if vbo is bound
	if (unbind_vbo)
		d->vbo.unbind();
	// no vbo: disable vertex attributes
	// has vbo, no vao: unbind vbo & set vertex attributes
	// has vbo, has vao: skip
	if (unsetv_skip)
		return;
	if (!d->g)
		return;
	for (int an = 0; an < d->g->attributes().size(); ++an) {
		glDisableVertexAttribArray(an);
	}
}

void GeometryRenderer::render()
{
	DPTR_D(GeometryRenderer);
	if (!d->g)
		return;
	bindBuffers();
	if (d->g->indexCount() > 0) {
		glDrawElements(d->g->primitive(), d->g->indexCount(), d->g->indexType(), d->ibo.isCreated() ? NULL : d->g->indexData()); // null: data in vao or ibo. not null: data in memory
	}
	else {
		glDrawArrays(d->g->primitive(), 0, d->g->vertexCount());
	}
	unbindBuffers();
}

NAMESPACE_END
