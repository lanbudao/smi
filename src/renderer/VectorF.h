#ifndef VECTORF_H
#define VECTORF_H

#include "innermath.h"
#include "global.h"
NAMESPACE_BEGIN

class Vector2D
{
public:
	Vector2D() {v[0] = 0.0; v[1] = 0.0;}
	Vector2D(float x_, float y_) { v[0] = x_; v[1] = y_; }

	inline float x() const { return v[0]; }
	inline float y() const { return v[1]; }
	inline void setX(float aX);
	inline void setY(float aY);

	inline bool isNull() const;

	inline Vector2D & operator+=(const Vector2D &vector);
	inline Vector2D & operator-=(const Vector2D &vector);
	inline Vector2D & operator*=(float factor);
	inline Vector2D & operator*=(const Vector2D &vector);
	inline Vector2D & operator/=(float divisor);
	inline Vector2D & operator/=(const Vector2D &vector);

	friend inline const Vector2D operator*(const Vector2D &vector, float factor);
	friend inline const Vector2D operator/(const Vector2D &vector, float divisor);

private:
	float v[2];
};

inline void Vector2D::setX(float aX) { v[0] = aX; }
inline void Vector2D::setY(float aY) { v[1] = aY; }
inline bool Vector2D::isNull() const
{
	return isNullF(v[0]) && isNullF(v[1]);
}

inline Vector2D &Vector2D::operator+=(const Vector2D &vector)
{
	v[0] += vector.v[0];
	v[1] += vector.v[1];
	return *this;
}

inline Vector2D &Vector2D::operator-=(const Vector2D &vector)
{
	v[0] -= vector.v[0];
	v[1] -= vector.v[1];
	return *this;
}

inline Vector2D &Vector2D::operator*=(float factor)
{
	v[0] *= factor;
	v[1] *= factor;
	return *this;
}

inline Vector2D &Vector2D::operator*=(const Vector2D &vector)
{
	v[0] *= vector.v[0];
	v[1] *= vector.v[1];
	return *this;
}

inline Vector2D &Vector2D::operator/=(float divisor)
{
	v[0] /= divisor;
	v[1] /= divisor;
	return *this;
}

inline Vector2D &Vector2D::operator/=(const Vector2D &vector)
{
	v[0] /= vector.v[0];
	v[1] /= vector.v[1];
	return *this;
}

inline const Vector2D operator*(const Vector2D &vector, float factor)
{
	return Vector2D(vector.v[0] * factor, vector.v[1] * factor);
}

inline const Vector2D operator/(const Vector2D &vector, float divisor)
{
	return Vector2D(vector.v[0] / divisor, vector.v[1] / divisor);
}

NAMESPACE_END
#endif //VECTORF_H
