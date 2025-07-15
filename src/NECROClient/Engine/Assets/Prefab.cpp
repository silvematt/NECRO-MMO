#include "Prefab.h"

#include <fstream>

#include "NECROEngine.h"
#include "Entity.h"
#include "Interactable.h"

namespace NECRO
{
namespace Client
{

	// Methods to read lines from the Prefab file
	void Prefab::GetIntFromFile(int* v, std::ifstream* stream, std::string* curLine, std::string* curValStr)
	{
		std::getline(*stream, *curLine);
		*curValStr = curLine->substr(curLine->find("=") + 2); // key = value;
		*curValStr = curValStr->substr(0, curValStr->find(";"));
		*v = ClientUtility::TryParseInt(*curValStr);
	}

	void Prefab::GetBoolFromFile(bool* v, std::ifstream* stream, std::string* curLine, std::string* curValStr)
	{
		std::getline(*stream, *curLine);
		*curValStr = curLine->substr(curLine->find("=") + 2); // key = value;
		*curValStr = curValStr->substr(0, curValStr->find(";"));
		*v = ClientUtility::TryParseInt(*curValStr);
	}

	void Prefab::GetFloatFromFile(float* v, std::ifstream* stream, std::string* curLine, std::string* curValStr)
	{
		std::getline(*stream, *curLine);
		*curValStr = curLine->substr(curLine->find("=") + 2); // key = value;
		*curValStr = curValStr->substr(0, curValStr->find(";"));
		*v = ClientUtility::TryParseFloat(*curValStr);
	}

	void Prefab::GetStringFromFile(std::string* v, std::ifstream* stream, std::string* curLine, std::string* curValStr)
	{
		std::getline(*stream, *curLine);
		*curValStr = curLine->substr(curLine->find("=") + 2); // key = value;
		*curValStr = curValStr->substr(0, curValStr->find(";"));
		*v = *curValStr;
	}

	//--------------------------------------------------------
	// Loads a Prefab from the file specified in 'filename'
	//--------------------------------------------------------
	bool Prefab::LoadFromFile(const std::string& filename)
	{
		std::ifstream stream(filename);

		SDL_Log("Loading Prefab: '%s'\n", filename.c_str());

		if (!stream.is_open())
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load prefab at: %s\n", filename.c_str());
			return false;
		}

		std::string curLine;
		std::string curValStr;
		int curValInt;

		// Prefab Name
		GetStringFromFile(&m_pName, &stream, &curLine, &curValStr);

		// Prefab Img
		GetStringFromFile(&m_pImgFile, &stream, &curLine, &curValStr);

		// toRender
		GetBoolFromFile(&m_toRender, &stream, &curLine, &curValStr);

		// isStatic
		GetBoolFromFile(&m_isStatic, &stream, &curLine, &curValStr);

		// PosOffsetX
		GetFloatFromFile(&m_posOffset.x, &stream, &curLine, &curValStr);

		// PosOffsetY
		GetFloatFromFile(&m_posOffset.y, &stream, &curLine, &curValStr);

		std::getline(stream, curLine); // line break

		// ColliderEnabled
		GetBoolFromFile(&m_hasCollider, &stream, &curLine, &curValStr);

		// colliderRectX
		GetIntFromFile(&m_collRect.x, &stream, &curLine, &curValStr);

		// colliderRectY
		GetIntFromFile(&m_collRect.y, &stream, &curLine, &curValStr);

		// colliderRectW
		GetIntFromFile(&m_collRect.w, &stream, &curLine, &curValStr);

		// colliderRectH
		GetIntFromFile(&m_collRect.h, &stream, &curLine, &curValStr);

		// collOffsetX
		GetIntFromFile(&m_collOffsetX, &stream, &curLine, &curValStr);

		// collOffsetY
		GetIntFromFile(&m_collOffsetY, &stream, &curLine, &curValStr);

		std::getline(stream, curLine); // line break

		// occlCheck
		GetBoolFromFile(&m_occlCheck, &stream, &curLine, &curValStr);

		// occlModX
		GetIntFromFile(&m_occlModX, &stream, &curLine, &curValStr);

		// occlModY
		GetIntFromFile(&m_occlModY, &stream, &curLine, &curValStr);

		std::getline(stream, curLine); // line break

		// BlocksLight
		GetBoolFromFile(&m_blocksLight, &stream, &curLine, &curValStr);

		// blocksLightValue
		GetFloatFromFile(&m_blocksLightValue, &stream, &curLine, &curValStr);

		std::getline(stream, curLine); // line break

		// EmitsLight
		GetBoolFromFile(&m_emitsLight, &stream, &curLine, &curValStr);

		// lightPropagationType
		GetIntFromFile(&m_lightPropagationType, &stream, &curLine, &curValStr);

		// LightRadius
		GetFloatFromFile(&m_lightRadius, &stream, &curLine, &curValStr);

