#ifndef NECROANIMSTATE_H
#define NECROANIMSTATE_H

#include <string>
#include "Image.h"

namespace NECRO
{
namespace Client
{
	//-------------------------------------------------
	// A single state inside an Animator
	//-------------------------------------------------
	class AnimState
	{
	public:
		AnimState(std::string pName, Image* pImg, float pSpeed);

	private:
		std::string name;
		Image* img;

		float speed = 75;

	public:
		Image* GetImg() const;
		float	GetSpeed() const;
	};

	inline Image* AnimState::GetImg() const
	{
		return img;
	}

	inline float AnimState::GetSpeed() const
	{
		return speed;
	}

}
}

#endif
