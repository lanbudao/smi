#ifndef GEOMETRY_RENDERER_H 
#define GEOMETRY_RENDERER_H

#include "sdk/DPTR.h"
#include "Geometry.h"

NAMESPACE_BEGIN

/*!
 * \brief The GeometryRenderer class
 * TODO: support multiple geometry with same primitive (glPrimitiveRestartIndex, glDrawArraysInstanced, glDrawArraysInstanced, glMultiDrawArrays...)
 */
class GeometryRendererPrivate;
class GeometryRenderer
{
	DPTR_DECLARE_PRIVATE(GeometryRenderer);
public:
	// TODO: VAB, VBUM etc.
	GeometryRenderer();
	virtual ~GeometryRenderer() {}
	// call updateBuffer internally in bindBuffer if feature is changed
	void setFeature(int f, bool on);
	void setFeatures(int value);
	int features() const;
	int actualFeatures() const;
	bool testFeatures(int value) const;
	/*!
	 * \brief updateGeometry
	 * Update geometry buffer. Rebind VBO, IBO to VAO if geometry attributes is changed.
	 * Assume attributes are bound in the order 0, 1, 2,....
	 * Make sure the old geometry is not destroyed before this call because it will be used to compare with the new \l geo
	 * \param geo null: release vao/vbo
	 */
	void updateGeometry(Geometry* geo = NULL);
	virtual void render();
protected:
	void bindBuffers();
	void unbindBuffers();
private:
	DPTR_DECLARE(GeometryRenderer);

};

NAMESPACE_END
#endif
