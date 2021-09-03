/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "types_sip.h"
#include <cmath>
#include <string>
#include "stream.h"

namespace	SIPBASE
{

class IStream;

// ======================================================================================================
/**
 * A 3D vector of float.
 * \date 2000
 */
class CVector
{
public:		// Attributes.
	float	x,y,z;

public:		// const.
	/// Null vector (0,0,0).
	static const	CVector		Null;
	/// I vector (1,0,0).
	static const	CVector		I;
	/// J vector (0,1,0).
	static const	CVector		J;
	/// K vector (0,0,1).
	static const	CVector		K;

public:		// Methods.
	/// @name Object.
	//@{
	/// Constructor wich do nothing.
	CVector() {}
	/// Constructor .
	CVector(float	_x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	/// Copy Constructor.
	CVector(const CVector &v) : x(v.x), y(v.y), z(v.z) {}
	//@}

	/// @name Base Maths.
	//@{
	CVector	&operator+=(const CVector &v);
	CVector	&operator-=(const CVector &v);
	CVector	&operator*=(float f);
	CVector	&operator/=(float f);
	CVector	operator+(const CVector &v) const;
	CVector	operator-(const CVector &v) const;
	CVector	operator*(float f) const;
	CVector	operator/(float f) const;
	CVector	operator-() const;
	//@}

	/// @name Advanced Maths.
	//@{
	/// Dot product.
	float	operator*(const CVector &v) const;
	/** Cross product.
	 * compute the cross product *this ^ v.
	 */
	CVector	operator^(const CVector &v) const;
	/// Return the norm of the vector.
	float	norm() const;
	/// Return the square of the norm of the vector.
	float	sqrnorm() const;
	/// Normalize the vector.
	void	normalize();
	/// Return the vector normalized.
	CVector	normed() const;
	//@}

	/// @name Misc.
	//@{
	void	set(float _x, float _y, float _z);
	bool	operator==(const CVector &v) const;
	bool	operator!=(const CVector &v) const;
	bool	isNull() const;
	/// This operator is here just for map/set insertion (no meaning). comparison order is x,y,z.
	bool	operator<(const CVector &v) const;
	/** 
	 * Setup the vector with spheric coordinates.
	 * sphericToCartesian(1,0,0) build the I vector  ((1,0,0)).
	 * the formula is:  \n
	 * x= r*cos(theta)*cos(phi) \n
	 * y= r*sin(theta)*cos(phi) \n
	 * z= r*sin(phi) \n
	 * \sa cartesianToSpheric()
	 */
	void	sphericToCartesian(float r, float theta,float phi);
	CVector crossProduct( const CVector& rkVector ) const
	{
		return CVector(
			y * rkVector.z - z * rkVector.y,
			z * rkVector.x - x * rkVector.z,
			x * rkVector.y - y * rkVector.x);

	}
	inline float length () const
	{
		return sqrt( x * x + y * y + z * z );
	}
	inline CVector& operator /= ( const CVector& rkVector )
	{
		x /= rkVector.x;
		y /= rkVector.y;
		z /= rkVector.z;

		return *this;
	}


	/**
	 * Get the sphreic coordinates of the vector.
	 * See sphericToCartesian() to know coordinates conventions.
	 * \sa sphericToCartesian()
	 */
	void	cartesianToSpheric(float &r, float &theta,float &phi) const;
	/// Set all vector x/y/z as minimum of a/b x/y/z  (respectively).
	void	minof(const CVector &a, const CVector &b);
	/// Set all vector x/y/z as maximum of a/b x/y/z  (respectively).
	void	maxof(const CVector &a, const CVector &b);
	/// serial.
	void	serial(IStream &f);
	//@}

	/// Returns the contents as a printable string "x y z"
	/// undeprecated, use the generic function toString()
	std::string	asString() const { return toString(); }

	/// Returns the contents as a printable string "x y z"
	std::string	toString() const;
	
	// friends.
	friend	CVector	operator*(float f, const CVector &v0);
};

// blend (faster version than the generic version found in algo.h)
inline CVector blend(const CVector &v0, const CVector &v1, float lambda)
{
	float invLambda = 1.f - lambda;
	return CVector(invLambda * v0.x + lambda * v1.x,
		           invLambda * v0.y + lambda * v1.y,
				   invLambda * v0.z + lambda * v1.z);
}

class Quaternion
{
public:
	inline Quaternion (	float fW = 1.0,	float fX = 0.0, float fY = 0.0, float fZ = 0.0)
	{
		w = fW;
		x = fX;
		y = fY;
		z = fZ;
	}
	inline Quaternion(float & rfAngle, const CVector& rkAxis)
	{
		this->FromAngleAxis(rfAngle, rkAxis);
	}
	void FromAngleAxis (const float& rfAngle, const CVector& rkAxis)
	{
		float fHalfAngle = (float)(0.5 * rfAngle);
		float fSin = sin(fHalfAngle);
		w = cos(fHalfAngle);
		x = fSin*rkAxis.x;
		y = fSin*rkAxis.y;
		z = fSin*rkAxis.z;
	}
	Quaternion operator* (const Quaternion& rkQ) const
	{
		return Quaternion
			(
			w * rkQ.w - x * rkQ.x - y * rkQ.y - z * rkQ.z,
			w * rkQ.x + x * rkQ.w + y * rkQ.z - z * rkQ.y,
			w * rkQ.y + y * rkQ.w + z * rkQ.x - x * rkQ.z,
			w * rkQ.z + z * rkQ.w + x * rkQ.y - y * rkQ.x
			);
	}
	CVector operator* (const CVector& v) const
	{
		// nVidia SDK implementation
		CVector uv, uuv;
		CVector qvec(x, y, z);
		uv = qvec.crossProduct(v);
		uuv = qvec.crossProduct(uv);
		uv *= (2.0f * w);
		uuv *= 2.0f;

		return v + uv + uuv;

	}
	Quaternion Inverse () const
	{
		float fNorm = w*w+x*x+y*y+z*z;
		if ( fNorm > 0.0 )
		{
			float fInvNorm = (float)(1.0/fNorm);
			return Quaternion(w*fInvNorm,-x*fInvNorm,-y*fInvNorm,-z*fInvNorm);
		}
		else
		{
			// return an invalid result to flag the error
			return Quaternion(0.0, 0.0, 0.0, 0.0);
		}
	}

	float w, x, y, z;

};

}


#include "vector_inline.h"


#endif // __VECTOR_H__

/* End of vector.h */
