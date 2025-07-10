#include "Input.h"
#include "NECROClient.h"

NECROInput::~NECROInput() 
{
	delete[] keys;
	delete[] prevKeys;
}

//--------------------------------------
// Initialize
//--------------------------------------
int NECROInput::Init()
{
	oldMouseX = oldMouseY = 0;

	const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
	const Uint8* keyboard = SDL_GetKeyboardState(&numKeys);

	keys = new Uint8[numKeys];
	prevKeys = new Uint8[numKeys];

	// Set mouse buttons
	for (int i = 1; i <= 3; i++) 
	{
		prevMouseButtons[i - 1] = 0;
		mouseButtons[i - 1] = mouseState & SDL_BUTTON(i);
	}

	// Initialize key state
	memcpy(keys, keyboard, sizeof(keys[0]) * numKeys);
	memcpy(prevKeys, keys, sizeof(prevKeys[0]) * numKeys);

	return 0;
}

//--------------------------------------
// Handles Input by polling events
//--------------------------------------
void NECROInput::Handle()
{
	mouseScroll = 0; // Mouse scroll must be reset

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
				mouseScroll = e.wheel.y;
				break;

			case SDL_MOUSEMOTION:
				SDL_SetRelativeMouseMode(SDL_TRUE);
				mouseMotionX = e.motion.xrel;
				mouseMotionY = e.motion.yrel;
				SDL_SetRelativeMouseMode(SDL_FALSE);
				break;

			case SDL_TEXTINPUT:
				if (curInputField)
				{
					// Check length and append text
					if(curInputField->GetTextLimit() == 0 ||
						curInputField->str.size() < curInputField->GetTextLimit())
					curInputField->str += e.text.text;
				}
				break;

			case SDL_KEYDOWN:
				if (curInputField)
				{
					// Backspace
					if (e.key.keysym.sym == SDLK_BACKSPACE && curInputField->str.length() > 0)
					{
						curInputField->str.pop_back();
					}
					// CTRL + c
					else if (e.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL)
					{
						SDL_SetClipboardText(curInputField->str.c_str());
					}
					// CTRL + v
					else if (e.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL)
					{
						char* tempText = SDL_GetClipboardText();
						curInputField->str = tempText;
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

	oldMouseX = mouseX;
	oldMouseY = mouseY;

	const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
	const Uint8* keyboard = SDL_GetKeyboardState(&numKeys);

	memcpy(prevKeys, keys, sizeof(prevKeys[0]) * numKeys);
	memcpy(keys, keyboard, sizeof(keys[0]) * numKeys);

	// Set mouse buttons
	for (int i = 1; i <= 3; i++) 
	{
		prevMouseButtons[i - 1] = mouseButtons[i - 1];
		mouseButtons[i - 1] = mouseState & SDL_BUTTON(i);
	}
}

int NECROInput::GetMouseDown(SDL_Scancode button) const
{
	if (button < SDL_BUTTON_LEFT || button > SDL_BUTTON_RIGHT)
		return -1;

	return (mouseButtons[button - 1] & ~prevMouseButtons[button - 1]);
}

int NECROInput::GetMouseUp(SDL_Scancode button) const
{
	if (button < SDL_BUTTON_LEFT || button > SDL_BUTTON_RIGHT)
		return -1;

	return (~mouseButtons[button - 1] & prevMouseButtons[button - 1]);
}

int NECROInput::GetKeyHeld(SDL_Scancode key) const 
{
	if (key < 0 || key > numKeys)
		return -1;

	return keys[key];
}

int NECROInput::GetKeyDown(SDL_Scancode key) const 
{
	if (key < 0 || key > numKeys)
		return -1;

	return (keys[key] & ~prevKeys[key]);
}

int NECROInput::GetKeyUp(SDL_Scancode key) const
{
	if (key < 0 || key >= numKeys)
		return -1;

	return (!keys[key] & prevKeys[key]);
}

int NECROInput::GetMouseHeld(SDL_Scancode button) const
{
	if (button < SDL_BUTTON_LEFT || button > SDL_BUTTON_RIGHT)
		return -1;

	return mouseButtons[button - 1];
}

Vector2 NECROInput::GetMouseMotionThisFrame()
{
	Vector2 mm(mouseMotionX, mouseMotionY);
	mouseMotionX = mouseMotionY = 0;
	return mm;
}
