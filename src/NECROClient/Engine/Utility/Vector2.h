#ifndef NECROVECTOR_2_H
#define NECROVECTOR_2_H

#include <math.h>

class Vector2
{
public:
	Vector2();
	Vector2(float pX, float pY);

	float x;
	float y;

	void Set(float pX, float pY);
	void Zero();
	void Clamp(float minX, float maxX, float minY, float maxY);

	void Normalize();
};

inline Vector2::Vector2() :
	x(0.0f),
	y(0.0f)
{

}

inline Vector2::Vector2(float pX, float pY) :
	x(pX),
	y(pY)
{

}

inline void Vector2::Set(float pX, float pY) 
{
	x = pX;
	y = pY;
}

inline void Vector2::Zero()
{
	x = 0;
	y = 0;
}

inline void Vector2::Normalize() 
{
	float length = sqrt((x * x) + (y * y));

	if (length != 0)
	{	
		x /= length;
		y /= length;
	}
}

inline void Vector2::Clamp(float minX, float maxX, float minY, float maxY)
{
	if (x < minX)
		x = minX;
	else if (x > maxX)
		x = maxX;

	if (y < minY)
		y = minY;
	else if (y > maxY)
		y = maxY;
}

#endif