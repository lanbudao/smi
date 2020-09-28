#include "ShaderManager.h"
#include "VideoShader.h"
#include "VideoMaterial.h"
#include "AVLog.h"
#include <map>

NAMESPACE_BEGIN

class ShaderManagerPrivate
{
public:
	ShaderManagerPrivate() {}
	~ShaderManagerPrivate()
	{
		for (auto &s: shader_cache) {
			VideoShader *shader = static_cast<VideoShader*>(s.second); 
			delete shader;
		}
		shader_cache.clear();
	}
	std::map<int, VideoShader*> shader_cache;
};
ShaderManager::ShaderManager():
	d_ptr(new ShaderManagerPrivate)
{
}


ShaderManager::~ShaderManager()
{
}

VideoShader* ShaderManager::prepareMaterial(VideoMaterial *material, int materialType)
{
	DPTR_D(ShaderManager);
	const int type = materialType != -1 ? materialType : material->type();
	VideoShader *shader = d->shader_cache[type];
	if (shader)
		return shader;
	AVDebug("[ShaderManager] cache a new shader material type(%d): %s.\n", type,
		VideoMaterial::typeName(type).c_str());
	shader = new VideoShader();
	shader->setVideoFormat(material->currentFormat());
	shader->setTextureTarget(material->textureTarget());
	shader->setMaterialType(material->type());
	shader->initialize();
	d->shader_cache[type] = shader;
	return shader;
}

NAMESPACE_END
