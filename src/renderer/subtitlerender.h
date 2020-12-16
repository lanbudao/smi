#ifndef SUBTITLE_RENDER_H
#define SUBTITLE_RENDER_H

#include "sdk/global.h"
#include "sdk/DPTR.h"

NAMESPACE_BEGIN

class SubtitleRenderPrivate;
class SubtitleRender
{
    DPTR_DECLARE_PRIVATE(SubtitleRender);
public:
    SubtitleRender();
    ~SubtitleRender();

    /**
     * @brief set the width of renderer
     * @param w the width of screen
     * @param h the height of screen
     */
    void setRenderWidth(int w, int h);
    /**
     * @brief set the pos of renderer
     * @param pos the effective range is 0~1.0, default value is 1.0.
     * It means the bottom of the screen
     */
    void setRenderPos(float pos);
    void setFontFile(const std::string &file);
    void setFontSize(int size);
    void drawText(const std::string& text);

    void drawPixmap() {}

private:
    DPTR_DECLARE(SubtitleRender);
};

#endif // SUBTITLE_RENDER_H

NAMESPACE_END