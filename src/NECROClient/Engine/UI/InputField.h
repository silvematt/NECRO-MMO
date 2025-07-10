#ifndef NECROINPUT_FIELD_H
#define NECROINPUT_FIELD_H

#include <string>
#include "SDL.h"
#include "SDL_ttf.h"

#include "Image.h"


//----------------------------------------------------------------------------------------------
// Selectable and writable Input Field displayed as UI element
//----------------------------------------------------------------------------------------------
class InputField
{
	SDL_Rect rect = { 0, 0, 0, 0 };
	SDL_Rect dstRect = { 0, 0, 0, 0 };

	Image* img = nullptr;
	Image* focusedImage = nullptr;
	TTF_Font* font = nullptr;
	SDL_Color color = { 255, 255, 255, 255 };

	bool isFocused = false;
	int xOffset = 5;
	int textLimit = 0;

public:
	std::string str = "";


public:
	InputField() = default;

	void		Init(SDL_Rect sRct, SDL_Rect dRct, const std::string& s, Image* im, Image* actIm, int tLimit = 0);

	void		Draw();
	void		SetFocused(bool f);
	int			GetTextLimit() const;
};

inline int InputField::GetTextLimit() const
{
	return textLimit;
}

#endif
