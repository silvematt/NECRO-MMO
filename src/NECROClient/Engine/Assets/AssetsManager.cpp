#include "AssetsManager.h"
#include "NECROEngine.h"

namespace NECRO
{
namespace Client
{
	//-------------------------------------------------
	// Initializing the AssetsManager loads everything
	//-------------------------------------------------
	int AssetsManager::Init()
	{
		// Load the fallback image, required for the AssetsManager to work properly
		if (LoadImageAsset("null_img.png", 0, 0))
		{
			LoadAllFonts(); // TODO: Fonts can be loaded only if needed as well as images, animators and prefabs, and we can have a default fallback font like the null_img.png
			return 0;
		}

		return 1;
	}

	//----------------------------------------------------------------------------
	// Load all the images directly (Unused now, images are loaded when necessary)
	//----------------------------------------------------------------------------
	void AssetsManager::LoadAllImages()
	{
		// General

		//LoadImage("tile.png", 0, 0);
		//LoadImage("tile_highlighted.png", 0, 0);
		//LoadImage("tree.png", -15, -5);
		//LoadImage("tile_debug_coll.png", 0, 0);

		// Player tilesets
		//LoadTilesetImage("player_war_idle.png", -30, 12, 128, 128, 1, 8);
		//LoadTilesetImage("player_war_run.png", -30, 12, 128, 128, 12, 8);
		//LoadTilesetImage("player_war_aim_stand.png", -30, 12, 128, 128, 1, 8);
		//LoadTilesetImage("player_war_aim_walk.png", -30, 12, 128, 128, 11, 8);
		//LoadTilesetImage("player_war_aim_strafe.png", -30, 12, 128, 128, 11, 8);
		//LoadTilesetImage("campfire01.png", 0, 0, 46, 46, 4, 1);
	}

	//-------------------------------------------------
	// Load all the fonts 
	//-------------------------------------------------
	void AssetsManager::LoadAllFonts()
	{
		LoadFont("montserrat.regular.ttf", FONT_DEFAULT_PTSIZE, "defaultFont");
	}


	// Unused now, prefabs are loaded when necessary
	void AssetsManager::LoadAllPrefabs()
	{
		//LoadPrefab("tree01.nprfb");
		//LoadPrefab("campfire01.nprfb");
	}

	// Unused now, animators are loaded when necessary
	void AssetsManager::LoadAllAnimators()
	{
		//LoadAnimator("player_war.nanim");
		//LoadAnimator("campfire01.nanim");
	}

	//-------------------------------------------------
	// Directly loads an Image 
	//-------------------------------------------------
	bool AssetsManager::LoadImageAsset(const std::string& filename, int xOffset, int yOffset, const std::string& shortname)
	{
		std::string fullPath = IMGS_FOLDER;
		fullPath += filename;

		Image img(LoadSDLTexture(fullPath.c_str()), xOffset, yOffset);
		if (img.GetSrc() != NULL)
		{
			m_images.insert({ shortname.empty() ? filename : shortname, std::move(img) });
			return true;
		}

		return false;
	}

	//-------------------------------------------------
	// Directly loads a Tileset Image 
	//-------------------------------------------------
	bool AssetsManager::LoadTilesetImage(const std::string& filename, int xOffset, int yOffset, int tWidth, int tHeight, int tNumX, int tNumY, const std::string& shortname)
	{
		std::string fullPath = IMGS_FOLDER;
		fullPath += filename;

		Image img(LoadSDLTexture(fullPath.c_str()), xOffset, yOffset, tWidth, tHeight, tNumX, tNumY);
		if (img.GetSrc() != NULL)
		{
			m_images.insert({ shortname.empty() ? filename : shortname, std::move(img) });
			return true;
		}

		return false;
	}

	//-------------------------------------------------------------------------------------
	// Loads an Image looking for its definition file 
	// It's the default way of loading images and it's called after a GetImage() call fails
	//-------------------------------------------------------------------------------------
	bool AssetsManager::LoadImageWithDefinition(const std::string& filename, const std::string& shortname)
	{
		std::string fullPath = IMGS_FOLDER;
		fullPath += filename;

		Image img(LoadSDLTexture(fullPath.c_str()), filename);
		if (img.GetSrc() != NULL)
		{
			m_images.insert({ shortname.empty() ? filename : shortname, std::move(img) });
			return true;
		}

		return false;
	}

	//-------------------------------------------------
	// Loads a single font into the fonts map
	//-------------------------------------------------
	bool AssetsManager::LoadFont(const std::string& filename, int ptsize, const std::string& shortname)
	{
		std::string fullPath = FONTS_FOLDER;
		fullPath += filename;

		TTF_Font* font = nullptr;

		font = TTF_OpenFont(fullPath.c_str(), ptsize);

		if (font != nullptr)
		{
			m_fonts.insert({ shortname.empty() ? filename : shortname, std::move(font) });
			return true;
		}
		else
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "AssetsManager: Failed to LoadFont(%s)\n", filename.c_str());
		}

