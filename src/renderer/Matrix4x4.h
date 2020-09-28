#ifndef MATRIX4x4_H
#define MATRIX4x4_H

#include <glad/glad.h>
#include <assert.h>

#include "RectF.h"

NAMESPACE_BEGIN

class Matrix4x4
{
public:
	Matrix4x4();
	Matrix4x4(Matrix4x4 const& matrix);
	Matrix4x4(float m11, float m12, float m13, float m14,
		float m21, float m22, float m23, float m24,
		float m31, float m32, float m33, float m34,
		float m41, float m42, float m43, float m44);
	void setToIdentity();
	void fill(float value);
	inline const float& operator()(int row, int column) const;
	inline float& operator()(int row, int column);
	Matrix4x4 inverted(bool *invertible = nullptr) const;
	void scale(GLfloat scale_x, GLfloat scale_y, GLfloat scale_z);
	void translate(GLfloat tran_x, GLfloat tran_y, GLfloat tran_z);
	void rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void ortho(const RectF& rect);
	void ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane);
	void frustum(float left, float right, float bottom, float top, float nearPlane, float farPlane);
	void perspective(float verticalAngle, float aspectRatio, float nearPlane, float farPlane);

	Matrix4x4 Matrix4x4::orthonormalInverse() const;

	friend Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2);

	inline Matrix4x4& operator *= (const Matrix4x4& other);
	inline Matrix4x4& operator*=(float factor);
	inline bool operator==(const Matrix4x4& other) const;
	inline bool operator!=(const Matrix4x4& other) const;

	inline float *data() { flagBits = General; return *m; }
	inline const float *data() const { return *m; }
	inline const float *constData() const { return *m; }

private:
	GLfloat m[4][4];
	int flagBits;           // Flag bits from the enum below.

    // When matrices are multiplied, the flag bits are or-ed together.
	enum {
		Identity = 0x0000, // Identity matrix
		Translation = 0x0001, // Contains a translation
		Scale = 0x0002, // Contains a scale
		Rotation2D = 0x0004, // Contains a rotation about the Z axis
		Rotation = 0x0008, // Contains an arbitrary rotation
		Perspective = 0x0010, // Last row is different from (0, 0, 0, 1)
		General = 0x001f  // General matrix, unknown contents
	};
	explicit Matrix4x4(int) {}
};

inline const float& Matrix4x4::operator()(int aRow, int aColumn) const
{
	//std::assert(aRow >= 0 && aRow < 4 && aColumn >= 0 && aColumn < 4);
	return m[aColumn][aRow];
}

inline float& Matrix4x4::operator()(int aRow, int aColumn)
{
	//std::assert(aRow >= 0 && aRow < 4 && aColumn >= 0 && aColumn < 4);
	return m[aColumn][aRow];
}

