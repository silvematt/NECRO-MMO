#ifndef NECROPREFAB_H
#define NECROPREFAB_H

#include <string>
#include <memory>

#include "SDL.h"
#include "Entity.h"

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
	std::string pName;
	std::string pImgFile;
	bool toRender;
	bool isStatic;
	Vector2 posOffset; // position offset when instantiated in a cell

	bool hasCollider;
	SDL_Rect collRect;
	int collOffsetX;
	int collOffsetY;

	bool occlCheck;
	int occlModX;
	int occlModY;

	bool blocksLight;
	float blocksLightValue;

	bool emitsLight;
	int lightPropagationType; // flat, raycast, etc
	float lightRadius;
	float lightIntensity;
	float lightDropoffMultiplier; // how much reduction of light there is for cells that are far from the source
	float lightFarDropoffThreshold; // the dropoff distance (in cells) from which the lightFarDropoffMultiplier is applied on top of the base dropoff
	float lightFarDropoffMultiplier;
	int lightR;
	int lightG;
	int lightB;
	bool lightAnimated;
	float lightMinIntensityDivider;
	float lightAnimSpeed;

	bool hasAnimator;
	std::string animFile;

	bool interactable;
	int gridDistanceInteraction;
	std::vector<InteractableData> interactablesData;

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

#endif