		return false;
	}

	//-------------------------------------------------
	// Loads a single animator in the animators map
	//-------------------------------------------------
	bool AssetsManager::LoadAnimator(const std::string& filename, const std::string& shortname)
	{
		std::string fullPath = ANIMATORS_FOLDER;
		fullPath += filename;

		Animator a; // animators.insert makes a copy, that's why we can define this here as a local member
		if (a.LoadFromFile(fullPath))
		{
			m_animators.insert({ shortname.empty() ? filename : shortname, std::move(a) });
			return true;
		}

		return false;
	}

	//-------------------------------------------------
	// Loads a single prefab in the prefabs map
	//-------------------------------------------------
	bool AssetsManager::LoadPrefab(const std::string& filename)
	{
		std::string fullPath = PREFABS_FOLDER;
		fullPath += filename;

		Prefab p;
		if (p.LoadFromFile(fullPath))
		{
			m_prefabs.insert({ p.GetName(), std::move(p) });
			return true;
		}

		return false;
	}

	//--------------------------------------------------------------------------
	// Gets an Image from the images map and attempts to load it if not found
	//--------------------------------------------------------------------------
	Image* AssetsManager::GetImage(const std::string& filename)
	{
		// Check if an entity initialized is without any image by choice
		// This allows us to skip the search and avoid the LogWarn
		if (filename == "NULL")
			return &m_images.at("null_img.png");

		auto it = m_images.find(filename);
		if (it != m_images.end())
		{
			return &it->second;
		}

		SDL_LogWarn(SDL_LOG_PRIORITY_WARN, "GetImage: %s not found! Attempting to load it...", filename.c_str());

		if (LoadImageWithDefinition(filename))
		{
			// Return it
			it = m_images.find(filename);
			if (it != m_images.end())
			{
				return &it->second;
			}
		}

		return &m_images.at("null_img.png");
	}


	//-------------------------------------------------
	// Returns an TTF_Font*
	//-------------------------------------------------
	TTF_Font* AssetsManager::GetFont(const std::string& filename)
	{
		auto it = m_fonts.find(filename);
		if (it != m_fonts.end())
		{
			return it->second;
		}

		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GetFont: %s not found!", filename.c_str());
		return nullptr;
	}

	//--------------------------------------------------------------------------
	// Gets a Prefab from the prefabs map and attempts to load it if not found
	//--------------------------------------------------------------------------
	Prefab* AssetsManager::GetPrefab(const std::string& prefabName)
	{
		// Search for the prefab
		auto it = m_prefabs.find(prefabName);
		if (it != m_prefabs.end())
		{
			return &it->second;
		}

		// If it wasn't found, attempt to load it
		SDL_LogWarn(SDL_LOG_PRIORITY_WARN, "GetPrefab: %s not found! Attempting to load it...", prefabName.c_str());
		if (LoadPrefab(prefabName + ".nprfb"))
		{
			// Try the search again
			it = m_prefabs.find(prefabName);
			if (it != m_prefabs.end())
			{
				return &it->second;
			}
		}

		return nullptr;
	}

	//-------------------------------------------------------------------------------
	// Gets an Animator from the animators map and attempts to load it if not found
	//-------------------------------------------------------------------------------
	Animator* AssetsManager::GetAnimator(const std::string& animName)
	{
		auto it = m_animators.find(animName);
		if (it != m_animators.end())
		{
			return &it->second;
		}

		// If it wasn't found, attempt to load it
		SDL_LogWarn(SDL_LOG_PRIORITY_WARN, "GetAnimator: %s not found! Attempting to load it...", animName.c_str());
		if (LoadAnimator(animName))
		{
			// Try the search again
			it = m_animators.find(animName);
			if (it != m_animators.end())
			{
				return &it->second;
			}
		}

		return nullptr;
	}


	//-------------------------------------------------------
	// Loads and returns an SDL_Texture using IMG_LoadTexture
	//-------------------------------------------------------
	SDL_Texture* AssetsManager::LoadSDLTexture(const char* filename)
	{
		SDL_Texture* texture;

		texture = IMG_LoadTexture(engine.GetRenderer().GetInnerRenderer(), filename);

		if (texture != NULL) // IMG_LoadTexture returns NULL on failure
		{
			return texture;
		}
		else
		{
			LOG_ERROR("AssetsManager: Failed to LoadImage({})", filename);
			return NULL;
		}
	}

}
}