		// Light Intensity
		GetFloatFromFile(&m_lightIntensity, &stream, &curLine, &curValStr);

		// Dropoff multiplier
		GetFloatFromFile(&m_lightDropoffMultiplier, &stream, &curLine, &curValStr);

		// lightFarDropoffThreshold
		GetFloatFromFile(&m_lightFarDropoffThreshold, &stream, &curLine, &curValStr);

		// lightFarDropoffMultiplier
		GetFloatFromFile(&m_lightFarDropoffMultiplier, &stream, &curLine, &curValStr);

		// LightR
		GetIntFromFile(&m_lightR, &stream, &curLine, &curValStr);

		// LightG
		GetIntFromFile(&m_lightG, &stream, &curLine, &curValStr);

		// LightB
		GetIntFromFile(&m_lightB, &stream, &curLine, &curValStr);

		// lightAnimated
		GetBoolFromFile(&m_lightAnimated, &stream, &curLine, &curValStr);

		// lightMinIntensityDivider
		GetFloatFromFile(&m_lightMinIntensityDivider, &stream, &curLine, &curValStr);

		// lightAnimSpeed
		GetFloatFromFile(&m_lightAnimSpeed, &stream, &curLine, &curValStr);

		std::getline(stream, curLine); // line break

		// HasAnim
		GetBoolFromFile(&m_hasAnimator, &stream, &curLine, &curValStr);

		// Anim File name
		GetStringFromFile(&m_animFile, &stream, &curLine, &curValStr);

		std::getline(stream, curLine); // line break

		// Interactable
		GetBoolFromFile(&m_interactable, &stream, &curLine, &curValStr);

		if (m_interactable)
		{
			// gridDistanceInteraction
			GetIntFromFile(&m_gridDistanceInteraction, &stream, &curLine, &curValStr);



			// Load Interactables
			bool doneReadingInteratables = false;
			while (!doneReadingInteratables)
			{
				std::getline(stream, curLine);

				if (curLine.find("ENDINTERACTABLES") != std::string::npos)
					doneReadingInteratables = true;
				else
				{
					// Load Interactables
					// INTERACT_ACTION:(1, "NULL", 0.0, 0.0); // Type, parStr, parFloat1, parFloat2
					// SDL_Log("%s\n", curLine.c_str());

					InteractableData data;

					// Load each data component

					// interactType
					int startPos = curLine.find("(");
					int endPos = curLine.find(",", startPos + 1);
					data.interactType = ClientUtility::TryParseInt(curLine.substr(startPos + 1, endPos - startPos - 1));

					// parStr
					startPos = curLine.find("\"");
					endPos = curLine.find("\"", startPos + 1);
					data.parStr = curLine.substr(startPos + 1, endPos - startPos - 1);

					// parFloat1
					startPos = curLine.find(",", startPos + 1);
					endPos = curLine.find(",", startPos + 1);
					data.parFloat1 = ClientUtility::TryParseFloat(curLine.substr(startPos + 1, endPos - startPos - 1));

					// parFloat2
					startPos = curLine.find(",", startPos + 1);
					endPos = curLine.find(")", startPos + 1);
					data.parFloat2 = ClientUtility::TryParseFloat(curLine.substr(startPos + 1, endPos - startPos - 1));

					// Add to vector
					m_interactablesData.push_back(data);
				}
			}
		}

		// ifstream is closed by destructor
		return true;
	}

