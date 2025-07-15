#include "InputField.h"
#include "NECROEngine.h"

namespace NECRO
{
namespace Client
{
	void InputField::Init(SDL_Rect sRct, SDL_Rect dRct, const std::string& s, Image* im, Image* actIm, int tLimit)
	{
		m_rect = sRct;
		m_dstRect = dRct;
		m_str = s;
		m_img = im;
		m_focusedImage = actIm;
		m_textLimit = tLimit;

		// Default font
		m_font = engine.GetAssetsManager().GetFont("defaultFont");
	}

	void InputField::Draw()
	{
		Renderer& r = engine.GetRenderer();

		// Draw correct background
		r.DrawImageDirectly(m_isFocused ? m_focusedImage->GetSrc() : m_img->GetSrc(), &m_rect, &m_dstRect);

		// Draw the only visible portion of the text inside the textfield
		int strSize = m_str.size();

		if (strSize > 0)
		{
			// Offset to draw the string within the bounds of the text field
			int w = 0, h = 0;
			TTF_SizeText(m_font, m_str.c_str(), &w, &h);

			// MaxWidth is the width of the inputfield in pixels
			int maxWidth = m_rect.w;

			// A substring will be adjusted until it fits the maximum width
			std::string visibleStr = m_str;
			int visibleWidth = w;

			// Until the visible width is greater than the maxwidth, remove a character and recalculate
			while (visibleWidth > maxWidth)
			{
				visibleStr = visibleStr.substr(1); // Remove the first character
				TTF_SizeText(m_font, visibleStr.c_str(), &visibleWidth, &h);
			}

			// Draw text inside the field img
			r.DrawTextDirectly(m_font, visibleStr.c_str(), m_dstRect.x + m_xOffset, m_dstRect.y, m_color);
		}
	}

	void InputField::SetFocused(bool f)
	{
		m_isFocused = f;

		if (m_isFocused)
		{
			// Reset the string
			m_str.clear();
			engine.GetInput().SetCurInputField(this);
		}
		else
			engine.GetInput().SetCurInputField(nullptr);
	}

}
}
