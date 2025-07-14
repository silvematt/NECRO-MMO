#ifndef NECROIMAGE_H
#define NECROIMAGE_H

#include "SDL.h"
#include "ClientUtility.h"
#include <memory>

#include <fstream>

namespace NECRO
{
namespace Client
{
	// IMGs Defs folder and extension
	#define IMG_DEFS_FOLDER "Data/imgs/defs/"
	#define IMG_DEFS_EXTENSION ".nidf"

	//-------------------------------------------------------
	// Represents an Image
	//-------------------------------------------------------
	class Image
	{
	public:
		// Data for tilesets
		struct Tileset
		{
			int tileWidth;
			int tileHeight;

			int tileXNum;
			int tileYNum;
		};

	private:
		SDL_Texture* imgTexture;
		int width;
		int height;

		SDL_Rect rect;

		int offsetX;
		int offsetY;					// Offset Y is used to draw images that are, for example 64x64 on map of 64x32
										// If the 'tree.png' is 64x64, it should be drawn with a y offset of -32 (to draw the bottom of the tree correctly)
		bool isTileset;
		std::shared_ptr<Tileset> tileset;

	public:
		Image(SDL_Texture* tex, int xOff, int yOff);
		Image(SDL_Texture* tex, int xOff, int yOff, int tWidth, int tHeight, int tNumX, int tNumY);
		Image(SDL_Texture* tex, const std::string& fileDef); // Constructor with img definition

		SDL_Texture* GetSrc() const;
		SDL_Rect& GetRect();
		int							GetWidth() const;
		int							GetHeight() const;
		int							GetXOffset() const;
		int							GetYOffset() const;

		bool						IsTileset() const;
		Tileset* GetTileset() const;

		int							GetTilesetWidth() const; // shortcut
		int							GetTilesetHeight() const; // shortcut
		SDL_Rect					TilesetGetSubframeAt(int x, int y);
	};


	//-------------------------------------------------------
	// Constructor
	//-------------------------------------------------------
	inline Image::Image(SDL_Texture* tex, int xOff, int yOff) :
		imgTexture(tex),
		offsetX(xOff),
		offsetY(yOff)
	{
		SDL_QueryTexture(imgTexture, NULL, NULL, &width, &height);
		SDL_SetTextureBlendMode(imgTexture, SDL_BLENDMODE_BLEND);

		rect = { 0, 0, width, height };

		isTileset = false;
	}

	//-------------------------------------------------------
	// Constructor for Tileset
	//-------------------------------------------------------
	inline Image::Image(SDL_Texture* tex, int xOff, int yOff, int tWidth, int tHeight, int tNumX, int tNumY) :
		imgTexture(tex),
		offsetX(xOff),
		offsetY(yOff)
	{
		SDL_QueryTexture(imgTexture, NULL, NULL, &width, &height);
		SDL_SetTextureBlendMode(imgTexture, SDL_BLENDMODE_BLEND);

		rect = { 0, 0, width, height };

		isTileset = true;

		tileset = std::make_shared<Tileset>();

		tileset->tileWidth = tWidth;
		tileset->tileHeight = tHeight;
		tileset->tileXNum = tNumX;
		tileset->tileYNum = tNumY;
	}

	//--------------------------------------------------------
	// Looks if there's a img definition and loads the image
	//--------------------------------------------------------
	inline Image::Image(SDL_Texture* tex, const std::string& fileDefinition) : Image::Image(tex, 0, 0) // Base constructor is called first
	{
		// At this point fileDefinition is 'img_name.png'

		// Cut the img extension and add the img_def extension
		std::string filedefName = fileDefinition.substr(0, fileDefinition.find_last_of('.'));
		filedefName += IMG_DEFS_EXTENSION;

		// Build a path to the img definition
		std::string fullPath = IMG_DEFS_FOLDER;
		fullPath += filedefName;

		// Try to open the file definition
		SDL_Log("Loading IMG '%s'... Looking for definition.\n", fileDefinition.c_str());
		std::ifstream stream(fullPath);

		if (!stream.is_open())
		{
			SDL_Log("No IMG definition found.\n");

			// At this point this image is loaded with the base constructor Image::Image(tex, 0, 0)
			return;
		}
		else
		{
			SDL_Log("Loading IMG '%s' from definition.\n", fileDefinition.c_str());

			std::string curLine;
			std::string curValStr;

			// IsTileset
			std::getline(stream, curLine);
			curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
			curValStr = curValStr.substr(0, curValStr.find(";"));
			isTileset = ClientUtility::TryParseInt(curValStr);

			// xOffset
			std::getline(stream, curLine);
			curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
			curValStr = curValStr.substr(0, curValStr.find(";"));
			offsetX = ClientUtility::TryParseInt(curValStr);

			// yOffset
			std::getline(stream, curLine);
			curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
			curValStr = curValStr.substr(0, curValStr.find(";"));
			offsetY = ClientUtility::TryParseInt(curValStr);

			if (isTileset)
			{
				// Make the tileset data now that is needed
				tileset = std::make_shared<Tileset>();

				// tWidth
				std::getline(stream, curLine);
				curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
				curValStr = curValStr.substr(0, curValStr.find(";"));
				tileset->tileWidth = ClientUtility::TryParseInt(curValStr);

				// tHeight
				std::getline(stream, curLine);
				curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
				curValStr = curValStr.substr(0, curValStr.find(";"));
				tileset->tileHeight = ClientUtility::TryParseInt(curValStr);

				// tileXNum
				std::getline(stream, curLine);
				curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
				curValStr = curValStr.substr(0, curValStr.find(";"));
				tileset->tileXNum = ClientUtility::TryParseInt(curValStr);

				// tileYNum
				std::getline(stream, curLine);
				curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
				curValStr = curValStr.substr(0, curValStr.find(";"));
				tileset->tileYNum = ClientUtility::TryParseInt(curValStr);
			}

			// ifstream is closed by destructor
			return;
		}
	}

	inline SDL_Rect Image::TilesetGetSubframeAt(int x, int y)
	{
		SDL_Rect r;
		r.x = x * tileset->tileWidth;
		r.y = y * tileset->tileHeight;
		r.w = tileset->tileWidth;
		r.h = tileset->tileHeight;
		return r;
	}

	inline SDL_Texture* Image::GetSrc() const
	{
		return imgTexture;
	}

	inline SDL_Rect& Image::GetRect()
	{
		return rect;
	}

	inline int Image::GetWidth() const
	{
		return width;
	}

	inline int Image::GetHeight() const
	{
		return height;
	}

	inline int Image::GetYOffset() const
	{
		return offsetY;
	}

	inline int Image::GetXOffset() const
	{
		return offsetX;
	}

	inline bool Image::IsTileset() const
	{
		return isTileset;
	}

	inline Image::Tileset* Image::GetTileset() const
	{
		if (IsTileset())
			return tileset.get();
		else
			return nullptr;
	}

	inline int Image::GetTilesetHeight() const
	{
		if (IsTileset())
			return tileset->tileHeight;
		else
			return 0;
	}

	inline int Image::GetTilesetWidth() const
	{
		if (IsTileset())
			return tileset->tileWidth;
		else
			return 0;
	}

}
}

#endif
