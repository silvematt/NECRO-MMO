#ifndef NECRORENDERER_H
#define NECRORENDERER_H

#include "SDL.h"
#include "SDL_ttf.h"

#include "RenderTarget.h"

namespace NECRO
{
namespace Client
{
	const int SCREEN_WIDTH = 1920;
	const int SCREEN_HEIGHT = 1080;

	const int HALF_SCREEN_WIDTH = 960;
	const int HALF_SCREEN_HEIGHT = 540;

	// The distance in terms of gridpos of entity->player to know when to start checking for occlusion tests
	const int	ENTITY_OCCLUSION_TEST_X_DIFF = 6;
	const int	ENTITY_OCCLUSION_TEST_Y_DIFF = 6;

	const Uint8 NOT_OCCLUDED_SPRITE_ALPHA_VALUE = 255;
	const Uint8 OCCLUDED_SPRITE_ALPHA_VALUE = 80;

	class Renderer
	{
	public:
		enum class ETargets
		{
			MAIN_TARGET = 0,
			OVERLAY_TARGET,
			DEBUG_TARGET
		};

	private:
		// Main elements
		SDL_Window* window;
		SDL_Renderer* innerRenderer;

		// Targets
		RenderTarget	mainTarget;
		RenderTarget	overlayTarget;
		RenderTarget	debugTarget;

		RenderTarget* curTarget;
		ETargets	curERenTarget; // current enum value

		bool			exportThisFrame = false; // if true, in the next renderer.Update render targets and final image will be exported as .png
		void			SaveTexture(const char* file_name, SDL_Renderer* renderer, SDL_Texture* texture);

	public:

		SDL_Window* const	GetWindow() const;
		SDL_Renderer* const	GetInnerRenderer() const;
		int						GetWidth();
		int						GetHeight();
		ETargets			GetCurrentERenderTargetVal();

		int						Init();
		int						Shutdown();
		void					Show();
		void					Update();
		void					Clear();

		void					SetRenderTarget(ETargets trg);

		void					DrawImageDirectly(SDL_Texture* toDraw, const SDL_Rect* srcRect, const SDL_Rect* dstRect);
		void					DrawTextDirectly(TTF_Font* font, const char* str, int screenX, int screenY, const SDL_Color& color);
		void					DrawIsoBox(SDL_Rect* r, SDL_Color c, float cameraOffsetX, float cameraOffsetY, float cameraZoom);

		void					DrawRect(SDL_Rect* r, SDL_Color c);

		void					SetScale(float scaleX, float scaleY);

		void					SetExportThisFrame() { exportThisFrame = true; };
		void					ExportTargetsSeparate();
		void					ExportComposedFinalImage();
	};

	// Color shortcuts
	extern const SDL_Color colorBlack;
	extern const SDL_Color colorGreen;
	extern const SDL_Color colorRed;
	extern const SDL_Color colorYellow;
	extern const SDL_Color colorWhite;
	extern const SDL_Color colorGray;
	extern const SDL_Color colorPink;

	inline SDL_Window* const Renderer::GetWindow() const
	{
		return window;
	}

	inline SDL_Renderer* const Renderer::GetInnerRenderer() const
	{
		return innerRenderer;
	}

	inline int Renderer::GetWidth()
	{
		return SCREEN_WIDTH;
	}

	inline int Renderer::GetHeight()
	{
		return SCREEN_HEIGHT;
	}

	inline Renderer::ETargets Renderer::GetCurrentERenderTargetVal()
	{
		return curERenTarget;
	}

}
}

#endif
