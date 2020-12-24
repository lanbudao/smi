#include "subtitlerender.h"
#ifdef SMI_HAVE_FREETYPE
#include "../../depends/ftgl/FTGL/ftgl.h"
#endif
#include "sdk/AVLog.h"
#include "glad/glad.h"
#include "glpackage.h"

NAMESPACE_BEGIN


class SubtitleRenderPrivate
{
public:
    SubtitleRenderPrivate():
        font(nullptr),
        fontsize(22),
        pos(1.0)
    {
#ifdef SMI_HAVE_FREETYPE
        layout.SetAlignment(FTGL::ALIGN_CENTER);
#endif
    }
    ~SubtitleRenderPrivate()
    {
#ifdef SMI_HAVE_FREETYPE
        if (font)
            delete font;
#endif
    }
#ifdef SMI_HAVE_FREETYPE
    FTFont* font;
    int fontsize;
    int render_width, render_height;
    float pos;
    FTSimpleLayout layout;
#endif
};

SubtitleRender::SubtitleRender():
    d_ptr(new SubtitleRenderPrivate)
{
    setRenderWidth(800, 480);
}

SubtitleRender::~SubtitleRender()
{
}

void SubtitleRender::setRenderWidth(int w, int h)
{
    DPTR_D(SubtitleRender);
    d->render_width = w;
    d->render_height = h;
    d->layout.SetLineLength(w);
}

void SubtitleRender::setRenderPos(float pos)
{
    DPTR_D(SubtitleRender);
    d->pos = pos;
}

void SubtitleRender::setFontFile(const std::string & file)
{
    if (file.empty())
        return;
#ifdef SMI_HAVE_FREETYPE
    DPTR_D(SubtitleRender);
    if (!d->font)
        delete d->font;
    d->font = new FTPixmapFont(file.c_str());
    d->layout.SetFont(d->font);
    if (d->font->Error()) {
        AVError("Could not load font %s\n", file.c_str());
        return;
    }
    d->font->FaceSize(d->fontsize);
    d->font->CharMap(ft_encoding_unicode);
#endif
}

void SubtitleRender::setFontSize(int size)
{
#ifdef SMI_HAVE_FREETYPE
    DPTR_D(SubtitleRender);
    d->fontsize = size;
    if (d->font) {
        d->font->FaceSize(size);
    }
#endif
}

void SubtitleRender::drawText(const std::string & text)
{
#ifdef SMI_HAVE_FREETYPE
    DPTR_D(SubtitleRender);
    if (!d->font)
        return;
    FTBBox box = d->layout.BBox(text.c_str());
    int scender = std::abs(box.Upper().Y() - box.Lower().Y());
    float max_pos = (d->render_height - scender) * 1.0 / (d->render_height);
    float pos = 1.0f - (d->pos * (max_pos * 2)); //To real pos, -1.0f~1.0f
    glRasterPos2f(-1.0f, pos);
    d->layout.Render(text.c_str());
#endif
}

NAMESPACE_END