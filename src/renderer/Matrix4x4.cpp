/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "Matrix4x4.h"
#include "sdk/global.h"
#include "util/innermath.h"

#include <limits>
#include <string.h>
#include "innermath.h"

NAMESPACE_BEGIN

static float degreesToRadians(float degrees)
{
	return degrees * float(M_PI / 180);
}

static inline double matrixDet2(const double m[4][4], int col0, int col1, int row0, int row1)
{
	return m[col0][row0] * m[col1][row1] - m[col0][row1] * m[col1][row0];
}

// The 4x4 matrix inverse algorithm is based on that described at:
// http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q24
// Some optimization has been done to avoid making copies of 3x3
// sub-matrices and to unroll the loops.

// Calculate the determinant of a 3x3 sub-matrix.
//     | A B C |
// M = | D E F |   det(M) = A * (EI - HF) - B * (DI - GF) + C * (DH - GE)
//     | G H I |
static inline double matrixDet3
(const double m[4][4], int col0, int col1, int col2,
	int row0, int row1, int row2)
{
	return m[col0][row0] * matrixDet2(m, col1, col2, row1, row2)
		- m[col1][row0] * matrixDet2(m, col0, col2, row1, row2)
		+ m[col2][row0] * matrixDet2(m, col0, col1, row1, row2);
}

// Calculate the determinant of a 4x4 matrix.
static inline double matrixDet4(const double m[4][4])
{
	double det;
	det = m[0][0] * matrixDet3(m, 1, 2, 3, 1, 2, 3);
	det -= m[1][0] * matrixDet3(m, 0, 2, 3, 1, 2, 3);
	det += m[2][0] * matrixDet3(m, 0, 1, 3, 1, 2, 3);
	det -= m[3][0] * matrixDet3(m, 0, 1, 2, 1, 2, 3);
	return det;
}

static inline void copyToDoubles(const float m[4][4], double mm[4][4])
{
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			mm[i][j] = double(m[i][j]);
}

Matrix4x4::Matrix4x4()
{
	setToIdentity();
}

void Matrix4x4::setToIdentity()
{
	m[0][0] = 1.0f;
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = 0.0f;
	m[1][0] = 0.0f;
	m[1][1] = 1.0f;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = 1.0f;
	m[2][3] = 0.0f;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
	flagBits = General;
}

void Matrix4x4::fill(float value)
{
	m[0][0] = value;
	m[0][1] = value;
	m[0][2] = value;
	m[0][3] = value;
	m[1][0] = value;
	m[1][1] = value;
	m[1][2] = value;
	m[1][3] = value;
	m[2][0] = value;
	m[2][1] = value;
	m[2][2] = value;
	m[2][3] = value;
	m[3][0] = value;
	m[3][1] = value;
	m[3][2] = value;
	m[3][3] = value;
	flagBits = General;
}

Matrix4x4::Matrix4x4(Matrix4x4 const& matrix)
{
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			m[i][j] = matrix.m[i][j];
	flagBits = General;
}


Matrix4x4::Matrix4x4(float m11, float m12, float m13, float m14,
	float m21, float m22, float m23, float m24,
	float m31, float m32, float m33, float m34,
	float m41, float m42, float m43, float m44)
{
	m[0][0] = m11; m[0][1] = m21; m[0][2] = m31; m[0][3] = m41;
	m[1][0] = m12; m[1][1] = m22; m[1][2] = m32; m[1][3] = m42;
	m[2][0] = m13; m[2][1] = m23; m[2][2] = m33; m[2][3] = m43;
	m[3][0] = m14; m[3][1] = m24; m[3][2] = m34; m[3][3] = m44;
	flagBits = General;
}

void Matrix4x4::scale(GLfloat scale_x, GLfloat scale_y, GLfloat scale_z)
{
	GLfloat scales[3] = { scale_x, scale_y, scale_z };

	for (int i = 0; i < 3; ++i)
	{
		m[i][0] *= scales[i];
		m[i][1] *= scales[i];
		m[i][2] *= scales[i];
		m[i][3] *= scales[i];
	}
	flagBits = General;
}

