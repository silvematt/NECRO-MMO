#include "Renderer.h"
#include <array>
#include <cmath>

#include "NMath.h"
#include <SDL_image.h>

namespace NECRO
{
namespace Client
{
	//--------------------------------------
	// Initialize
	//--------------------------------------
	int Renderer::Init()
	{
		// Create Window
		Uint32 winFlags = 0;
		//Uint32 winFlags = SDL_WINDOW_FULLSCREEN;
		m_window = SDL_CreateWindow("NECROClient", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, winFlags);

		if (!m_window)
			return -1;

		// Create Renderer
		Uint32 renFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE;
		m_innerRenderer = SDL_CreateRenderer(m_window, -1, renFlags);

		if (!m_innerRenderer)
			return -2;

		// Initialize the Draw Color to black
		SDL_SetRenderDrawColor(m_innerRenderer, colorBlack.r, colorBlack.g, colorBlack.b, colorBlack.a);

		// Create the RenderTargets
		m_mainTarget.CreateMain(m_innerRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);
		m_overlayTarget.Create(m_innerRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);
		m_debugTarget.Create(m_innerRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);

		SetRenderTarget(ETargets::MAIN_TARGET);

		m_exportThisFrame = false;

		return 0;
	}

	//--------------------------------------
	// Shuts down the Rendering SubSystem
	//--------------------------------------
	int Renderer::Shutdown()
	{
		if (m_innerRenderer)
			SDL_DestroyRenderer(m_innerRenderer);

		if (m_window)
			SDL_DestroyWindow(m_window);

		return 0;
	}

	//--------------------------------------
	// Render Present
	//--------------------------------------
	void Renderer::Show()
	{
		SDL_RenderPresent(m_innerRenderer);
	}

	//-----------------------------------------------------------------------------
	// Update the innerRenderer with the stuff that has been drawn to the targets
	//-----------------------------------------------------------------------------
	void Renderer::Update()
	{
		// If there was a cmd issued to export render targe textures separately, we need to do it now before we start overlapping them
		if (m_exportThisFrame)
			ExportTargetsSeparate();

		// Actual Renderer::Update
		SetRenderTarget(ETargets::MAIN_TARGET);

		// Copy the other targets on the (default) mainTarget
		SDL_RenderCopy(m_innerRenderer, m_overlayTarget.GetTargetTexture(), NULL, NULL);
		SDL_RenderCopy(m_innerRenderer, m_debugTarget.GetTargetTexture(), NULL, NULL);

		// Export final image as well
		if (m_exportThisFrame)
		{
			ExportComposedFinalImage();
			m_exportThisFrame = false;
		}
	}

	//----------------------------------------------
	// Calls SDL_RenderSetScale on the innerRenderer
	//----------------------------------------------
	void Renderer::SetScale(float scaleX, float scaleY)
	{
		SDL_RenderSetScale(m_innerRenderer, scaleX, scaleY);
	}

	//----------------------------------------------
	// Draws a Texture directly in the innerRenderer
	//----------------------------------------------
	void Renderer::DrawImageDirectly(SDL_Texture* toDraw, const SDL_Rect* srcRect, const SDL_Rect* dstRect)
	{
		//if(toDraw == nullptr)
		//	SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Nothing to draw!\n");
		SDL_RenderCopy(m_innerRenderer, toDraw, srcRect, dstRect);
	}

	//-------------------------------------------------------------------
	// Draws a Text directly to the innerRenderer
	// 
	// TODO: For actual text rendering we can have a list of text to draw
	// with their own SDL_Texture and Surface, so that we do not create
	// them each frame. 
	//-------------------------------------------------------------------
	void Renderer::DrawTextDirectly(TTF_Font* font, const char* str, int screenX, int screenY, const SDL_Color& color)
	{
		SDL_Texture* renderedText = nullptr;

		SDL_Surface* textSurf = TTF_RenderText_Solid(font, str, color);
		renderedText = SDL_CreateTextureFromSurface(m_innerRenderer, textSurf);
		SDL_Rect dest = { screenX, screenY, textSurf->w, textSurf->h };

		SDL_RenderCopy(m_innerRenderer, renderedText, NULL, &dest);

		SDL_DestroyTexture(renderedText);
		SDL_FreeSurface(textSurf);
	}

