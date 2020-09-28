#ifndef INNERMATH_H
#define INNERMATH_H

#include <algorithm>
#include <corecrt_math_defines.h>
#include <cmath>

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