Matrix4x4 Matrix4x4::inverted(bool * invertible) const
{
	// Handle some of the easy cases first.
	if (flagBits == Identity) {
		if (invertible)
			*invertible = true;
		return Matrix4x4();
	}
	else if (flagBits == Translation) {
		Matrix4x4 inv;
		inv.m[3][0] = -m[3][0];
		inv.m[3][1] = -m[3][1];
		inv.m[3][2] = -m[3][2];
		inv.flagBits = Translation;
		if (invertible)
			*invertible = true;
		return inv;
	}
	else if (flagBits < Rotation2D) {
		// Translation | Scale
		if (m[0][0] == 0 || m[1][1] == 0 || m[2][2] == 0) {
			if (invertible)
				*invertible = false;
			return Matrix4x4();
		}
		Matrix4x4 inv;
		inv.m[0][0] = 1.0f / m[0][0];
		inv.m[1][1] = 1.0f / m[1][1];
		inv.m[2][2] = 1.0f / m[2][2];
		inv.m[3][0] = -m[3][0] * inv.m[0][0];
		inv.m[3][1] = -m[3][1] * inv.m[1][1];
		inv.m[3][2] = -m[3][2] * inv.m[2][2];
		inv.flagBits = flagBits;

		if (invertible)
			*invertible = true;
		return inv;
	}
	else if ((flagBits & ~(Translation | Rotation2D | Rotation)) == Identity) {
		if (invertible)
			*invertible = true;
		return orthonormalInverse();
	}
	else if (flagBits < Perspective) {
		Matrix4x4 inv(1); // The "1" says to not load the identity.

		double mm[4][4];
		copyToDoubles(m, mm);

		double det = matrixDet3(mm, 0, 1, 2, 0, 1, 2);
		if (det == 0.0f) {
			if (invertible)
				*invertible = false;
			return Matrix4x4();
		}
		det = 1.0f / det;

		inv.m[0][0] = matrixDet2(mm, 1, 2, 1, 2) * det;
		inv.m[0][1] = -matrixDet2(mm, 0, 2, 1, 2) * det;
		inv.m[0][2] = matrixDet2(mm, 0, 1, 1, 2) * det;
		inv.m[0][3] = 0;
		inv.m[1][0] = -matrixDet2(mm, 1, 2, 0, 2) * det;
		inv.m[1][1] = matrixDet2(mm, 0, 2, 0, 2) * det;
		inv.m[1][2] = -matrixDet2(mm, 0, 1, 0, 2) * det;
		inv.m[1][3] = 0;
		inv.m[2][0] = matrixDet2(mm, 1, 2, 0, 1) * det;
		inv.m[2][1] = -matrixDet2(mm, 0, 2, 0, 1) * det;
		inv.m[2][2] = matrixDet2(mm, 0, 1, 0, 1) * det;
		inv.m[2][3] = 0;
		inv.m[3][0] = -inv.m[0][0] * m[3][0] - inv.m[1][0] * m[3][1] - inv.m[2][0] * m[3][2];
		inv.m[3][1] = -inv.m[0][1] * m[3][0] - inv.m[1][1] * m[3][1] - inv.m[2][1] * m[3][2];
		inv.m[3][2] = -inv.m[0][2] * m[3][0] - inv.m[1][2] * m[3][1] - inv.m[2][2] * m[3][2];
		inv.m[3][3] = 1;
		inv.flagBits = flagBits;

		if (invertible)
			*invertible = true;
		return inv;
	}

	Matrix4x4 inv(1); // The "1" says to not load the identity.

	double mm[4][4];
	copyToDoubles(m, mm);

	double det = matrixDet4(mm);
	if (det == 0.0f) {
		if (invertible)
			*invertible = false;
		return Matrix4x4();
	}
	det = 1.0f / det;

	inv.m[0][0] = matrixDet3(mm, 1, 2, 3, 1, 2, 3) * det;
	inv.m[0][1] = -matrixDet3(mm, 0, 2, 3, 1, 2, 3) * det;
	inv.m[0][2] = matrixDet3(mm, 0, 1, 3, 1, 2, 3) * det;
	inv.m[0][3] = -matrixDet3(mm, 0, 1, 2, 1, 2, 3) * det;
	inv.m[1][0] = -matrixDet3(mm, 1, 2, 3, 0, 2, 3) * det;
	inv.m[1][1] = matrixDet3(mm, 0, 2, 3, 0, 2, 3) * det;
	inv.m[1][2] = -matrixDet3(mm, 0, 1, 3, 0, 2, 3) * det;
	inv.m[1][3] = matrixDet3(mm, 0, 1, 2, 0, 2, 3) * det;
	inv.m[2][0] = matrixDet3(mm, 1, 2, 3, 0, 1, 3) * det;
	inv.m[2][1] = -matrixDet3(mm, 0, 2, 3, 0, 1, 3) * det;
	inv.m[2][2] = matrixDet3(mm, 0, 1, 3, 0, 1, 3) * det;
	inv.m[2][3] = -matrixDet3(mm, 0, 1, 2, 0, 1, 3) * det;
	inv.m[3][0] = -matrixDet3(mm, 1, 2, 3, 0, 1, 2) * det;
	inv.m[3][1] = matrixDet3(mm, 0, 2, 3, 0, 1, 2) * det;
	inv.m[3][2] = -matrixDet3(mm, 0, 1, 3, 0, 1, 2) * det;
	inv.m[3][3] = matrixDet3(mm, 0, 1, 2, 0, 1, 2) * det;
	inv.flagBits = flagBits;

	if (invertible)
		*invertible = true;
	return inv;
}