inline Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2)
{
	int flagBits = m1.flagBits | m2.flagBits;
	if (flagBits < Matrix4x4::Rotation2D) {
		Matrix4x4 m = m1;
		m.m[3][0] += m.m[0][0] * m2.m[3][0];
		m.m[3][1] += m.m[1][1] * m2.m[3][1];
		m.m[3][2] += m.m[2][2] * m2.m[3][2];

		m.m[0][0] *= m2.m[0][0];
		m.m[1][1] *= m2.m[1][1];
		m.m[2][2] *= m2.m[2][2];
		m.flagBits = flagBits;
		return m;
	}

	Matrix4x4 m(1);
	m.m[0][0] = m1.m[0][0] * m2.m[0][0]
		+ m1.m[1][0] * m2.m[0][1]
		+ m1.m[2][0] * m2.m[0][2]
		+ m1.m[3][0] * m2.m[0][3];
	m.m[0][1] = m1.m[0][1] * m2.m[0][0]
		+ m1.m[1][1] * m2.m[0][1]
		+ m1.m[2][1] * m2.m[0][2]
		+ m1.m[3][1] * m2.m[0][3];
	m.m[0][2] = m1.m[0][2] * m2.m[0][0]
		+ m1.m[1][2] * m2.m[0][1]
		+ m1.m[2][2] * m2.m[0][2]
		+ m1.m[3][2] * m2.m[0][3];
	m.m[0][3] = m1.m[0][3] * m2.m[0][0]
		+ m1.m[1][3] * m2.m[0][1]
		+ m1.m[2][3] * m2.m[0][2]
		+ m1.m[3][3] * m2.m[0][3];

	m.m[1][0] = m1.m[0][0] * m2.m[1][0]
		+ m1.m[1][0] * m2.m[1][1]
		+ m1.m[2][0] * m2.m[1][2]
		+ m1.m[3][0] * m2.m[1][3];
	m.m[1][1] = m1.m[0][1] * m2.m[1][0]
		+ m1.m[1][1] * m2.m[1][1]
		+ m1.m[2][1] * m2.m[1][2]
		+ m1.m[3][1] * m2.m[1][3];
	m.m[1][2] = m1.m[0][2] * m2.m[1][0]
		+ m1.m[1][2] * m2.m[1][1]
		+ m1.m[2][2] * m2.m[1][2]
		+ m1.m[3][2] * m2.m[1][3];
	m.m[1][3] = m1.m[0][3] * m2.m[1][0]
		+ m1.m[1][3] * m2.m[1][1]
		+ m1.m[2][3] * m2.m[1][2]
		+ m1.m[3][3] * m2.m[1][3];

	m.m[2][0] = m1.m[0][0] * m2.m[2][0]
		+ m1.m[1][0] * m2.m[2][1]
		+ m1.m[2][0] * m2.m[2][2]
		+ m1.m[3][0] * m2.m[2][3];
	m.m[2][1] = m1.m[0][1] * m2.m[2][0]
		+ m1.m[1][1] * m2.m[2][1]
		+ m1.m[2][1] * m2.m[2][2]
		+ m1.m[3][1] * m2.m[2][3];
	m.m[2][2] = m1.m[0][2] * m2.m[2][0]
		+ m1.m[1][2] * m2.m[2][1]
		+ m1.m[2][2] * m2.m[2][2]
		+ m1.m[3][2] * m2.m[2][3];
	m.m[2][3] = m1.m[0][3] * m2.m[2][0]
		+ m1.m[1][3] * m2.m[2][1]
		+ m1.m[2][3] * m2.m[2][2]
		+ m1.m[3][3] * m2.m[2][3];

	m.m[3][0] = m1.m[0][0] * m2.m[3][0]
		+ m1.m[1][0] * m2.m[3][1]
		+ m1.m[2][0] * m2.m[3][2]
		+ m1.m[3][0] * m2.m[3][3];
	m.m[3][1] = m1.m[0][1] * m2.m[3][0]
		+ m1.m[1][1] * m2.m[3][1]
		+ m1.m[2][1] * m2.m[3][2]
		+ m1.m[3][1] * m2.m[3][3];
	m.m[3][2] = m1.m[0][2] * m2.m[3][0]
		+ m1.m[1][2] * m2.m[3][1]
		+ m1.m[2][2] * m2.m[3][2]
		+ m1.m[3][2] * m2.m[3][3];
	m.m[3][3] = m1.m[0][3] * m2.m[3][0]
		+ m1.m[1][3] * m2.m[3][1]
		+ m1.m[2][3] * m2.m[3][2]
		+ m1.m[3][3] * m2.m[3][3];
	m.flagBits = flagBits;
	return m;
}

