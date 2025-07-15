#include "RenderTarget.h"
#include "Renderer.h"

namespace NECRO
{
namespace Client
{
	// ----------------------------------------------------------------------------------------------------
	// Creates the Render Target for the mainTarget, who's texture is = NULL
	// ----------------------------------------------------------------------------------------------------
	void RenderTarget::CreateMain(SDL_Renderer* cntx, int w, int h)
	{
		m_context = cntx;
		m_texture = NULL; // This allows to call SDL_SetRenderTarget(NULL) that sets the RenderTarget to the renderer's
	}

	// ----------------------------------------------------------------------------------------------------
	// Creates the Render Target for the mainTarget, who's texture is = NULL
	// ----------------------------------------------------------------------------------------------------
	void RenderTarget::Create(SDL_Renderer* cntx, int w, int h)
	{
		m_context = cntx;

		// Determine Pixel format
		SDL_RendererInfo rInfo;
		SDL_GetRendererInfo(m_context, &rInfo);

		m_texture = SDL_CreateTexture(m_context, rInfo.texture_formats[0], SDL_TEXTUREACCESS_TARGET, w, h);

		if (!m_texture)
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Create RenderTarget!\n");

		// Allow Alpha Blending
		SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_BLEND);
	}

	// ----------------------------------------------------------------------------------------------------
	// Clears the target ensuring transparency, so one texture doesn't cover the previous entirely
	// ----------------------------------------------------------------------------------------------------
	void RenderTarget::Clear()
	{
		SDL_Texture* currentTarget = SDL_GetRenderTarget(m_context);

		// Clear the RenderTarget
		SDL_SetRenderTarget(m_context, m_texture);
		SDL_SetRenderDrawColor(m_context, colorBlack.r, colorBlack.g, colorBlack.b, SDL_ALPHA_TRANSPARENT); // Ensure the empty parts are transparent
		SDL_RenderClear(m_context);

		SDL_SetRenderTarget(m_context, currentTarget);
	}

}
}
