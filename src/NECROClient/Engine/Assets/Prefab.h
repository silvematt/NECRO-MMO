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
		std::string pName;		// name of the prefab
		std::string pImgFile;	// name of the img associated with this prefab
		bool toRender;			// false for invisible objects
		bool isStatic;			// is it an unmovable object?
		Vector2 posOffset;		// position offset when instantiated in a cell

		bool hasCollider;		// has a collider component
		SDL_Rect collRect;
		int collOffsetX;
		int collOffsetY;

		bool occlCheck;				// If true, the prefab can occlude the player so we can render this prefab transparent
		int occlModX;				// X and Y offsets for the occlusion rect that will be tested against the player/entities that needs to be visible all the time
		int occlModY;

		bool blocksLight;			// If true, lights that hits this prefab will block the propagation by blocksLightValue
		float blocksLightValue;

		bool emitsLight;			// does it have a Light component?
		int lightPropagationType;	// flat, raycast, etc
		float lightRadius;
		float lightIntensity;
		float lightDropoffMultiplier;		// how much reduction of light there is for cells that are far from the source
		float lightFarDropoffThreshold;		// the dropoff distance (in cells) from which the lightFarDropoffMultiplier is applied on top of the base dropoff
		float lightFarDropoffMultiplier;
		int lightR;							// Light RGB
		int lightG;
		int lightB;
		bool lightAnimated;					// If true, light will be animated (it will flicker) 
		float lightMinIntensityDivider;		// minIntensity = lightIntensity / minIntensityDivider;
		float lightAnimSpeed;				// aniamtion speed

		bool hasAnimator;					// If true, this prefab will have an animator component
		std::string animFile;				// Anim file

		bool interactable;									// If true, the player will be able to interact with this object while he's in PLAY_MDOE
		int gridDistanceInteraction;						// The distance (in grid units) at which the player can interact 
		std::vector<InteractableData> interactablesData;	// The consequences of the interactions

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
		return pName;
	}

}
}

#endif