void Matrix4x4::translate(GLfloat tran_x, GLfloat tran_y, GLfloat tran_z)
{
	for (int i = 0; i < 4; ++i)
		m[3][i] += (m[0][i] * tran_x +
			m[1][i] * tran_y +
			m[2][i] * tran_z);

}

void Matrix4x4::rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	if (angle == 0.0f)
		return;
	float c, s;
	if (angle == 90.0f || angle == -270.0f) {
		s = 1.0f;
		c = 0.0f;
	}
	else if (angle == -90.0f || angle == 270.0f) {
		s = -1.0f;
		c = 0.0f;
	}
	else if (angle == 180.0f || angle == -180.0f) {
		s = 0.0f;
		c = -1.0f;
	}
	else {
		float a = degreesToRadians(angle);
		c = std::cos(a);
		s = std::sin(a);
	}
	if (x == 0.0f) {
		if (y == 0.0f) {
			if (z != 0.0f) {
				// Rotate around the Z axis.
				if (z < 0)
					s = -s;
				float tmp;
				m[0][0] = (tmp = m[0][0]) * c + m[1][0] * s;
				m[1][0] = m[1][0] * c - tmp * s;
				m[0][1] = (tmp = m[0][1]) * c + m[1][1] * s;
				m[1][1] = m[1][1] * c - tmp * s;
				m[0][2] = (tmp = m[0][2]) * c + m[1][2] * s;
				m[1][2] = m[1][2] * c - tmp * s;
				m[0][3] = (tmp = m[0][3]) * c + m[1][3] * s;
				m[1][3] = m[1][3] * c - tmp * s;

				flagBits |= Rotation2D;
				return;
			}
		}
		else if (z == 0.0f) {
			// Rotate around the Y axis.
			if (y < 0)
				s = -s;
			float tmp;
			m[2][0] = (tmp = m[2][0]) * c + m[0][0] * s;
			m[0][0] = m[0][0] * c - tmp * s;
			m[2][1] = (tmp = m[2][1]) * c + m[0][1] * s;
			m[0][1] = m[0][1] * c - tmp * s;
			m[2][2] = (tmp = m[2][2]) * c + m[0][2] * s;
			m[0][2] = m[0][2] * c - tmp * s;
			m[2][3] = (tmp = m[2][3]) * c + m[0][3] * s;
			m[0][3] = m[0][3] * c - tmp * s;

			flagBits |= Rotation;
			return;
		}
	}
	else if (y == 0.0f && z == 0.0f) {
		// Rotate around the X axis.
		if (x < 0)
			s = -s;
		float tmp;
		m[1][0] = (tmp = m[1][0]) * c + m[2][0] * s;
		m[2][0] = m[2][0] * c - tmp * s;
		m[1][1] = (tmp = m[1][1]) * c + m[2][1] * s;
		m[2][1] = m[2][1] * c - tmp * s;
		m[1][2] = (tmp = m[1][2]) * c + m[2][2] * s;
		m[2][2] = m[2][2] * c - tmp * s;
		m[1][3] = (tmp = m[1][3]) * c + m[2][3] * s;
		m[2][3] = m[2][3] * c - tmp * s;

		flagBits |= Rotation;
		return;
	}

	double len = double(x) * double(x) +
		double(y) * double(y) +
		double(z) * double(z);
	if (!FuzzyCompare(len, 1.0) && !FuzzyIsNull(len)) {
		len = std::sqrt(len);
		x = float(double(x) / len);
		y = float(double(y) / len);
		z = float(double(z) / len);
	}
	float ic = 1.0f - c;
	Matrix4x4 rot(1); // The "1" says to not load the identity.
	rot.m[0][0] = x * x * ic + c;
	rot.m[1][0] = x * y * ic - z * s;
	rot.m[2][0] = x * z * ic + y * s;
	rot.m[3][0] = 0.0f;
	rot.m[0][1] = y * x * ic + z * s;
	rot.m[1][1] = y * y * ic + c;
	rot.m[2][1] = y * z * ic - x * s;
	rot.m[3][1] = 0.0f;
	rot.m[0][2] = x * z * ic - y * s;
	rot.m[1][2] = y * z * ic + x * s;
	rot.m[2][2] = z * z * ic + c;
	rot.m[3][2] = 0.0f;
	rot.m[0][3] = 0.0f;
	rot.m[1][3] = 0.0f;
	rot.m[2][3] = 0.0f;
	rot.m[3][3] = 1.0f;
	rot.flagBits = Rotation;
	*this *= rot;
}

