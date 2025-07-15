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
		std::string m_name;
		Image*		m_img;

		float m_speed = 75;

	public:
		Image*	GetImg() const;
		float	GetSpeed() const;
	};

	inline Image* AnimState::GetImg() const
	{
		return m_img;
	}

	inline float AnimState::GetSpeed() const
	{
		return m_speed;
	}

}
}

#endif
