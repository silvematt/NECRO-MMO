#ifndef NECRORENDER_TARGET_H
#define NECRORENDER_TARGET_H

#include "SDL.h"

namespace NECRO
{
namespace Client
{
	// ----------------------------------------------------------------------------------------------------
	// A target to render on.
	// 
	// It is a to use SDL_RenderTarget to allow writing on Textures and then blitting these
	// textures on the innerRenderer. We can have one texture to render the game (that can scale
	// when zooming in and out) another that will render overlay, another that will render the minimap, etc.
	// 
	// It allows to essentialy compose the final image by overlapping different textures.
	// 
	// mainTarget is always the SDL_Renderer target, which is set by: SDL_SetRenderTarget(NULL) 
	// ----------------------------------------------------------------------------------------------------
	class RenderTarget
	{
	private:
		SDL_Renderer*	m_context;
		SDL_Texture*	m_texture;

	public:
		void	CreateMain(SDL_Renderer* cntx, int w, int h);
		void	Create(SDL_Renderer* cntx, int w, int h);
		void	Clear();

		SDL_Texture* GetTargetTexture() const;
	};

	inline SDL_Texture* RenderTarget::GetTargetTexture() const
	{
		return m_texture;
	}

}
}

#endif
