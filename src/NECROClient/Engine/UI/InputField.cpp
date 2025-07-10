#include "InputField.h"
#include "NECROClient.h"

void InputField::Init(SDL_Rect sRct, SDL_Rect dRct, const std::string& s, Image* im, Image* actIm, int tLimit)
{
	rect = sRct;
	dstRect = dRct;
	str = s;
	img = im;
	focusedImage = actIm;
	textLimit = tLimit;

	// Default font
	font = engine.GetAssetsManager().GetFont("defaultFont");
}

void InputField::Draw()
{
	NECRORenderer& r = engine.GetRenderer();

	// Draw correct background
	r.DrawImageDirectly(isFocused ? focusedImage->GetSrc() : img->GetSrc(), &rect, &dstRect);

	// Draw the only visible portion of the text inside the textfield
	int strSize = str.size();

	if (strSize > 0)
	{
		// Offset to draw the string within the bounds of the text field
		int w = 0, h = 0;
		TTF_SizeText(font, str.c_str(), &w, &h);

		// MaxWidth is the width of the inputfield in pixels
		int maxWidth = rect.w; 

		// A substring will be adjusted until it fits the maximum width
		std::string visibleStr = str;
		int visibleWidth = w;

		// Until the visible width is greater than the maxwidth, remove a character and recalculate
		while (visibleWidth > maxWidth)
		{
			visibleStr = visibleStr.substr(1); // Remove the first character
			TTF_SizeText(font, visibleStr.c_str(), &visibleWidth, &h);
		}

		// Draw text inside the field img
		r.DrawTextDirectly(font, visibleStr.c_str(), dstRect.x + xOffset, dstRect.y, color);
	}
}

void InputField::SetFocused(bool f)
{
	isFocused = f;

	if (isFocused)
	{
		// Reset the string
		str.clear();
		engine.GetInput().SetCurInputField(this);
	}
	else
		engine.GetInput().SetCurInputField(nullptr);
}
