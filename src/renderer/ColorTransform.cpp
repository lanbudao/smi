/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#include "ColorTransform.h"
#include "AVLog.h"
#include "innermath.h"

NAMESPACE_BEGIN

// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
static const Matrix4x4 kXYZ2sRGB(3.2404542f,  -1.5371385f, -0.4985314f, 0.0f,
                                 -0.9692660f,  1.8760108f,  0.0415560f, 0.0f,
                                 0.0556434f, -0.2040259f,  1.0572252f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f);
// http://www.cs.utah.edu/~halzahaw/CS7650/Project2/project2_index.html no gamma correction
static const Matrix4x4 kXYZ_RGB(2.5623f,  -1.1661f, -0.3962f, 0.0f,
                                -1.0215f,  1.9778f, 0.0437f,  0.0f,
                                0.0752f, -0.2562f, 1.1810f,  0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f);

static const Matrix4x4 kGBR2RGB = Matrix4x4(0, 0, 1, 0,
                                            1, 0, 0, 0,
                                            0, 1, 0, 0,
                                            0, 0, 0, 1);

const Matrix4x4 yuv2rgb_bt601 =
        Matrix4x4(
            1.0f,  0.000f,  1.402f, 0.0f,
            1.0f, -0.344f, -0.714f, 0.0f,
            1.0f,  1.772f,  0.000f, 0.0f,
            0.0f,  0.000f,  0.000f, 1.0f)
        *
        Matrix4x4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, -0.5f,
            0.0f, 0.0f, 1.0f, -0.5f,
            0.0f, 0.0f, 0.0f, 1.0f)
        ;
const Matrix4x4 yuv2rgb_bt709 =
        Matrix4x4(
            1.0f,  0.000f,  1.5701f, 0.0f,
            1.0f, -0.187f, -0.4664f, 0.0f,
            1.0f,  1.8556f, 0.000f,  0.0f,
            0.0f,  0.000f,  0.000f,  1.0f)
        *
        Matrix4x4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, -0.5f,
            0.0f, 0.0f, 1.0f, -0.5f,
            0.0f, 0.0f, 0.0f, 1.0f)
        ;

// For yuv->rgb, assume yuv is full range before convertion, rgb is full range after convertion. so if input yuv is limited range, transform to full range first. If display rgb is limited range, transform to limited range at last.
// *ColorRangeYUV(...)
static Matrix4x4 ColorRangeYUV(ColorRange from, ColorRange to)
{
    if (from == to)
        return Matrix4x4();
    static const float Y2 = 235, Y1 = 16, C2 = 240, C1 = 16;
    static const float s = 255; //TODO: can be others
    if (from == ColorRange_Limited) { //TODO: Unknown. But what if realy want unknown?
        AVDebug("input yuv limited range\n");
        // [Y1, Y2] => [0, s]
        Matrix4x4 m;
        m.scale(s/(Y2 - Y1), s/(C2 - C1), s/(C2 - C1));
        m.translate(-Y1/s, -C1/s, -C1/s);
        return m;
    }
    if (from == ColorRange_Full) {
        // [0, s] => [Y1, Y2]
        Matrix4x4 m;
        m.translate(Y1/s, C1/s, C1/s);
        m.scale((Y2 - Y1)/s, (C2 - C1)/s, (C2 - C1)/s);
        return m;
    }
    // ColorRange_Unknown
    return Matrix4x4();
}

// ColorRangeRGB(...)*
static Matrix4x4 ColorRangeRGB(ColorRange from, ColorRange to)
{
    if (from == to)
        return Matrix4x4();
    static const float R2 = 235, R1 = 16;
    static const float s = 255;
    if (to == ColorRange_Limited) {
        AVDebug("output rgb limited range");
        Matrix4x4 m;
        m.translate(R1/s, R1/s, R1/s);
        m.scale((R2 - R1)/s, (R2 - R1)/s, (R2 - R1)/s);
        return m;
    }
    if (to == ColorRange_Full) { // TODO: Unknown
        Matrix4x4 m;
        m.scale(s/(R2 - R1), s/(R2 - R1), s/(R2 - R1));
        m.translate(-s/R1, -s/R1, -s/R1);
        return m;
    }
    return Matrix4x4();
}

