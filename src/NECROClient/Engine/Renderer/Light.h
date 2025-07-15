#ifndef NECROLIGHT_H
#define NECROLIGHT_H

#include "SDL.h"

#include "Vector2.h"


namespace NECRO
{
namespace Client
{
	class Entity;

	//-----------------------------------------------------------------------------------------------------------------
	// A light is attached to an Entity (or classes derived from Entity)
	// 
	// Lighting works this way: each World has a base color and intensity, each Cell in the world has its own
	// color and intensity (initially derived from the world) that defines the color of the entities in it. 
	// 
	// Each light modify the Cells' color and intensity in their radius.
	//-----------------------------------------------------------------------------------------------------------------
	class Light
	{
	public:
		enum class PropagationSetting
		{
			Flat = 0,
			Raycast = 1
		};

	public:
		Entity*		m_owner;
					
		bool		m_enabled = true;
					
		Vector2		m_pos;
		float		m_intensity;
		float		m_radius;
		float		m_dropoffMultiplier; // how much reduction of light there is for cells that are far from the source
		float		m_farDropoffThreshold; // the dropoff distance (in cells) from which the lightFarDropoffMultiplier is applied on top of the base dropoff
		float		m_farDropoffMultiplier;
		SDL_Color	m_color;

		// Anim parameters
		float m_animSpeed;
		float m_minIntensity;
		float m_maxIntensity;
		float m_minIntensityDivider;

	private:
		PropagationSetting m_curPropagation;

		// Settings to animate lights
		bool m_doAnim = false;

		// For pulse lighting effect
		bool m_goUp = false;
		bool m_goDown = true;

	private:
		void Animate();
		void PropagateLight();

		// Different kinds of light propagation
		void RaycastLightPropagation();
		void FlatLightPropagation();

	public:
		void Init(Entity* owner);
		void Update();

		void SetAnim(bool v);

		void SetPropagationSetting(PropagationSetting s);
	};

	inline void Light::SetPropagationSetting(PropagationSetting s)
	{
		m_curPropagation = s;
	}

}
}

#endif
