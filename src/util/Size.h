#ifndef SIZE_H
#define SIZE_H

#include "sdk/global.h"

class FFPRO_EXPORT SizeF
{
public:
	SizeF() : width(0), height(0) {}
	SizeF(float w, float h) { width = w; height = h; }

	inline bool isNull() const { return width == 0 && height == 0; }
	inline bool isValid() const { return width >= 0 && height >= 0; }
	inline bool hasNone() const { return width == 0 || height == 0; }

	friend inline bool operator == (const SizeF &s1, const SizeF &s2) { return s1.width == s2.width && s1.height == s2.height; }
	friend inline bool operator != (const SizeF &s1, const SizeF &s2) { return s1.width != s2.width || s1.height != s2.height; }
	friend inline const SizeF operator + (const SizeF &s1, const SizeF &s2) { return SizeF(s1.width + s2.width, s1.height + s2.height); }
	friend inline const SizeF operator - (const SizeF &s1, const SizeF &s2) { return SizeF(s1.width - s2.width, s1.height - s2.height); }

	inline void transpose() noexcept { std::swap(width, height); };
	inline SizeF transposed() const noexcept { return SizeF(height, width); };

	float width, height;
};

class FFPRO_EXPORT Size
{
public:
    Size(): width(0), height(0) {}
    Size(int w, int h) {width = w; height = h;}

    inline bool isNull() const {return width == 0 && height == 0;}
    inline bool isValid() const {return width >= 0 && height >= 0;}
    inline bool hasNone() const {return width == 0 || height == 0;}

    friend inline bool operator == (const Size &s1, const Size &s2) {return s1.width == s2.width && s1.height == s2.height;}
    friend inline bool operator != (const Size &s1, const Size &s2) {return s1.width != s2.width || s1.height != s2.height;}
    friend inline const Size operator + (const Size &s1, const Size &s2) {return Size(s1.width + s2.width, s1.height + s2.height);}
    friend inline const Size operator - (const Size &s1, const Size &s2) {return Size(s1.width - s2.width, s1.height - s2.height);}

	inline void transpose() noexcept { std::swap(width, height); };
	inline Size transposed() const noexcept { return Size(height, width); };

	SizeF toSizeF() { return SizeF((float)width, (float)height); }

    int width, height;
};

#endif //SIZE_H
