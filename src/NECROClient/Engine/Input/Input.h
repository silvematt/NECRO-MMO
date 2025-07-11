#ifndef NECROINPUT_H
#define NECROINPUT_H

#include "SDL.h"
#include "Vector2.h"
#include "InputField.h"

namespace NECRO
{
namespace Client
{
	class Input
	{
	private:
		int				mouseX;
		int				mouseY;
		int				oldMouseX;
		int				oldMouseY;
		int				mouseButtons[3];
		int				prevMouseButtons[3];
		int				mouseScroll;

		int				mouseMotionX;
		int				mouseMotionY;

		Uint8* keys;
		Uint8* prevKeys;
		int				numKeys;

		InputField* curInputField;

	public:
		~Input();

		int				GetMouseX() const;
		int				GetMouseY() const;
		int				GetMouseScroll() const;
		int				GetMouseHeld(SDL_Scancode button) const;
		int				GetMouseDown(SDL_Scancode button) const;
		int				GetMouseUp(SDL_Scancode button) const;
		Vector2			GetMouseMotionThisFrame();
		int				GetKeyHeld(SDL_Scancode key) const;
		int				GetKeyDown(SDL_Scancode key) const;
		int				GetKeyUp(SDL_Scancode key) const;
		int				Init();
		void			Handle();
		void			SetCurInputField(InputField* i);

	};

	inline int Input::GetMouseX() const
	{
		return mouseX;
	}

	inline int Input::GetMouseY() const
	{
		return mouseY;
	}

	inline int Input::GetMouseScroll() const
	{
		return mouseScroll;
	}

	inline void Input::SetCurInputField(InputField* i)
	{
		curInputField = i;

		if (curInputField)
			SDL_StartTextInput();
		else
			SDL_StopTextInput();
	}

}
}

#endif