inline Matrix4x4& Matrix4x4::operator*=(const Matrix4x4& o)
{
	const Matrix4x4 other = o; // prevent aliasing when &o == this ### Qt 6: take o by value
	flagBits |= other.flagBits;

	if (flagBits < Rotation2D) {
		m[3][0] += m[0][0] * other.m[3][0];
		m[3][1] += m[1][1] * other.m[3][1];
		m[3][2] += m[2][2] * other.m[3][2];

		m[0][0] *= other.m[0][0];
		m[1][1] *= other.m[1][1];
		m[2][2] *= other.m[2][2];
		return *this;
	}

	float m0, m1, m2;
	m0 = m[0][0] * other.m[0][0]
		+ m[1][0] * other.m[0][1]
		+ m[2][0] * other.m[0][2]
		+ m[3][0] * other.m[0][3];
	m1 = m[0][0] * other.m[1][0]
		+ m[1][0] * other.m[1][1]
		+ m[2][0] * other.m[1][2]
		+ m[3][0] * other.m[1][3];
	m2 = m[0][0] * other.m[2][0]
		+ m[1][0] * other.m[2][1]
		+ m[2][0] * other.m[2][2]
		+ m[3][0] * other.m[2][3];
	m[3][0] = m[0][0] * other.m[3][0]
		+ m[1][0] * other.m[3][1]
		+ m[2][0] * other.m[3][2]
		+ m[3][0] * other.m[3][3];
	m[0][0] = m0;
	m[1][0] = m1;
	m[2][0] = m2;

	m0 = m[0][1] * other.m[0][0]
		+ m[1][1] * other.m[0][1]
		+ m[2][1] * other.m[0][2]
		+ m[3][1] * other.m[0][3];
	m1 = m[0][1] * other.m[1][0]
		+ m[1][1] * other.m[1][1]
		+ m[2][1] * other.m[1][2]
		+ m[3][1] * other.m[1][3];
	m2 = m[0][1] * other.m[2][0]
		+ m[1][1] * other.m[2][1]
		+ m[2][1] * other.m[2][2]
		+ m[3][1] * other.m[2][3];
	m[3][1] = m[0][1] * other.m[3][0]
		+ m[1][1] * other.m[3][1]
		+ m[2][1] * other.m[3][2]
		+ m[3][1] * other.m[3][3];
	m[0][1] = m0;
	m[1][1] = m1;
	m[2][1] = m2;

	m0 = m[0][2] * other.m[0][0]
		+ m[1][2] * other.m[0][1]
		+ m[2][2] * other.m[0][2]
		+ m[3][2] * other.m[0][3];
	m1 = m[0][2] * other.m[1][0]
		+ m[1][2] * other.m[1][1]
		+ m[2][2] * other.m[1][2]
		+ m[3][2] * other.m[1][3];
	m2 = m[0][2] * other.m[2][0]
		+ m[1][2] * other.m[2][1]
		+ m[2][2] * other.m[2][2]
		+ m[3][2] * other.m[2][3];
	m[3][2] = m[0][2] * other.m[3][0]
		+ m[1][2] * other.m[3][1]
		+ m[2][2] * other.m[3][2]
		+ m[3][2] * other.m[3][3];
	m[0][2] = m0;
	m[1][2] = m1;
	m[2][2] = m2;

	m0 = m[0][3] * other.m[0][0]
		+ m[1][3] * other.m[0][1]
		+ m[2][3] * other.m[0][2]
		+ m[3][3] * other.m[0][3];
	m1 = m[0][3] * other.m[1][0]
		+ m[1][3] * other.m[1][1]
		+ m[2][3] * other.m[1][2]
		+ m[3][3] * other.m[1][3];
	m2 = m[0][3] * other.m[2][0]
		+ m[1][3] * other.m[2][1]
		+ m[2][3] * other.m[2][2]
		+ m[3][3] * other.m[2][3];
	m[3][3] = m[0][3] * other.m[3][0]
		+ m[1][3] * other.m[3][1]
		+ m[2][3] * other.m[3][2]
		+ m[3][3] * other.m[3][3];
	m[0][3] = m0;
	m[1][3] = m1;
	m[2][3] = m2;
	return *this;
}

inline Matrix4x4& Matrix4x4::operator*=(float factor)
{
	m[0][0] *= factor;
	m[0][1] *= factor;
	m[0][2] *= factor;
	m[0][3] *= factor;
	m[1][0] *= factor;
	m[1][1] *= factor;
	m[1][2] *= factor;
	m[1][3] *= factor;
	m[2][0] *= factor;
	m[2][1] *= factor;
	m[2][2] *= factor;
	m[2][3] *= factor;
	m[3][0] *= factor;
	m[3][1] *= factor;
	m[3][2] *= factor;
	m[3][3] *= factor;
	flagBits = General;
	return *this;
}

inline bool Matrix4x4::operator==(const Matrix4x4& other) const
{
	return m[0][0] == other.m[0][0] &&
		m[0][1] == other.m[0][1] &&
		m[0][2] == other.m[0][2] &&
		m[0][3] == other.m[0][3] &&
		m[1][0] == other.m[1][0] &&
		m[1][1] == other.m[1][1] &&
		m[1][2] == other.m[1][2] &&
		m[1][3] == other.m[1][3] &&
		m[2][0] == other.m[2][0] &&
		m[2][1] == other.m[2][1] &&
		m[2][2] == other.m[2][2] &&
		m[2][3] == other.m[2][3] &&
		m[3][0] == other.m[3][0] &&
		m[3][1] == other.m[3][1] &&
		m[3][2] == other.m[3][2] &&
		m[3][3] == other.m[3][3];
}

inline bool Matrix4x4::operator!=(const Matrix4x4& other) const
{
	return m[0][0] != other.m[0][0] ||
		m[0][1] != other.m[0][1] ||
		m[0][2] != other.m[0][2] ||
		m[0][3] != other.m[0][3] ||
		m[1][0] != other.m[1][0] ||
		m[1][1] != other.m[1][1] ||
		m[1][2] != other.m[1][2] ||
		m[1][3] != other.m[1][3] ||
		m[2][0] != other.m[2][0] ||
		m[2][1] != other.m[2][1] ||
		m[2][2] != other.m[2][2] ||
		m[2][3] != other.m[2][3] ||
		m[3][0] != other.m[3][0] ||
		m[3][1] != other.m[3][1] ||
		m[3][2] != other.m[3][2] ||
		m[3][3] != other.m[3][3];
}

NAMESPACE_END
#endif // MATRIX4x4_H
