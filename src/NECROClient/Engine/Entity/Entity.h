#ifndef NECROENTITY_H
#define NECROENTITY_H

#include <memory>

#include "Vector2.h"
#include "Image.h"
#include "Collider.h"
#include "Light.h"
#include "Animator.h"
#include "Interactable.h"

#define LAYER_Z_COEFFICIENT 100 // A layer counts as 100 Z pos unit for entities

class Interactable;
class Cell;
class Animator;

//-----------------------------------------------------------------------------
// An Enity is the base unit of something that exists inside of a Cell.
//-----------------------------------------------------------------------------
class Entity
{
	// Flags that describe entities
public:
	enum Flags
	{
		FCanOccludePlayer = 1,
		FBlocksLight = 2,
		Dynamic = 4				// Dynamic entities like player and AI have special properties, such as Z positioning which is calculated from the ZPos and not layers
	};

	friend class Prefab;
	friend class Animator; // Animator is a friend class of Entity

private:
	static uint32_t ENT_NEXT_ID; // EntityID static track
public:
	static bool DEBUG_COLLIDER_ENABLED;
	static int	DEBUG_COLLIDER_LAYER; // -1 to debug all layers, otherwise debug only layer value
	static bool DEBUG_OCCLUSION_ENABLED;

protected:
	uint32_t ID;				// EntityID
	Image* img;
	uint32_t layer = 0;
	Cell* owner;				// Owner of this entity
	Cell* nextOwner;			// Used to TransferToCellQueue()

	bool toRender = true;

	uint16_t eFlags = 0;		// this entityflags value

	// Used for entities that uses tilesets, index of X and Y, they will be multiplied by img->GetTileset().tileWidth and img->GetTileset().tileHeight
	int tilesetXOff, tilesetYOff;

	int occlModifierX, occlModifierY; // used to help shape the occlusion box starting from the dst rect
	SDL_Rect occlusionRect;
	bool occludes = false;		// if true, it will be drawn with OCCLUDED_SPRITE_ALPHA_VALUE

	SDL_Color lightingColor;	// Calculated in UpdateLighting(), it is the color the entity when drawn (not the color of the light it emits)

	float blocksLightValue = 0.0f;

	// "Components", they are created or not in base of prefab options. An alternative could be std::optional
	std::unique_ptr<Collider> coll;
	std::unique_ptr<Light> emittingLight;
	std::unique_ptr<Animator> anim;
	std::vector<std::unique_ptr<Interactable>> interactables;

	// How much this entity affects the zModifier of the cell it sits in
	float zModifierValue = 0.0;
	
public:
	virtual ~Entity();
	Entity();
	Entity(Vector2 pInitialPos, Image* pImg);

	Vector2 pos;				// orthographic pos
	float	zPos;				// Z is up
	float depth;				// For isometric sorting

	int gridPosX, gridPosY;		// Position in the gridmap

	Vector2 isoPos;				// isometric pos (used only for rendering)

public:
	const uint32_t	GetID() const;
	Cell*			GetOwner();

	void			SetFlag(Flags flag);	// Manage flags
	void			ClearFlag(Flags flag);
	bool			TestFlag(Flags flag);

	void			SetTilesetOffset(int x, int y);

	int				GetLayerFromZPos() const;

	// Optional components functions
	void			CreateLight();
	bool			HasLight() const;
	Light*			GetLight() const;

	void			CreateCollider();
	bool			HasCollider() const;
	Collider*		GetCollider() const;

	void			CreateAnimator();
	bool			HasAnimator() const;
	Animator*		GetAnimator() const;

	void			CreateInteractable();
	bool			HasInteractable() const;
	Interactable*	GetInteractable(int indx) const;
	int				GetInteractablesSize() const;
	void			DestroyInteractables();		// Called if InteractableType is out of bounds during prefab loading, to prevent destructive behaviors

	bool			Occludes();

	void			SetLayer(uint32_t newLayer);
	uint32_t		GetLayer();
	void			SetImg(Image* pImg);
	void			SetOwner(Cell* c);
	void			ClearOwner();
	void			TransferToCellImmediately(Cell* c);		// Transfer this entity to exist in another cell 
	void			TransferToCellQueue(Cell* c);			// Transfer this entity to exist in another cell AFTER a world update completes
	void			SetOccludes(bool val);
	void			UpdateLighting();
	bool			BlocksLight();
	float			GetLightBlockValue();
	void			SetOcclusionModifierValues(int x, int y);

	virtual void	Update();
	virtual void	Draw();

	float			GetZCellModifier() const;
	void			SetZCellModifier(float v);

	virtual void	OnCellChanges();
};

inline const uint32_t Entity::GetID() const
{
	return ID;
}

inline Cell* Entity::GetOwner()
{
	return owner;
}

inline void Entity::SetTilesetOffset(int x, int y)
{
	tilesetXOff = x;
	tilesetYOff = y;
}

inline int Entity::GetLayerFromZPos() const
{
	return std::floor(zPos / LAYER_Z_COEFFICIENT);
}

inline void Entity::CreateCollider()
{
	if (!HasCollider())
		coll = std::make_unique<Collider>();
}

inline bool Entity::HasCollider() const
{
	return coll != nullptr;
}

inline Collider* Entity::GetCollider() const
{
	return coll.get();
}

inline void Entity::CreateLight()
{
	if (!HasLight())
		emittingLight = std::make_unique<Light>();
}

inline bool Entity::HasLight() const
{
	return emittingLight != nullptr;
}

inline Light* Entity::GetLight() const
{
	return emittingLight.get();
}

inline void Entity::CreateAnimator()
{
	if (!HasAnimator())
		anim = std::make_unique<Animator>();
}

inline bool Entity::HasAnimator() const
{
	return anim != nullptr;
}

inline Animator* Entity::GetAnimator() const
{
	return anim.get();
}

inline void Entity::CreateInteractable()
{
	interactables.push_back(std::make_unique<Interactable>(this));
}

inline bool Entity::HasInteractable() const
{
	return interactables.size() > 0;
}

inline Interactable* Entity::GetInteractable(int indx) const
{
	if (interactables.at(indx))
		return interactables[indx].get();
	else
		return nullptr;
}

inline int Entity::GetInteractablesSize() const
{
	return interactables.size();
}


// Called if InteractableType is out of bounds, to prevent destructive behaviors
inline void Entity::DestroyInteractables()
{
	for (int i = 0; i < interactables.size(); i++)
	{
		interactables[i].reset();
	}

	interactables.clear();
}

inline void Entity::SetFlag(Flags flag)
{
	eFlags |= flag;
}

inline void Entity::ClearFlag(Flags flag)
{
	eFlags &= ~flag;
}

inline bool Entity::TestFlag(Flags flag)
{
	return ((eFlags & flag) > 0) ? true : false;
}

inline void Entity::SetOccludes(bool val)
{
	occludes = val;
}

inline bool Entity::Occludes()
{
	return occludes;
}

inline bool Entity::BlocksLight()
{
	return TestFlag(FBlocksLight);
}

inline float Entity::GetLightBlockValue()
{
	return blocksLightValue;
}

inline void Entity::SetLayer(uint32_t newLayer)
{
	layer = newLayer;
}

inline uint32_t Entity::GetLayer()
{
	return layer;
}

inline void Entity::SetOcclusionModifierValues(int x, int y)
{
	occlModifierX = x;
	occlModifierY = y;
}

inline float Entity::GetZCellModifier() const
{
	return zModifierValue;
}

inline void Entity::SetZCellModifier(float v)
{
	zModifierValue = v;
}

#endif