void Matrix4x4::ortho(const RectF& rect)
{
	ortho(rect.left(), rect.right(), rect.bottom(), rect.top(), -1.0f, 1.0f);
}

/*!
	Multiplies this matrix by another that applies an orthographic
	projection for a window with lower-left corner (\a left, \a bottom),
	upper-right corner (\a right, \a top), and the specified \a nearPlane
	and \a farPlane clipping planes.

	\sa frustum(), perspective()
*/
void Matrix4x4::ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
	// Bail out if the projection volume is zero-sized.
	if (left == right || bottom == top || nearPlane == farPlane)
		return;

	// Construct the projection.
	float width = right - left;
	float invheight = top - bottom;
	float clip = farPlane - nearPlane;
	Matrix4x4 m(1);
	m.m[0][0] = 2.0f / width;
	m.m[1][0] = 0.0f;
	m.m[2][0] = 0.0f;
	m.m[3][0] = -(left + right) / width;
	m.m[0][1] = 0.0f;
	m.m[1][1] = 2.0f / invheight;
	m.m[2][1] = 0.0f;
	m.m[3][1] = -(top + bottom) / invheight;
	m.m[0][2] = 0.0f;
	m.m[1][2] = 0.0f;
	m.m[2][2] = -2.0f / clip;
	m.m[3][2] = -(nearPlane + farPlane) / clip;
	m.m[0][3] = 0.0f;
	m.m[1][3] = 0.0f;
	m.m[2][3] = 0.0f;
	m.m[3][3] = 1.0f;
	m.flagBits = Translation | Scale;

	// Apply the projection.
	*this *= m;
}

