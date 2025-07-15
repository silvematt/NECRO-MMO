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

namespace NECRO
{
namespace Client
{
	inline constexpr int FONT_DEFAULT_PTSIZE = 24;

	inline constexpr const char* IMGS_FOLDER = "Data/imgs/";
	inline constexpr const char* FONTS_FOLDER = "Data/fonts/";
	inline constexpr const char* PREFABS_FOLDER = "Data/prefabs/";
	inline constexpr const char* ANIMATORS_FOLDER = "Data/prefabs/animators/";

	// TODO on AssetsManager:
	// 1) Make sure ill-formed files are stopped from being loaded instead of trying to go on and throwing exceptions
	class AssetsManager
	{
	private:
		std::unordered_map<std::string, Image>		m_images;
		std::unordered_map<std::string, TTF_Font*>	m_fonts;
		std::unordered_map<std::string, Prefab>		m_prefabs;
		std::unordered_map<std::string, Animator>	m_animators;

	private:
		void				LoadAllImages();
		void				LoadAllFonts();
		void				LoadAllPrefabs();
		void				LoadAllAnimators();

		SDL_Texture* LoadSDLTexture(const char* file);

		bool				LoadImageAsset(const std::string& file, int xOffset, int yOffset, const std::string& shortname = std::string());	// Shortname will be the key of the ump if NOT empty
		bool				LoadTilesetImage(const std::string& filename, int xOffset, int yOffset, int tWidth, int tHeight, int tNumX, int tNumY, const std::string& shortname = std::string());
		bool				LoadImageWithDefinition(const std::string& file, const std::string& shortname = std::string());	// Shortname will be the key of the ump if NOT empty

		bool				LoadFont(const std::string& file, int ptsize, const std::string& shortname = std::string());
		bool				LoadPrefab(const std::string& file);
		bool				LoadAnimator(const std::string& file, const std::string& shortname = std::string());

	public:
		int					Init();			// Load everything

		Image* GetImage(const std::string& filename);
		TTF_Font* GetFont(const std::string& filename);
		Prefab* GetPrefab(const std::string& prefabName);
		Animator* GetAnimator(const std::string& animName);
	};

}
}

#endif
