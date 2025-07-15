#ifndef NECROINPUT_FIELD_H
#define NECROINPUT_FIELD_H

#include <string>
#include "SDL.h"
#include "SDL_ttf.h"

#include "Image.h"

namespace NECRO
{
namespace Client
{
	//----------------------------------------------------------------------------------------------
	// Selectable and writable Input Field displayed as UI element
	//----------------------------------------------------------------------------------------------
	class InputField
	{
		SDL_Rect	m_rect = { 0, 0, 0, 0 };
		SDL_Rect	m_dstRect = { 0, 0, 0, 0 };

		Image*		m_img = nullptr;
		Image*		m_focusedImage = nullptr;
		TTF_Font*	m_font = nullptr;
		SDL_Color	m_color = { 255, 255, 255, 255 };

		bool	m_isFocused = false;
		int		m_xOffset = 5;
		int		m_textLimit = 0;

	public:
		std::string m_str = "";


	public:
		InputField() = default;

		void		Init(SDL_Rect sRct, SDL_Rect dRct, const std::string& s, Image* im, Image* actIm, int tLimit = 0);

		void		Draw();
		void		SetFocused(bool f);
		int			GetTextLimit() const;
	};

	inline int InputField::GetTextLimit() const
	{
		return m_textLimit;
	}

}
}

#endif
