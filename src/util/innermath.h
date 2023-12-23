#ifndef INNERMATH_H
#define INNERMATH_H

#include <algorithm>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265
#endif
#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923   // pi/2
#endif

// Definitions of useful mathematical constants
//
// Define _USE_MATH_DEFINES before including <math.h> to expose these macro
// definitions for common math constants.  These are placed under an #ifdef
// since these commonly-defined names are not part of the C or C++ standards
//#define M_E        2.71828182845904523536   // e
//#define M_LOG2E    1.44269504088896340736   // log2(e)
//#define M_LOG10E   0.434294481903251827651  // log10(e)
//#define M_LN2      0.693147180559945309417  // ln(2)
//#define M_LN10     2.30258509299404568402   // ln(10)
//#define M_PI       3.14159265358979323846   // pi
//#define M_PI_2     1.57079632679489661923   // pi/2
//#define M_PI_4     0.785398163397448309616  // pi/4
//#define M_1_PI     0.318309886183790671538  // 1/pi
//#define M_2_PI     0.636619772367581343076  // 2/pi
//#define M_2_SQRTPI 1.12837916709551257390   // 2/sqrt(pi)
//#define M_SQRT2    1.41421356237309504880   // sqrt(2)

static inline bool FuzzyCompare(double p1, double p2)
{
	return (std::abs(p1 - p2) * 1000000000000. <= std::min(std::abs(p1), std::abs(p2)));
}

static inline bool FuzzyCompare(float p1, float p2)
{
	return (std::abs(p1 - p2) * 100000.f <= std::min(std::abs(p1), std::abs(p2)));
}

static inline bool FuzzyIsNull(double d)
{
	return std::abs(d) <= 0.000000000001;
}

static inline bool FuzzyIsNull(float f)
{
	return std::abs(f) <= 0.00001f;
}

static inline bool isNullF(float f)
{
	union U {
		float f;
		unsigned int u;
	};
	U val;
	val.f = f;
	return (val.u & 0x7fffffff) == 0;
}

#endif // INNERMATH_H
