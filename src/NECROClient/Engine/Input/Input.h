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
		int				m_mouseX;
		int				m_mouseY;
		int				m_oldMouseX;
		int				m_oldMouseY;
		int				m_mouseButtons[3];
		int				m_prevMouseButtons[3];
		int				m_mouseScroll;
						
		int				m_mouseMotionX;
		int				m_mouseMotionY;

		Uint8*		m_keys;
		Uint8*		m_prevKeys;
		int			m_numKeys;

		InputField* m_curInputField;

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
		return m_mouseX;
	}

	inline int Input::GetMouseY() const
	{
		return m_mouseY;
	}

	inline int Input::GetMouseScroll() const
	{
		return m_mouseScroll;
	}

	inline void Input::SetCurInputField(InputField* i)
	{
		m_curInputField = i;

		if (m_curInputField)
			SDL_StartTextInput();
		else
			SDL_StopTextInput();
	}

}
}

#endif
