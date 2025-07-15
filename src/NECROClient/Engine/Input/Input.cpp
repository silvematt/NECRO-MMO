#include "Input.h"
#include "NECROEngine.h"

namespace NECRO
{
namespace Client
{
	Input::~Input()
	{
		delete[] m_keys;
		delete[] m_prevKeys;
	}

	//--------------------------------------
	// Initialize
	//--------------------------------------
	int Input::Init()
	{
		m_oldMouseX = m_oldMouseY = 0;

		const Uint32 mouseState = SDL_GetMouseState(&m_mouseX, &m_mouseY);
		const Uint8* keyboard = SDL_GetKeyboardState(&m_numKeys);

		m_keys = new Uint8[m_numKeys];
		m_prevKeys = new Uint8[m_numKeys];

		// Set mouse buttons
		for (int i = 1; i <= 3; i++)
		{
			m_prevMouseButtons[i - 1] = 0;
			m_mouseButtons[i - 1] = mouseState & SDL_BUTTON(i);
		}

		// Initialize key state
		memcpy(m_keys, keyboard, sizeof(m_keys[0]) * m_numKeys);
		memcpy(m_prevKeys, m_keys, sizeof(m_prevKeys[0]) * m_numKeys);

		return 0;
	}

	//--------------------------------------
	// Handles Input by polling events
	//--------------------------------------
	void Input::Handle()
	{
		m_mouseScroll = 0; // Mouse scroll must be reset

		SDL_Event e;

		//Handle events
		while (SDL_PollEvent(&e) != 0)
		{
			switch (e.type)
			{
			case SDL_QUIT:
				engine.Stop();
				break;

			case SDL_MOUSEWHEEL:
				m_mouseScroll = e.wheel.y;
				break;

			case SDL_MOUSEMOTION:
				SDL_SetRelativeMouseMode(SDL_TRUE);
				m_mouseMotionX = e.motion.xrel;
				m_mouseMotionY = e.motion.yrel;
				SDL_SetRelativeMouseMode(SDL_FALSE);
				break;

			case SDL_TEXTINPUT:
				if (m_curInputField)
				{
					// Check length and append text
					if (m_curInputField->GetTextLimit() == 0 ||
						m_curInputField->m_str.size() < m_curInputField->GetTextLimit())
						m_curInputField->m_str += e.text.text;
				}
				break;

			case SDL_KEYDOWN:
				if (m_curInputField)
				{
					// Backspace
					if (e.key.keysym.sym == SDLK_BACKSPACE && m_curInputField->m_str.length() > 0)
					{
						m_curInputField->m_str.pop_back();
					}
					// CTRL + c
					else if (e.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL)
					{
						SDL_SetClipboardText(m_curInputField->m_str.c_str());
					}
					// CTRL + v
					else if (e.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL)
					{
						char* tempText = SDL_GetClipboardText();
						m_curInputField->m_str = tempText;
						SDL_free(tempText);
					}
				}

				if (e.key.keysym.sym == SDLK_F12)
				{
					engine.GetRenderer().SetExportThisFrame();
					LOG_INFO("Took a screenshot of render targets and final image, saved in the.exe location");
				}
				break;
			}
		}

		m_oldMouseX = m_mouseX;
		m_oldMouseY = m_mouseY;

		const Uint32 mouseState = SDL_GetMouseState(&m_mouseX, &m_mouseY);
		const Uint8* keyboard = SDL_GetKeyboardState(&m_numKeys);

		memcpy(m_prevKeys, m_keys, sizeof(m_prevKeys[0]) * m_numKeys);
		memcpy(m_keys, keyboard, sizeof(m_keys[0]) * m_numKeys);

		// Set mouse buttons
		for (int i = 1; i <= 3; i++)
		{
			m_prevMouseButtons[i - 1] = m_mouseButtons[i - 1];
			m_mouseButtons[i - 1] = mouseState & SDL_BUTTON(i);
		}
	}

	int Input::GetMouseDown(SDL_Scancode button) const
	{
		if (button < SDL_BUTTON_LEFT || button > SDL_BUTTON_RIGHT)
			return -1;

		return (m_mouseButtons[button - 1] & ~m_prevMouseButtons[button - 1]);
	}

	int Input::GetMouseUp(SDL_Scancode button) const
	{
		if (button < SDL_BUTTON_LEFT || button > SDL_BUTTON_RIGHT)
			return -1;

		return (~m_mouseButtons[button - 1] & m_prevMouseButtons[button - 1]);
	}

	int Input::GetKeyHeld(SDL_Scancode key) const
	{
		if (key < 0 || key > m_numKeys)
			return -1;

		return m_keys[key];
	}

	int Input::GetKeyDown(SDL_Scancode key) const
	{
		if (key < 0 || key > m_numKeys)
			return -1;

		return (m_keys[key] & ~m_prevKeys[key]);
	}

	int Input::GetKeyUp(SDL_Scancode key) const
	{
		if (key < 0 || key >= m_numKeys)
			return -1;

		return (!m_keys[key] & m_prevKeys[key]);
	}

	int Input::GetMouseHeld(SDL_Scancode button) const
	{
		if (button < SDL_BUTTON_LEFT || button > SDL_BUTTON_RIGHT)
			return -1;

		return m_mouseButtons[button - 1];
	}

	Vector2 Input::GetMouseMotionThisFrame()
	{
		Vector2 mm(m_mouseMotionX, m_mouseMotionY);
		m_mouseMotionX = m_mouseMotionY = 0;
		return mm;
	}

}
}
