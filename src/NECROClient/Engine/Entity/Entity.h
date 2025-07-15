#ifndef NECROENTITY_H
#define NECROENTITY_H

#include <memory>

#include "Vector2.h"
#include "Image.h"
#include "Collider.h"
#include "Light.h"
#include "Animator.h"
#include "Interactable.h"

namespace NECRO
{
namespace Client
{
	inline constexpr int LAYER_Z_COEFFICIENT = 100; // A layer counts as 100 Z pos unit for entities

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
			FDynamic = 4				// Dynamic entities like player and AI have special properties, such as Z positioning which is calculated from the ZPos and not layers
		};

		friend class Prefab;
		friend class Animator;

	private:
		static uint32_t ENT_NEXT_ID; // EntityID static track
	public:
		static bool DEBUG_COLLIDER_ENABLED;
		static int	DEBUG_COLLIDER_LAYER;		// -1 to debug all layers, otherwise debug only layer value
		static bool DEBUG_OCCLUSION_ENABLED;

	protected:
		uint32_t	m_ID;				// EntityID
		Image*		m_img;
		uint32_t	m_layer = 0;
		Cell*		m_owner;				// Owner of this entity
		Cell*		m_nextOwner;			// Used to TransferToCellQueue()

		bool		m_toRender = true;

		uint16_t	m_eFlags = 0;		// this entityflags value

		// Used for entities that uses tilesets, index of X and Y, they will be multiplied by img->GetTileset().tileWidth and img->GetTileset().tileHeight
		int m_tilesetXOff;
		int m_tilesetYOff;

		int			m_occlModifierX;
		int			m_occlModifierY; // used to help shape the occlusion box starting from the dst rect
		SDL_Rect	m_occlusionRect;
		bool		m_occludes = false;		// if true, it will be drawn with OCCLUDED_SPRITE_ALPHA_VALUE

		SDL_Color	m_lightingColor;	// Calculated in UpdateLighting(), it is the color the entity when drawn (not the color of the light it emits)

		float		m_blocksLightValue = 0.0f;

		// "Components", they are created or not in base of prefab options. An alternative could be std::optional
		std::unique_ptr<Collider>	m_coll;
		std::unique_ptr<Light>		m_emittingLight;
		std::unique_ptr<Animator>	m_anim;
		std::vector<std::unique_ptr<Interactable>> m_interactables;

		// How much this entity affects the zModifier of the cell it sits in
		float m_zModifierValue = 0.0;

	public:
		virtual ~Entity();
		Entity();
		Entity(Vector2 pInitialPos, Image* pImg);

		Vector2 m_pos;			// orthographic pos
		float	m_zPos;			// Z is up
		float	m_depth;		// For isometric sorting

		int		m_gridPosX;
		int		m_gridPosY;			// Position in the gridmap

		Vector2 m_isoPos;			// isometric pos (used only for rendering)

	public:
		const uint32_t	GetID() const;
		Cell* GetOwner();

		void			SetFlag(Flags flag);	// Manage flags
		void			ClearFlag(Flags flag);
		bool			TestFlag(Flags flag);

		void			SetTilesetOffset(int x, int y);

		int				GetLayerFromZPos() const;

		// Optional components functions
		void			CreateLight();
		bool			HasLight() const;
		Light* GetLight() const;

		void			CreateCollider();
		bool			HasCollider() const;
		Collider* GetCollider() const;

		void			CreateAnimator();
		bool			HasAnimator() const;
		Animator* GetAnimator() const;

		void			CreateInteractable();
		bool			HasInteractable() const;
		Interactable* GetInteractable(int indx) const;
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
		return m_ID;
	}

	inline Cell* Entity::GetOwner()
	{
		return m_owner;
	}

	inline void Entity::SetTilesetOffset(int x, int y)
	{
		m_tilesetXOff = x;
		m_tilesetYOff = y;
	}

	inline int Entity::GetLayerFromZPos() const
	{
		return std::floor(m_zPos / LAYER_Z_COEFFICIENT);
	}

	inline void Entity::CreateCollider()
	{
		if (!HasCollider())
			m_coll = std::make_unique<Collider>();
	}

	inline bool Entity::HasCollider() const
	{
		return m_coll != nullptr;
	}

	inline Collider* Entity::GetCollider() const
	{
		return m_coll.get();
	}

	inline void Entity::CreateLight()
	{
		if (!HasLight())
			m_emittingLight = std::make_unique<Light>();
	}

	inline bool Entity::HasLight() const
	{
		return m_emittingLight != nullptr;
	}

	inline Light* Entity::GetLight() const
	{
		return m_emittingLight.get();
	}

	inline void Entity::CreateAnimator()
	{
		if (!HasAnimator())
			m_anim = std::make_unique<Animator>();
	}

	inline bool Entity::HasAnimator() const
	{
		return m_anim != nullptr;
	}

	inline Animator* Entity::GetAnimator() const
	{
		return m_anim.get();
	}

	inline void Entity::CreateInteractable()
	{
		m_interactables.push_back(std::make_unique<Interactable>(this));
	}

	inline bool Entity::HasInteractable() const
	{
		return m_interactables.size() > 0;
	}

	inline Interactable* Entity::GetInteractable(int indx) const
	{
		if (m_interactables.at(indx))
			return m_interactables[indx].get();
		else
			return nullptr;
	}

	inline int Entity::GetInteractablesSize() const
	{
		return m_interactables.size();
	}


	// Called if InteractableType is out of bounds, to prevent destructive behaviors
	inline void Entity::DestroyInteractables()
	{
		for (int i = 0; i < m_interactables.size(); i++)
		{
			m_interactables[i].reset();
		}

		m_interactables.clear();
	}

	inline void Entity::SetFlag(Flags flag)
	{
		m_eFlags |= flag;
	}

	inline void Entity::ClearFlag(Flags flag)
	{
		m_eFlags &= ~flag;
	}

	inline bool Entity::TestFlag(Flags flag)
	{
		return ((m_eFlags & flag) > 0) ? true : false;
	}

	inline void Entity::SetOccludes(bool val)
	{
		m_occludes = val;
	}

	inline bool Entity::Occludes()
	{
		return m_occludes;
	}

	inline bool Entity::BlocksLight()
	{
		return TestFlag(FBlocksLight);
	}

	inline float Entity::GetLightBlockValue()
	{
		return m_blocksLightValue;
	}

	inline void Entity::SetLayer(uint32_t newLayer)
	{
		m_layer = newLayer;
	}

	inline uint32_t Entity::GetLayer()
	{
		return m_layer;
	}

	inline void Entity::SetOcclusionModifierValues(int x, int y)
	{
		m_occlModifierX = x;
		m_occlModifierY = y;
	}

	inline float Entity::GetZCellModifier() const
	{
		return m_zModifierValue;
	}

	inline void Entity::SetZCellModifier(float v)
	{
		m_zModifierValue = v;
	}

}
}

#endif
