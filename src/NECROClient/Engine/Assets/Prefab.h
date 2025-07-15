#ifndef NECROPREFAB_H
#define NECROPREFAB_H

#include <string>
#include <memory>

#include "SDL.h"
#include "Entity.h"


namespace NECRO
{
namespace Client
{
	// Structs to group array-based Prefabs data, like Interactables
	struct InteractableData
	{
		int interactType;
		std::string parStr;
		float parFloat1;
		float parFloat2;
	};

	class Prefab
	{
	private:
		std::string		m_pName;			// name of the prefab
		std::string		m_pImgFile;		// name of the img associated with this prefab
		bool			m_toRender;		// false for invisible objects
		bool			m_isStatic;		// is it an unmovable object?
		Vector2			m_posOffset;		// position offset when instantiated in a cell
						
		bool			m_hasCollider;	// has a collider component
		SDL_Rect		m_collRect;
		int				m_collOffsetX;
		int				m_collOffsetY;
						
		bool			m_occlCheck;				// If true, the prefab can occlude the player so we can render this prefab transparent
		int				m_occlModX;				// X and Y offsets for the occlusion rect that will be tested against the player/entities that needs to be visible all the time
		int				m_occlModY;
						
		bool			m_blocksLight;			// If true, lights that hits this prefab will block the propagation by blocksLightValue
		float			m_blocksLightValue;
						
		bool			m_emitsLight;					// does it have a Light component?
		int				m_lightPropagationType;		// flat, raycast, etc
		float			m_lightRadius;
		float			m_lightIntensity;
		float			m_lightDropoffMultiplier;		// how much reduction of light there is for cells that are far from the source
		float			m_lightFarDropoffThreshold;	// the dropoff distance (in cells) from which the lightFarDropoffMultiplier is applied on top of the base dropoff
		float			m_lightFarDropoffMultiplier;
		int				m_lightR;						// Light RGB
		int				m_lightG;
		int				m_lightB;
		bool			m_lightAnimated;					// If true, light will be animated (it will flicker) 
		float			m_lightMinIntensityDivider;		// minIntensity = lightIntensity / minIntensityDivider;
		float			m_lightAnimSpeed;					// animation speed

		bool		m_hasAnimator;					// If true, this prefab will have an animator component
		std::string m_animFile;						// Anim file

		bool m_interactable;								// If true, the player will be able to interact with this object while he's in PLAY_MDOE
		int m_gridDistanceInteraction;						// The distance (in grid units) at which the player can interact 
		std::vector<InteractableData> m_interactablesData;	// The consequences of the interactions

	private:
		// Methods to read lines from Prefab file
		void		GetIntFromFile(int* v, std::ifstream* stream, std::string* curLine, std::string* curValStr);
		void		GetBoolFromFile(bool* v, std::ifstream* stream, std::string* curLine, std::string* curValStr);
		void		GetFloatFromFile(float* v, std::ifstream* stream, std::string* curLine, std::string* curValStr);
		void		GetStringFromFile(std::string* v, std::ifstream* stream, std::string* curLine, std::string* curValStr);

	public:
		bool		LoadFromFile(const std::string& filename);
		void		Log();


		std::string GetName();

		static std::unique_ptr<Entity> InstantiatePrefab(const std::string& prefabName, Vector2 pos);
	};

	inline std::string Prefab::GetName()
	{
		return m_pName;
	}

}
}

#endif