class ColorTransformPrivate
{
public:
    ColorTransformPrivate()
        : recompute(true)
        , cs_in(ColorSpace_RGB)
        , cs_out(ColorSpace_RGB)
        , range_in(ColorRange_Limited)
        , range_out(ColorRange_Full)
        , hue(0)
        , saturation(0)
        , contrast(0)
        , brightness(0)
        , bpc_scale(1.0)
        , a_bpc_scale(false)
    {}
    ~ColorTransformPrivate() {}

    void reset() {
        recompute = true;
        //cs_in = cs_out = ColorSpace_RGB; ///
        //range_in = range_out = ColorRange_Unknown;
        hue = 0;
        saturation = 0;
        contrast = 0;
        brightness = 0;
        bpc_scale = 1.0;
        a_bpc_scale = false;
        M.setToIdentity();
    }
    // TODO: optimize for other color spaces
    void compute() const {
        recompute = false;
        //http://docs.rainmeter.net/tips/colormatrix-guide
        //http://www.graficaobscura.com/matrix/index.html
        //http://beesbuzz.biz/code/hsv_color_transforms.php
        // ??
        const float b = brightness;
        // brightness R,G,B
        Matrix4x4 B(1, 0, 0, b,
                    0, 1, 0, b,
                    0, 0, 1, b,
                    0, 0, 0, 1);
        // Contrast (offset) R,G,B
        const float c = contrast + 1.0f;
        const float t = (1.0f - c) / 2.0f;
        Matrix4x4 C(c, 0, 0, t,
                    0, c, 0, t,
                    0, 0, c, t,
                    0, 0, 0, 1);
        // Saturation
        const float wr = 0.3086f;
        const float wg = 0.6094f;
        const float wb = 0.0820f;
        float s = saturation + 1.0f;
        Matrix4x4 S(
                    (1.0f - s)*wr + s, (1.0f - s)*wg    , (1.0f - s)*wb    , 0.0f,
                    (1.0f - s)*wr    , (1.0f - s)*wg + s, (1.0f - s)*wb    , 0.0f,
                    (1.0f - s)*wr    , (1.0f - s)*wg    , (1.0f - s)*wb + s, 0.0f,
                    0.0f,              0.0f,              0.0f, 1.0f
                    );
        // Hue
        const float n = 1.0f / sqrtf(3.0f);       // normalized hue rotation axis: sqrt(3)*(1 1 1)
        const float h = static_cast<float>(hue * M_PI);               // hue rotation angle
        const float hc = cosf(h);
        const float hs = sinf(h);
        Matrix4x4 H(     // conversion of angle/axis representation to matrix representation
                         n*n*(1.0f - hc) + hc  , n*n*(1.0f - hc) - n*hs, n*n*(1.0f - hc) + n*hs, 0.0f,
                         n*n*(1.0f - hc) + n*hs, n*n*(1.0f - hc) + hc  , n*n*(1.0f - hc) - n*hs, 0.0f,
                         n*n*(1.0f - hc) - n*hs, n*n*(1.0f - hc) + n*hs, n*n*(1.0f - hc) + hc  , 0.0f,
                         0.0f,                   0.0f,                   0.0f, 1.0f
                         );

        // B*C*S*H*rgb_range_mat(*yuv2rgb*yuv_range_mat)*bpc_scale
        M = B*C*S*H;
        // M *= rgb_range_translate*rgb_range_scale
        // TODO: transform to output color space other than RGB
        switch (cs_out) {
        case ColorSpace_XYZ:
            M = kXYZ2sRGB.inverted() * M;
            break;
        case ColorSpace_RGB:
            M *= ColorRangeRGB(ColorRange_Full, range_out);
            break;
        case ColorSpace_GBR:
            M *= ColorRangeRGB(ColorRange_Full, range_out);
            M = kGBR2RGB.inverted() * M;
            break;
        default:
            M = YUV2RGB(cs_out).inverted() * M;
            break;
        }

        switch (cs_in) {
        case ColorSpace_XYZ:
            M *= kXYZ2sRGB;
            break;
        case ColorSpace_RGB:
            break;
        case ColorSpace_GBR:
            M *= kGBR2RGB;
            break;
        default:
            M *= YUV2RGB(cs_in)*ColorRangeYUV(range_in, ColorRange_Full);
            break;
        }
        if (bpc_scale != 1.0 && cs_in != ColorSpace_XYZ) { // why no range correction for xyz?
            //qDebug("bpc scale: %f", bpc_scale);
            M *= Matrix4x4(bpc_scale, 0, 0, 0,
                           0, bpc_scale, 0, 0,
                           0, 0, bpc_scale, 0,
                           0, 0, 0, a_bpc_scale ? bpc_scale : 1); // scale alpha channel too
        }
        //qDebug() << "color mat: " << M;
    }