/*!
	Multiplies this matrix by another that applies a perspective
	frustum projection for a window with lower-left corner (\a left, \a bottom),
	upper-right corner (\a right, \a top), and the specified \a nearPlane
	and \a farPlane clipping planes.

	\sa ortho(), perspective()
*/
void Matrix4x4::frustum(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
	// Bail out if the projection volume is zero-sized.
	if (left == right || bottom == top || nearPlane == farPlane)
		return;

	// Construct the projection.
	Matrix4x4 m(1);
	float width = right - left;
	float invheight = top - bottom;
	float clip = farPlane - nearPlane;
	m.m[0][0] = 2.0f * nearPlane / width;
	m.m[1][0] = 0.0f;
	m.m[2][0] = (left + right) / width;
	m.m[3][0] = 0.0f;
	m.m[0][1] = 0.0f;
	m.m[1][1] = 2.0f * nearPlane / invheight;
	m.m[2][1] = (top + bottom) / invheight;
	m.m[3][1] = 0.0f;
	m.m[0][2] = 0.0f;
	m.m[1][2] = 0.0f;
	m.m[2][2] = -(nearPlane + farPlane) / clip;
	m.m[3][2] = -2.0f * nearPlane * farPlane / clip;
	m.m[0][3] = 0.0f;
	m.m[1][3] = 0.0f;
	m.m[2][3] = -1.0f;
	m.m[3][3] = 0.0f;
	m.flagBits = General;

	// Apply the projection.
	*this *= m;
}

/*!
	Multiplies this matrix by another that applies a perspective
	projection. The vertical field of view will be \a verticalAngle degrees
	within a window with a given \a aspectRatio that determines the horizontal
	field of view.
	The projection will have the specified \a nearPlane and \a farPlane clipping
	planes which are the distances from the viewer to the corresponding planes.

	\sa ortho(), frustum()
*/
void Matrix4x4::perspective(float verticalAngle, float aspectRatio, float nearPlane, float farPlane)
{
	// Bail out if the projection volume is zero-sized.
	if (nearPlane == farPlane || aspectRatio == 0.0f)
		return;

	// Construct the projection.
	Matrix4x4 m(1);
	float radians = degreesToRadians(verticalAngle / 2.0f);
	float sine = std::sin(radians);
	if (sine == 0.0f)
		return;
	float cotan = std::cos(radians) / sine;
	float clip = farPlane - nearPlane;
	m.m[0][0] = cotan / aspectRatio;
	m.m[1][0] = 0.0f;
	m.m[2][0] = 0.0f;
	m.m[3][0] = 0.0f;
	m.m[0][1] = 0.0f;
	m.m[1][1] = cotan;
	m.m[2][1] = 0.0f;
	m.m[3][1] = 0.0f;
	m.m[0][2] = 0.0f;
	m.m[1][2] = 0.0f;
	m.m[2][2] = -(nearPlane + farPlane) / clip;
	m.m[3][2] = -(2.0f * nearPlane * farPlane) / clip;
	m.m[0][3] = 0.0f;
	m.m[1][3] = 0.0f;
	m.m[2][3] = -1.0f;
	m.m[3][3] = 0.0f;
	m.flagBits = General;

	// Apply the projection.
	*this *= m;
}

// Helper routine for inverting orthonormal matrices that consist
// of just rotations and translations.
Matrix4x4 Matrix4x4::orthonormalInverse() const
{
	Matrix4x4 result(1);  // The '1' says not to load identity

	result.m[0][0] = m[0][0];
	result.m[1][0] = m[0][1];
	result.m[2][0] = m[0][2];

	result.m[0][1] = m[1][0];
	result.m[1][1] = m[1][1];
	result.m[2][1] = m[1][2];

	result.m[0][2] = m[2][0];
	result.m[1][2] = m[2][1];
	result.m[2][2] = m[2][2];

	result.m[0][3] = 0.0f;
	result.m[1][3] = 0.0f;
	result.m[2][3] = 0.0f;

	result.m[3][0] = -(result.m[0][0] * m[3][0] + result.m[1][0] * m[3][1] + result.m[2][0] * m[3][2]);
	result.m[3][1] = -(result.m[0][1] * m[3][0] + result.m[1][1] * m[3][1] + result.m[2][1] * m[3][2]);
	result.m[3][2] = -(result.m[0][2] * m[3][0] + result.m[1][2] * m[3][1] + result.m[2][2] * m[3][2]);
	result.m[3][3] = 1.0f;

	result.flagBits = flagBits;

	return result;
}

NAMESPACE_END
