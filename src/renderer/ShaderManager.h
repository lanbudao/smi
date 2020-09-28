#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <sdk/DPTR.h>
#include "global.h"

NAMESPACE_BEGIN
class VideoShader;
class VideoMaterial;
/*!
 * \brief The ShaderManager class
 * Cache VideoShader and shader programes for different video material type.
 * TODO: ShaderManager does not change for a given vo, so we can expose VideoRenderer.shaderManager() to set custom shader. It's better than VideoRenderer.opengl() because OpenGLVideo exposes too many apis that may confuse user.
 */
 class ShaderManagerPrivate;
class ShaderManager
{
    DPTR_DECLARE_PRIVATE(ShaderManager);
public:
    ShaderManager();
    ~ShaderManager();
    VideoShader* prepareMaterial(VideoMaterial *material, int materialType = -1);
//    void setCacheSize(int value);

private:
    DPTR_DECLARE(ShaderManager);
};
NAMESPACE_END
#endif // SHADERMANAGER_H