	//----------------------------------------------------------------------------------
	// From the Rect, convert the points in isometric and draw them as a iso-box
	//----------------------------------------------------------------------------------
	void Renderer::DrawIsoBox(SDL_Rect* rect, SDL_Color color, float cameraOffsetX, float cameraOffsetY, float cameraZoom) // offset are camera offset
	{
		// To correctly use SDL_RenderDrawLines, manually scale the coordinates based on the zoom level
		float previousScaleX = 0, previousScaleY = 0;
		SDL_RenderGetScale(m_innerRenderer, &previousScaleX, &previousScaleY);

		// Set Scale to 1
		SetScale(1.0, 1.0);

		std::array<SDL_Point, 5> points;

		// Define the 4 corners of the rectangle
		points[0] = { rect->x, rect->y };
		points[1] = { rect->x + rect->w, rect->y };
		points[2] = { rect->x + rect->w, rect->y + rect->h };
		points[3] = { rect->x, rect->y + rect->h };
		points[4] = points[0];  // Closing the loop

		// Convert to isometric and translate with camera, while accounting for zoom level
		for (size_t i = 0; i < points.size(); ++i)
		{
			float x = static_cast<float>(points[i].x);
			float y = static_cast<float>(points[i].y);

			NMath::CartToIso(x / CELL_WIDTH, y / CELL_HEIGHT, x, y);

			// Account for camera offset
			x += cameraOffsetX;
			y += cameraOffsetY;

			// Apply zoom level
			x *= cameraZoom;
			y *= cameraZoom;

			x = std::floor(x);
			y = std::floor(y);

			points[i] = { static_cast<int>(x), static_cast<int>(y) };
		}

		// Set render color and draw the lines
		SDL_SetRenderDrawColor(m_innerRenderer, color.r, color.g, color.b, color.a);
		SDL_RenderDrawLines(m_innerRenderer, points.data(), points.size());
		//SDL_RenderDrawPoints(innerRenderer, points.data(), points.size());

		// Restore scale
		SetScale(previousScaleX, previousScaleY);
	}

	//------------------------------------------------
	// Calls SDL_RenderFillRect on the inner renderer
	//------------------------------------------------
	void Renderer::DrawRect(SDL_Rect* r, SDL_Color c)
	{
		SDL_SetRenderDrawColor(m_innerRenderer, c.r, c.g, c.b, c.a);
		SDL_RenderFillRect(m_innerRenderer, r);
	}

	//--------------------------------------
	// Clear the renderer targets
	//--------------------------------------
	void Renderer::Clear()
	{
		m_mainTarget.Clear();
		m_overlayTarget.Clear();
		m_debugTarget.Clear();
	}

	//------------------------------------------------
	// Sets the Render target of the innerRenderer
	//------------------------------------------------
	void Renderer::SetRenderTarget(ETargets trg)
	{
		switch (trg)
		{
		case ETargets::MAIN_TARGET:
			m_curTarget = &m_mainTarget;
			SDL_SetRenderTarget(m_innerRenderer, m_mainTarget.GetTargetTexture());
			break;

		case ETargets::OVERLAY_TARGET:
			m_curTarget = &m_overlayTarget;
			SDL_SetRenderTarget(m_innerRenderer, m_overlayTarget.GetTargetTexture());
			break;

		case ETargets::DEBUG_TARGET:
			m_curTarget = &m_debugTarget;
			SDL_SetRenderTarget(m_innerRenderer, m_debugTarget.GetTargetTexture());
			break;
		}

		m_curERenTarget = trg;
	}


	//------------------------------------------------
	// Saves texture to disk, using IMG_SavePNG
	//------------------------------------------------
	void Renderer::SaveTexture(const char* file_name, SDL_Renderer* renderer, SDL_Texture* texture)
	{
		// Backup current render target
		auto previousTarget = Renderer::GetCurrentERenderTargetVal();

		SDL_Texture* target = SDL_GetRenderTarget(renderer);

		// Set texture as render target
		if (SDL_SetRenderTarget(renderer, texture) != 0)
		{
			SDL_Log("Failed to set render target: %s", SDL_GetError());
			return;
		}

		int width, height;

		if (texture != NULL)
		{
			if (SDL_QueryTexture(texture, NULL, NULL, &width, &height) != 0)
			{
				SDL_Log("Failed to query texture: %s", SDL_GetError());
				SDL_SetRenderTarget(renderer, target);
				return;
			}
		}
		else // if texture is null, then it's the final composed image
		{
			width = SCREEN_WIDTH;
			height = SCREEN_HEIGHT;
		}

		SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
		if (!surface)
		{
			SDL_Log("Failed to create surface: %s", SDL_GetError());
			SDL_SetRenderTarget(renderer, target);
			return;
		}

		if (SDL_RenderReadPixels(renderer, NULL, surface->format->format, surface->pixels, surface->pitch) != 0)
		{
			SDL_Log("Failed to read pixels: %s", SDL_GetError());
			SDL_FreeSurface(surface);
			SDL_SetRenderTarget(renderer, target);
			return;
		}

		if (IMG_SavePNG(surface, file_name) != 0)
			SDL_Log("Failed to save PNG: %s", SDL_GetError());

		SDL_FreeSurface(surface);

		// Restore previous render target
		SetRenderTarget(previousTarget);
	}


	//-----------------------------------------------------------------------------------------------
	// Exports the render targets as separate images, needs to be done before Renderer.Update()
	// so before the main target starts to get layered with the others
	//-----------------------------------------------------------------------------------------------
	void Renderer::ExportTargetsSeparate()
	{
		SaveTexture("main.png", m_innerRenderer, NULL);
		SaveTexture("overlay.png", m_innerRenderer, m_overlayTarget.GetTargetTexture());
		SaveTexture("debug.png", m_innerRenderer, m_debugTarget.GetTargetTexture());
	}

	//-----------------------------------------------------------------------------------------------
	// Exports the final composed image, in order to work, this needs to be done AFTER Renderer.Update()
	//-----------------------------------------------------------------------------------------------
	void Renderer::ExportComposedFinalImage()
	{
		SaveTexture("final.png", m_innerRenderer, NULL);
	}

}
}