	//-------------------------------------------------------------------
	// Instantiates and returns an Entity unique_ptr from a Prefab
	//-------------------------------------------------------------------
	std::unique_ptr<Entity> Prefab::InstantiatePrefab(const std::string& prefabName, Vector2 pos)
	{
		Prefab* p = engine.GetAssetsManager().GetPrefab(prefabName);

		if (p)
		{
			// Create the entity
			std::unique_ptr<Entity> e(new Entity(Vector2(static_cast<float>(pos.x + p->m_posOffset.x), static_cast<float>(pos.y + p->m_posOffset.y)), engine.GetAssetsManager().GetImage(p->m_pImgFile)));

			e->m_toRender = p->m_toRender;

			// Check if the prefab has a collider
			if (p->m_hasCollider)
			{
				// If so, create and set it
				e->CreateCollider();
				e->GetCollider()->Init(true, e.get(), p->m_collRect.x, p->m_collRect.y, p->m_collRect.w, p->m_collRect.h);
				e->GetCollider()->SetOffset(p->m_collOffsetX, p->m_collOffsetY);
			}

			// Check occlusion
			if (p->m_occlCheck)
				e->SetFlag(Entity::Flags::FCanOccludePlayer);

			e->SetOcclusionModifierValues(p->m_occlModX, p->m_occlModY);

			// Light block
			if (p->m_blocksLight)
				e->SetFlag(Entity::Flags::FBlocksLight);

			e->m_blocksLightValue = p->m_blocksLightValue;

			// Check Lighting
			if (p->m_emitsLight)
			{
				e->CreateLight();

				Light* thisLight = e->GetLight();

				thisLight->SetPropagationSetting(static_cast<Light::PropagationSetting>(p->m_lightPropagationType));

				thisLight->m_color.r = p->m_lightR;
				thisLight->m_color.g = p->m_lightG;
				thisLight->m_color.b = p->m_lightB;
				thisLight->m_intensity = p->m_lightIntensity;
				thisLight->m_radius = p->m_lightRadius;
				thisLight->m_dropoffMultiplier = p->m_lightDropoffMultiplier;
				thisLight->m_farDropoffThreshold = p->m_lightFarDropoffThreshold;
				thisLight->m_farDropoffMultiplier = p->m_lightFarDropoffMultiplier;

				// Pos relative to entity
				thisLight->m_pos.x = 0;
				thisLight->m_pos.y = 0;

				// Animated
				thisLight->SetAnim(p->m_lightAnimated);
				thisLight->m_minIntensityDivider = p->m_lightMinIntensityDivider;
				thisLight->m_animSpeed = p->m_lightAnimSpeed;

				// Init
				thisLight->Init(e.get());
			}

			// Check Animator
			if (p->m_hasAnimator)
			{
				e->CreateAnimator();
				e->GetAnimator()->Init(e.get());

				if (!p->m_animFile.empty() && p->m_animFile.compare("NULL") != 0)
				{
					Animator* anim = e->GetAnimator();
					Animator* other = engine.GetAssetsManager().GetAnimator(p->m_animFile);
					if (other)
					{
						*anim = *other; // Sets *anim to be equal to the one in the asset manager (copy-assignment)
						anim->PlayDefaultIfNotNull();
					}
				}
				else
				{
					SDL_LogWarn(SDL_LOG_PRIORITY_WARN, "Prefab %s has Animator enabled, but no Animator file was specified.", p->m_pName.c_str());
				}
			}

			// Check Interactable
			if (p->m_interactable)
			{
				for (int i = 0; i < p->m_interactablesData.size(); i++)
				{
					e->CreateInteractable();

					InteractableData* data = &p->m_interactablesData[i];
					if (data->interactType >= 0 && data->interactType < static_cast<int>(Interactable::InteractType::LAST_VAL))
					{
						Interactable* thisIn = e->GetInteractable(i);
						thisIn->m_gridDistanceInteraction = p->m_gridDistanceInteraction;
						thisIn->m_type = static_cast<Interactable::InteractType>(data->interactType);
						thisIn->m_parStr = data->parStr;
						thisIn->m_parFloat1 = data->parFloat1;
						thisIn->m_parFloat2 = data->parFloat2;
					}
					else
					{
						SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error during Prefab Instantiation of '%s', InteractType is out of bounds at %d.", prefabName.c_str(), i);
					}
				}
			}

			return std::move(e);
		}
		else
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Prefab instantiation failed, %s doesn't exist.", prefabName.c_str());
			return NULL;
		}
	}

	void Prefab::Log()
	{
		SDL_Log("LOGGING PREFAB:");
		SDL_Log("Prefab Name: %s", m_pName.c_str());
		SDL_Log("Prefab File: %s", m_pImgFile.c_str());
		SDL_Log("IsStatic:    %d", m_isStatic);
		SDL_Log("CollEnabled: %d", m_hasCollider);
		SDL_Log("CollRect.x:  %d", m_collRect.x);
		SDL_Log("CollRect.y:  %d", m_collRect.y);
		SDL_Log("CollRect.w:  %d", m_collRect.w);
		SDL_Log("CollRect.h:  %d", m_collRect.h);
		SDL_Log("CollOff.x:   %d", m_collOffsetX);
		SDL_Log("CollOff.y:   %d", m_collOffsetY);
		SDL_Log("OcclCheck:   %d", m_occlCheck);
		SDL_Log("OcclModX:	  %d", m_occlModX);
		SDL_Log("OcclModY:	  %d", m_occlModY);
		SDL_Log("EmitsLight:  %d", m_emitsLight);
		SDL_Log("LPropagation:%d", m_lightPropagationType);
		SDL_Log("LightRadius: %f", m_lightRadius);
		SDL_Log("LIntensity:  %f", m_lightIntensity);
		SDL_Log("LightColorR: %d", m_lightR);
		SDL_Log("LightColorG: %d", m_lightG);
		SDL_Log("LightColorB: %d", m_lightB);
		SDL_Log("LightAnim:   %d", m_lightAnimated);
		SDL_Log("LMinIntDivid:%f", m_lightMinIntensityDivider);
		SDL_Log("LAnimSpeed:  %f", m_lightAnimSpeed);
		SDL_Log("END");
	}

}
}