    mutable bool recompute;
    ColorSpace cs_in, cs_out;
    ColorRange range_in, range_out;
    float hue, saturation, contrast, brightness;
    float bpc_scale;
    bool a_bpc_scale;
    mutable Matrix4x4 M; // count the transformations between spaces
};

ColorTransform::ColorTransform()
    : d_ptr(new ColorTransformPrivate())
{
}

ColorTransform::~ColorTransform()
{
}

ColorSpace ColorTransform::inputColorSpace() const
{
    DPTR_D(const ColorTransform);
    return d->cs_in;
}

void ColorTransform::setInputColorSpace(ColorSpace cs)
{
    DPTR_D(ColorTransform);
    if (d->cs_in == cs)
        return;
    d->cs_in = cs;
    d->recompute = true; //TODO: only recompute color space transform
}

ColorSpace ColorTransform::outputColorSpace() const
{
    DPTR_D(const ColorTransform);
    return d->cs_out;
}

void ColorTransform::setOutputColorSpace(ColorSpace cs)
{
    DPTR_D(ColorTransform);
    if (d->cs_out == cs)
        return;
    d->cs_out = cs;
    d->recompute = true; //TODO: only recompute color space transform
}

ColorRange ColorTransform::inputColorRange() const
{
    DPTR_D(const ColorTransform);
    return d->range_in;
}

void ColorTransform::setInputColorRange(ColorRange value)
{
    DPTR_D(ColorTransform);
    if (d->range_in == value)
        return;
    d->range_in = value;
    d->recompute = true;
}

ColorRange ColorTransform::outputColorRange() const
{
    DPTR_D(const ColorTransform);
    return d->range_out;
}

void ColorTransform::setOutputColorRange(ColorRange value)
{
    DPTR_D(ColorTransform);
    if (d->range_out == value)
        return;
    d->range_out = value;
    d->recompute = true;
}

Matrix4x4 ColorTransform::matrix() const
{
    DPTR_D(const ColorTransform);
    if (d->recompute)
        d->compute();
    return d->M;
}

const Matrix4x4& ColorTransform::matrixRef() const
{
    DPTR_D(const ColorTransform);
    if (d->recompute)
        d->compute();
    return d->M;
}

/*!
 * \brief reset
 *   set to identity
 */
void ColorTransform::reset()
{
    DPTR_D(ColorTransform);
    d->reset();
}

void ColorTransform::setBrightness(float brightness)
{
    DPTR_D(ColorTransform);
    if (d->brightness == brightness)
        return;
    d->brightness = brightness;
    d->recompute = true;
}

double ColorTransform::brightness() const
{
    DPTR_D(const ColorTransform);
    return d->brightness;
}

// -1~1
void ColorTransform::setHue(float hue)
{
    DPTR_D(ColorTransform);
    if (d->hue == hue)
        return;
    d->hue = hue;
    d->recompute = true;
}

double ColorTransform::hue() const
{
    DPTR_D(const ColorTransform);
    return d->hue;
}

void ColorTransform::setContrast(float contrast)
{
    DPTR_D(ColorTransform);
    if (d->contrast == contrast)
        return;
    d->contrast = contrast;
    d->recompute = true;
}

double ColorTransform::contrast() const
{
    DPTR_D(const ColorTransform);
    return d->contrast;
}

void ColorTransform::setSaturation(float saturation)
{
    DPTR_D(ColorTransform);
    if (d->saturation == saturation)
        return;
    d->saturation = saturation;
    d->recompute = true;
}

double ColorTransform::saturation() const
{
    DPTR_D(const ColorTransform);
    return d->saturation;
}

void ColorTransform::setChannelDepthScale(float value, bool scaleAlpha)
{
    DPTR_D(ColorTransform);
    if (d->bpc_scale == value && d->a_bpc_scale == scaleAlpha)
        return;
    AVDebug("ColorTransform bpc_scale %f=>%f, scale alpha: %d=>%d", d->bpc_scale, value, d->a_bpc_scale, scaleAlpha);
    d->bpc_scale = value;
    d->a_bpc_scale = scaleAlpha;
    d->recompute = true;
}

NAMESPACE_END
