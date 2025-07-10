#ifndef NECROASSETS_MANAGER_H
#define NECROASSETS_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#include "Image.h"
#include "Prefab.h"

const int FONT_DEFAULT_PTSIZE = 24;

// TODO on AssetsManager:
// 1) Make sure ill-formed files are stopped from being loaded instead of trying to go on and throwing exceptions
class NECROAssetsManager
{
private:
	std::unordered_map<std::string, Image> images;
	std::unordered_map<std::string, TTF_Font*> fonts;
	std::unordered_map<std::string, Prefab> prefabs;
	std::unordered_map<std::string, Animator> animators;

private:
	void				LoadAllImages();
	void				LoadAllFonts();
	void				LoadAllPrefabs();
	void				LoadAllAnimators();

	SDL_Texture*		LoadSDLTexture(const char* file);

	bool				LoadImageAsset(const std::string& file, int xOffset, int yOffset, const std::string& shortname = std::string());	// Shortname will be the key of the ump if NOT empty
	bool				LoadTilesetImage(const std::string& filename, int xOffset, int yOffset, int tWidth, int tHeight, int tNumX, int tNumY, const std::string& shortname = std::string());
	bool				LoadImageWithDefinition(const std::string& file, const std::string& shortname = std::string());	// Shortname will be the key of the ump if NOT empty
	
	bool				LoadFont(const std::string& file, int ptsize, const std::string& shortname = std::string());
	bool				LoadPrefab(const std::string& file);
	bool				LoadAnimator(const std::string& file, const std::string& shortname = std::string());

public:
	int					Init();			// Load everything

	Image*				GetImage(const std::string& filename);
	TTF_Font*			GetFont(const std::string& filename);
	Prefab*				GetPrefab(const std::string& prefabName);
	Animator*			GetAnimator(const std::string& animName);
};

extern const char* IMGS_FOLDER;
extern const char* FONTS_FOLDER;
extern const char* PREFABS_FOLDER;
extern const char* ANIMATORS_FOLDER;

#endif
