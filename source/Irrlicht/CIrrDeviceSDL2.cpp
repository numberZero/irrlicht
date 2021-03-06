// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// linux/joystick.h includes linux/input.h, which #defines values for various KEY_FOO keys.
// These override the irr::KEY_FOO equivalents, which stops key handling from working.
// As a workaround, defining _INPUT_H stops linux/input.h from being included; it
// doesn't actually seem to be necessary except to pull in sys/ioctl.h.
#define _INPUT_H
#define _UAPI_INPUT_EVENT_CODES_H

#include "CIrrDeviceSDL2.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <time.h>
#include "IEventReceiver.h"
#include "ISceneManager.h"
#include "IGUIEnvironment.h"
#include "os.h"
#include "CTimer.h"
#include "irrString.h"
#include "Keycodes.h"
#include "COSOperator.h"
#include "CColorConverter.h"
#include "SIrrCreationParameters.h"
#include "IGUISpriteBank.h"
#include <utf8.h>

#include <fcntl.h>
#include <unistd.h>

#ifdef __FreeBSD__
#include <sys/joystick.h>
#else

#include <sys/ioctl.h> // Would normally be included in linux/input.h
#include <linux/joystick.h>
#undef _INPUT_H
#undef _UAPI_INPUT_EVENT_CODES_H
#endif

namespace irr
{
	namespace video
	{
		IVideoDriver* createOpenGLDriver(const SIrrlichtCreationParameters& params, io::IFileSystem* io);
		IVideoDriver* createOGLES1Driver(const SIrrlichtCreationParameters& params, io::IFileSystem* io);
		IVideoDriver* createOGLES2Driver(const SIrrlichtCreationParameters& params, io::IFileSystem* io);
	}
} // end namespace irr

namespace irr
{

const char* wmDeleteWindow = "WM_DELETE_WINDOW";

//! constructor
CIrrDeviceSDL2::CIrrDeviceSDL2(const SIrrlichtCreationParameters& param)
	: CIrrDeviceStub(param),
	window(0),
	Context(0),
	Width(param.WindowSize.Width), Height(param.WindowSize.Height),
	WindowHasFocus(false), WindowMinimized(false)
{
	#ifdef _DEBUG
	setDebugName("CIrrDeviceSDL2");
	#endif

	// print version, distribution etc.
	// thx to LynxLuna for pointing me to the uname function
	core::stringc linuxversion;
	struct utsname LinuxInfo;
	uname(&LinuxInfo);

	linuxversion += LinuxInfo.sysname;
	linuxversion += " ";
	linuxversion += LinuxInfo.release;
	linuxversion += " ";
	linuxversion += LinuxInfo.version;
	linuxversion += " ";
	linuxversion += LinuxInfo.machine;

	Operator = new COSOperator(linuxversion);
	os::Printer::log(linuxversion.c_str(), ELL_INFORMATION);

	// create keymap
	createKeyMap();

	// create window
	if (CreationParams.DriverType != video::EDT_NULL)
	{
		SDL_Init(SDL_INIT_VIDEO);

		// create the window, only if we do not use the null device
		if (!createWindow())
			return;
	}

	// create cursor control
	CursorControl = new CCursorControl(this, CreationParams.DriverType == video::EDT_NULL);

	// create driver
	createDriver();

	if (!VideoDriver)
		return;

	createGUIAndScene();
}


//! destructor
CIrrDeviceSDL2::~CIrrDeviceSDL2()
{
	// Disable cursor (it is drop'ed in stub)
	if (CursorControl)
	{
		CursorControl->setVisible(false);
		static_cast<CCursorControl*>(CursorControl)->clearCursors();
	}

	// Must free OpenGL textures etc before destroying context, so can't wait for stub destructor
	if ( GUIEnvironment )
	{
		GUIEnvironment->drop();
		GUIEnvironment = NULL;
	}
	if ( SceneManager )
	{
		SceneManager->drop();
		SceneManager = NULL;
	}
	if ( VideoDriver )
	{
		VideoDriver->drop();
		VideoDriver = NULL;
	}

	if (Context)
	{
		SDL_GL_DeleteContext(Context);
	}

	// Reset fullscreen resolution change
	switchToFullscreen(true);

	SDL_DestroyWindow(window);

	for (u32 joystick = 0; joystick < ActiveJoysticks.size(); ++joystick)
	{
		if (ActiveJoysticks[joystick].fd >= 0)
		{
			close(ActiveJoysticks[joystick].fd);
		}
	}
}

bool CIrrDeviceSDL2::switchToFullscreen(bool reset)
{
	if (!CreationParams.Fullscreen)
		return true;
	if (reset)
	{
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		return true;
	}

	getVideoModeList();

	return CreationParams.Fullscreen;
}


bool CIrrDeviceSDL2::createWindow()
{
	Uint32 windowFlags = SDL_WINDOW_RESIZABLE;
	if (CreationParams.Fullscreen)
		windowFlags |= SDL_WINDOW_FULLSCREEN;

	windowFlags |= SDL_WINDOW_OPENGL;
	if (CreationParams.Bits==16)
	{
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 4 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 4 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 4 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, CreationParams.WithAlphaChannel?1:0 );
	}
	else
	{
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, CreationParams.WithAlphaChannel?8:0 );
	}
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, CreationParams.ZBufferBits);
	if (CreationParams.Doublebuffer)
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	if (CreationParams.Stereobuffer)
		SDL_GL_SetAttribute( SDL_GL_STEREO, 1 );
	if (CreationParams.AntiAlias>1)
	{
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, CreationParams.AntiAlias );
	}

	switch (CreationParams.DriverType) {
		case video::EDT_OPENGL:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
			break;
		case video::EDT_OGLES1:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
			break;
		case video::EDT_OGLES2:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			break;
		default:;
	}

	window = SDL_CreateWindow(
		"Irrlicht (title not set)",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		CreationParams.WindowSize.Width,
		CreationParams.WindowSize.Height,
		windowFlags
	);
	if (!window)
	{
		os::Printer::log("Could not create window", SDL_GetError(), ELL_ERROR);
		return false;
	}
	WindowMinimized = false;
	WindowHasFocus = true;
	Context = SDL_GL_CreateContext(window);
	if (!Context) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		Context = SDL_GL_CreateContext(window);
		if (Context)
			return true;
		os::Printer::log("Could not create context", SDL_GetError(), ELL_ERROR);
		SDL_DestroyWindow(window);
		return false;
	}
	return true;
}


//! create the driver
void CIrrDeviceSDL2::createDriver()
{
	switch(CreationParams.DriverType)
	{
	case video::EDT_OPENGL:
		#ifdef _IRR_COMPILE_WITH_OPENGL_
		VideoDriver = video::createOpenGLDriver(CreationParams, FileSystem);
		#else
		os::Printer::log("No OpenGL support compiled in.", ELL_ERROR);
		#endif
		break;

	case video::EDT_OGLES1:
		#ifdef _IRR_COMPILE_WITH_OGLES1_
		VideoDriver = video::createOGLES1Driver(CreationParams, FileSystem);
		#else
		os::Printer::log("No OpenGL ES 1 support compiled in.", ELL_ERROR);
		#endif
		break;

	case video::EDT_OGLES2:
		#ifdef _IRR_COMPILE_WITH_OGLES2_
		VideoDriver = video::createOGLES2Driver(CreationParams, FileSystem);
		#else
		os::Printer::log("No OpenGL ES 2 support compiled in.", ELL_ERROR);
		#endif
		break;

	case video::EDT_NULL:
		VideoDriver = video::createNullDriver(FileSystem, CreationParams.WindowSize);
		break;

	default:
		os::Printer::log("Unable to create video driver of unknown type.", ELL_ERROR);
		break;
	}
}

static E_MOUSE_BUTTON_STATE_MASK mouse_buttons_sdl_to_irr(u32 buttons) {
	u32 result = 0;
	if (buttons & SDL_BUTTON_LMASK)
		result |= EMBSM_LEFT;
	if (buttons & SDL_BUTTON_RMASK)
		result |= irr::EMBSM_RIGHT;
	if (buttons & SDL_BUTTON_MMASK)
		result |= irr::EMBSM_MIDDLE;
	if (buttons & SDL_BUTTON_X1MASK)
		result |= irr::EMBSM_EXTRA1;
	if (buttons & SDL_BUTTON_X2MASK)
		result |= irr::EMBSM_EXTRA2;
	return E_MOUSE_BUTTON_STATE_MASK(buttons);
}

//! runs the device. Returns false if device wants to be deleted
bool CIrrDeviceSDL2::run()
{
	os::Timer::tick();

	SEvent irrevent;
	SDL_Event SDL_event;

	while (!Close && SDL_PollEvent( &SDL_event ))
	{
		switch ( SDL_event.type )
		{
			case SDL_QUIT:
				Close = true;
				break;

			case SDL_USEREVENT:
				irrevent.EventType = irr::EET_USER_EVENT;
				irrevent.UserEvent.UserData1 = *(reinterpret_cast<s32*>(&SDL_event.user.data1));
				irrevent.UserEvent.UserData2 = *(reinterpret_cast<s32*>(&SDL_event.user.data2));
				postEventFromUser(irrevent);
				break;

			case SDL_MOUSEMOTION:
				if (SDL_event.motion.which == SDL_TOUCH_MOUSEID)
					break; // syntetic mouse event; the real one was SDL_FINGERMOTION
				irrevent.EventType = irr::EET_MOUSE_INPUT_EVENT;
				irrevent.MouseInput.Event = irr::EMIE_MOUSE_MOVED;
				MouseX = irrevent.MouseInput.X = SDL_event.motion.x;
				MouseY = irrevent.MouseInput.Y = SDL_event.motion.y;
				MouseButtonStates = irrevent.MouseInput.ButtonStates = mouse_buttons_sdl_to_irr(SDL_event.motion.state);
				irrevent.MouseInput.Shift = (KeyMode & KMOD_SHIFT) != 0;
				irrevent.MouseInput.Control = (KeyMode & KMOD_CTRL) != 0;

				postEventFromUser(irrevent);
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (SDL_event.button.which == SDL_TOUCH_MOUSEID)
					break; // syntetic mouse event; the real one was SDL_FINGER*
				irrevent.EventType = irr::EET_MOUSE_INPUT_EVENT;
				irrevent.MouseInput.X = SDL_event.button.x;
				irrevent.MouseInput.Y = SDL_event.button.y;
				irrevent.MouseInput.Shift = (KeyMode & KMOD_SHIFT) != 0;
				irrevent.MouseInput.Control = (KeyMode & KMOD_CTRL) != 0;

				irrevent.MouseInput.Event = irr::EMIE_MOUSE_MOVED;

				switch(SDL_event.button.button)
				{
				case SDL_BUTTON_LEFT:
					if (SDL_event.type == SDL_MOUSEBUTTONDOWN)
					{
						irrevent.MouseInput.Event = irr::EMIE_LMOUSE_PRESSED_DOWN;
						MouseButtonStates |= irr::EMBSM_LEFT;
					}
					else
					{
						irrevent.MouseInput.Event = irr::EMIE_LMOUSE_LEFT_UP;
						MouseButtonStates &= ~irr::EMBSM_LEFT;
					}
					break;

				case SDL_BUTTON_RIGHT:
					if (SDL_event.type == SDL_MOUSEBUTTONDOWN)
					{
						irrevent.MouseInput.Event = irr::EMIE_RMOUSE_PRESSED_DOWN;
						MouseButtonStates |= irr::EMBSM_RIGHT;
					}
					else
					{
						irrevent.MouseInput.Event = irr::EMIE_RMOUSE_LEFT_UP;
						MouseButtonStates &= ~irr::EMBSM_RIGHT;
					}
					break;

				case SDL_BUTTON_MIDDLE:
					if (SDL_event.type == SDL_MOUSEBUTTONDOWN)
					{
						irrevent.MouseInput.Event = irr::EMIE_MMOUSE_PRESSED_DOWN;
						MouseButtonStates |= irr::EMBSM_MIDDLE;
					}
					else
					{
						irrevent.MouseInput.Event = irr::EMIE_MMOUSE_LEFT_UP;
						MouseButtonStates &= ~irr::EMBSM_MIDDLE;
					}
					break;
				}

				irrevent.MouseInput.ButtonStates = MouseButtonStates;

				if (irrevent.MouseInput.Event != irr::EMIE_MOUSE_MOVED)
				{
					postEventFromUser(irrevent);

					if ( irrevent.MouseInput.Event >= EMIE_LMOUSE_PRESSED_DOWN && irrevent.MouseInput.Event <= EMIE_MMOUSE_PRESSED_DOWN )
					{
						u32 clicks = checkSuccessiveClicks(irrevent.MouseInput.X, irrevent.MouseInput.Y, irrevent.MouseInput.Event);
						if ( clicks == 2 )
						{
							irrevent.MouseInput.Event = (EMOUSE_INPUT_EVENT)(EMIE_LMOUSE_DOUBLE_CLICK + irrevent.MouseInput.Event-EMIE_LMOUSE_PRESSED_DOWN);
							postEventFromUser(irrevent);
						}
						else if ( clicks == 3 )
						{
							irrevent.MouseInput.Event = (EMOUSE_INPUT_EVENT)(EMIE_LMOUSE_TRIPLE_CLICK + irrevent.MouseInput.Event-EMIE_LMOUSE_PRESSED_DOWN);
							postEventFromUser(irrevent);
						}
					}
				}
				break;

			case SDL_KEYDOWN:
			case SDL_KEYUP:
				{
					SKeyMap mp;
					mp.SDLKey = SDL_event.key.keysym.sym;
					s32 idx = KeyMap.binary_search(mp);

					EKEY_CODE key;
					if (idx == -1)
						key = (EKEY_CODE)0;
					else
						key = (EKEY_CODE)KeyMap[idx].Win32Key;
					KeyMode = SDL_event.key.keysym.mod;

					irrevent.EventType = irr::EET_KEY_INPUT_EVENT;
					irrevent.KeyInput.Char = L'\0';
					irrevent.KeyInput.Key = key;
					irrevent.KeyInput.PressedDown = (SDL_event.type == SDL_KEYDOWN);
					irrevent.KeyInput.Shift = (SDL_event.key.keysym.mod & KMOD_SHIFT) != 0;
					irrevent.KeyInput.Control = (SDL_event.key.keysym.mod & KMOD_CTRL) != 0;
					postEventFromUser(irrevent);
				}
				break;

			// TODO events
// 			case SDL_LOCALECHANGED:

// 			case SDL_DISPLAYEVENT:
// 				break;

			case SDL_WINDOWEVENT:
				{
					switch (SDL_event.window.event) {
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							Width = SDL_event.window.data1;
							Height = SDL_event.window.data2;
							if (VideoDriver)
								VideoDriver->OnResize(core::dimension2d<u32>(Width, Height));
							break;

						case SDL_WINDOWEVENT_HIDDEN:
						case SDL_WINDOWEVENT_MINIMIZED:
							WindowMinimized = true;
							break;

						case SDL_WINDOWEVENT_SHOWN:
						case SDL_WINDOWEVENT_MAXIMIZED:
						case SDL_WINDOWEVENT_RESTORED:
							WindowMinimized = false;
							break;

						case SDL_WINDOWEVENT_ENTER:
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							WindowHasFocus = true;
							break;

						case SDL_WINDOWEVENT_LEAVE:
						case SDL_WINDOWEVENT_FOCUS_LOST:
							WindowHasFocus = false;
							break;

						case SDL_WINDOWEVENT_CLOSE: // handles alt+f4 etc.
							Close = true;
							break;
					}
				}
				break;

			case SDL_SYSWMEVENT:
				break;

			case SDL_TEXTEDITING:
				break;

			case SDL_TEXTINPUT:
				{
					irrevent.EventType = irr::EET_KEY_INPUT_EVENT;
					irrevent.KeyInput.Key = (EKEY_CODE)0;
					irrevent.KeyInput.Shift = (KeyMode & KMOD_SHIFT) != 0;
					irrevent.KeyInput.Control = (KeyMode & KMOD_CTRL) != 0;
					char* str = SDL_event.text.text;
					char* end = str + strnlen(str, sizeof(SDL_event.text.text));
					uint32_t cp = 0;
					for (char* str_i = str; utf8::internal::validate_next(str_i, end, cp) == utf8::internal::UTF8_OK; ) {
						irrevent.KeyInput.Char = cp;
						irrevent.KeyInput.PressedDown = true;
						postEventFromUser(irrevent);
						irrevent.KeyInput.PressedDown = false;
						postEventFromUser(irrevent);
					}
				}
				break;

			case SDL_KEYMAPCHANGED:
				break;

			case SDL_MOUSEWHEEL:
				{
					irrevent.EventType = irr::EET_MOUSE_INPUT_EVENT;
					irrevent.MouseInput.X = MouseX;
					irrevent.MouseInput.Y = MouseY;
					irrevent.MouseInput.Wheel = SDL_event.wheel.y;
					irrevent.MouseInput.Shift = (KeyMode & KMOD_SHIFT) != 0;
					irrevent.MouseInput.Control = (KeyMode & KMOD_CTRL) != 0;
					irrevent.MouseInput.ButtonStates = MouseButtonStates;
					irrevent.MouseInput.Event = irr::EMIE_MOUSE_WHEEL;
					postEventFromUser(irrevent);
				}
				break;

// 			case SDL_JOYAXISMOTION:
// 			case SDL_JOYBALLMOTION:
// 			case SDL_JOYHATMOTION:
// 			case SDL_JOYBUTTONDOWN:
// 			case SDL_JOYBUTTONUP:
// 			case SDL_JOYDEVICEADDED:
// 			case SDL_JOYDEVICEREMOVED:

			/* Game controller events */
// 			case SDL_CONTROLLERAXISMOTION:
// 			case SDL_CONTROLLERBUTTONDOWN:
// 			case SDL_CONTROLLERBUTTONUP:
// 			case SDL_CONTROLLERDEVICEADDED:
// 			case SDL_CONTROLLERDEVICEREMOVED:
// 			case SDL_CONTROLLERDEVICEREMAPPED:
// 			case SDL_CONTROLLERTOUCHPADDOWN:
// 			case SDL_CONTROLLERTOUCHPADMOTION:
// 			case SDL_CONTROLLERTOUCHPADUP:
// 			case SDL_CONTROLLERSENSORUPDATE:

			/* Touch events */
			case SDL_FINGERDOWN:
			case SDL_FINGERUP:
			case SDL_FINGERMOTION:
			{
				irrevent.EventType = irr::EET_TOUCH_INPUT_EVENT;
				if (SDL_event.tfinger.type == SDL_FINGERDOWN)
					irrevent.TouchInput.ID = addTouch(SDL_event.tfinger.touchId, SDL_event.tfinger.fingerId);
				else
					irrevent.TouchInput.ID = findTouch(SDL_event.tfinger.touchId, SDL_event.tfinger.fingerId);
				irrevent.TouchInput.X = SDL_event.tfinger.x * Width;
				irrevent.TouchInput.Y = SDL_event.tfinger.y * Height;
				irrevent.TouchInput.touchedCount = CurrentTouches.size();
				switch (SDL_event.tfinger.type) {
					case SDL_FINGERDOWN: irrevent.TouchInput.Event = irr::ETIE_PRESSED_DOWN; break;
					case SDL_FINGERUP: irrevent.TouchInput.Event = irr::ETIE_LEFT_UP; break;
					case SDL_FINGERMOTION: irrevent.TouchInput.Event = irr::ETIE_MOVED; break;
				}
				if (SDL_event.tfinger.type == SDL_FINGERUP)
					removeTouch(irrevent.TouchInput.ID);
				postEventFromUser(irrevent);
			}

			/* Gesture events */
// 			case SDL_DOLLARGESTURE:
// 			case SDL_DOLLARRECORD:
// 			case SDL_MULTIGESTURE:

			/* Clipboard events */
			case SDL_CLIPBOARDUPDATE:

			/* Drag and drop events */
// 			case SDL_DROPFILE:
// 			case SDL_DROPTEXT:
// 			case SDL_DROPBEGIN:
// 			case SDL_DROPCOMPLETE:

			/* Audio hotplug events */
// 			case SDL_AUDIODEVICEADDED:
// 			case SDL_AUDIODEVICEREMOVED:

			/* Sensor events */
// 			case SDL_SENSORUPDATE:

			/* Render events */
// 			case SDL_RENDER_TARGETS_RESET:
// 			case SDL_RENDER_DEVICE_RESET:

			// Mobile related:
// 			case SDL_APP_TERMINATING:
// 			case SDL_APP_LOWMEMORY:
// 			case SDL_APP_WILLENTERBACKGROUND:
// 			case SDL_APP_DIDENTERBACKGROUND:
// 			case SDL_APP_WILLENTERFOREGROUND:
// 			case SDL_APP_DIDENTERFOREGROUND:
				break;

		}
		// TODO: this part is to rewrite inspired by irrlicht SDL1 mapping
#if 0
			case ClientMessage:
				{
					char *atom = XGetAtomName(display, event.xclient.message_type);
					if (*atom == *wmDeleteWindow)
					{
						os::Printer::log("Quit message received.", ELL_INFORMATION);
						Close = true;
					}
					else
					{
						// we assume it's a user message
						irrevent.EventType = irr::EET_USER_EVENT;
						irrevent.UserEvent.UserData1 = (s32)event.xclient.data.l[0];
						irrevent.UserEvent.UserData2 = (s32)event.xclient.data.l[1];
						postEventFromUser(irrevent);
					}
					XFree(atom);
				}
				break;

			case SelectionRequest:
				{
					XEvent respond;
					XSelectionRequestEvent *req = &(event.xselectionrequest);
					if (  req->target == XA_STRING)
					{
						XChangeProperty (display,
								req->requestor,
								req->property, req->target,
								8, // format
								PropModeReplace,
								(unsigned char*) Clipboard.c_str(),
								Clipboard.size());
						respond.xselection.property = req->property;
					}
					else if ( req->target == X_ATOM_TARGETS )
					{
						long data[2];

						data[0] = X_ATOM_TEXT;
						data[1] = XA_STRING;

						XChangeProperty (display, req->requestor,
								req->property, req->target,
								8, PropModeReplace,
								(unsigned char *) &data,
								sizeof (data));
						respond.xselection.property = req->property;
					}
					else
					{
						respond.xselection.property= None;
					}
					respond.xselection.type= SelectionNotify;
					respond.xselection.display= req->display;
					respond.xselection.requestor= req->requestor;
					respond.xselection.selection=req->selection;
					respond.xselection.target= req->target;
					respond.xselection.time = req->time;
					XSendEvent (display, req->requestor,0,0,&respond);
					XFlush (display);
				}
				break;

			default:
				break;
			} // end switch
#endif
	} // end while

	if (!Close)
		pollJoysticks();

	return !Close;
}


size_t CIrrDeviceSDL2::findTouch(SDL_TouchID device, SDL_FingerID touch) {
	for (unsigned k = 0; k < CurrentTouches.size(); k++)
		if (CurrentTouches[k].device == device && CurrentTouches[k].touch == touch)
			return CurrentTouches[k].id;
	os::Printer::log("Touch not found by device/finger id", ELL_WARNING);
	return addTouch(device, touch);
}

size_t CIrrDeviceSDL2::addTouch(SDL_TouchID device, SDL_FingerID touch) {
	CurrentTouches.push_back({device, touch, ++LastTouchId});
	return LastTouchId;
}

void CIrrDeviceSDL2::removeTouch(size_t id) {
	for (unsigned k = 0; k < CurrentTouches.size(); k++)
		if (CurrentTouches[k].id == id) {
			CurrentTouches.erase(k);
			return;
		}
	os::Printer::log("Touch not found by internal id", ELL_ERROR);
}

//! Pause the current process for the minimum time allowed only to allow other processes to execute
void CIrrDeviceSDL2::yield()
{
	SDL_Delay(0);
}


//! Pause execution and let other processes to run for a specified amount of time.
void CIrrDeviceSDL2::sleep(u32 timeMs, bool pauseTimer=false)
{
	const bool wasStopped = Timer ? Timer->isStopped() : true;

	struct timespec ts;
	ts.tv_sec = (time_t) (timeMs / 1000);
	ts.tv_nsec = (long) (timeMs % 1000) * 1000000;

	if (pauseTimer && !wasStopped)
		Timer->stop();

	nanosleep(&ts, NULL);

	if (pauseTimer && !wasStopped)
		Timer->start();
}


//! sets the caption of the window
void CIrrDeviceSDL2::setWindowCaption(const wchar_t* text)
{
	if (CreationParams.DriverType == video::EDT_NULL)
		return;

	core::stringc textc = text;
	SDL_SetWindowTitle(window, textc.c_str());
}


//! presents a surface in the client area
bool CIrrDeviceSDL2::present(video::IImage* surface, void* windowId, core::rect<s32>* srcClip)
{
	SDL_Surface *sdlSurface = SDL_CreateRGBSurfaceFrom(
			surface->lock(), surface->getDimension().Width, surface->getDimension().Height,
			surface->getBitsPerPixel(), surface->getPitch(),
			surface->getRedMask(), surface->getGreenMask(), surface->getBlueMask(), surface->getAlphaMask());
	if (!sdlSurface)
		return false;
	SDL_SetSurfaceAlphaMod(sdlSurface, 0);
	SDL_SetColorKey(sdlSurface, 0, 0);
	sdlSurface->format->BitsPerPixel=surface->getBitsPerPixel();
	sdlSurface->format->BytesPerPixel=surface->getBytesPerPixel();
	if ((surface->getColorFormat()==video::ECF_R8G8B8) ||
			(surface->getColorFormat()==video::ECF_A8R8G8B8))
	{
		sdlSurface->format->Rloss=0;
		sdlSurface->format->Gloss=0;
		sdlSurface->format->Bloss=0;
		sdlSurface->format->Rshift=16;
		sdlSurface->format->Gshift=8;
		sdlSurface->format->Bshift=0;
		if (surface->getColorFormat()==video::ECF_R8G8B8)
		{
			sdlSurface->format->Aloss=8;
			sdlSurface->format->Ashift=32;
		}
		else
		{
			sdlSurface->format->Aloss=0;
			sdlSurface->format->Ashift=24;
		}
	}
	else if (surface->getColorFormat()==video::ECF_R5G6B5)
	{
		sdlSurface->format->Rloss=3;
		sdlSurface->format->Gloss=2;
		sdlSurface->format->Bloss=3;
		sdlSurface->format->Aloss=8;
		sdlSurface->format->Rshift=11;
		sdlSurface->format->Gshift=5;
		sdlSurface->format->Bshift=0;
		sdlSurface->format->Ashift=16;
	}
	else if (surface->getColorFormat()==video::ECF_A1R5G5B5)
	{
		sdlSurface->format->Rloss=3;
		sdlSurface->format->Gloss=3;
		sdlSurface->format->Bloss=3;
		sdlSurface->format->Aloss=7;
		sdlSurface->format->Rshift=10;
		sdlSurface->format->Gshift=5;
		sdlSurface->format->Bshift=0;
		sdlSurface->format->Ashift=15;
	}

	SDL_Surface* scr = (SDL_Surface* )windowId;
	if (scr)
	{
		if (srcClip)
		{
			SDL_Rect sdlsrcClip;
			sdlsrcClip.x = srcClip->UpperLeftCorner.X;
			sdlsrcClip.y = srcClip->UpperLeftCorner.Y;
			sdlsrcClip.w = srcClip->getWidth();
			sdlsrcClip.h = srcClip->getHeight();
			SDL_BlitSurface(sdlSurface, &sdlsrcClip, scr, NULL);
		}
		else
			SDL_BlitSurface(sdlSurface, NULL, scr, NULL);
	}

	SDL_FreeSurface(sdlSurface);
	surface->unlock();
	return (scr != 0);
}


//! notifies the device that it should close itself
void CIrrDeviceSDL2::closeDevice()
{
	Close = true;
}


//! returns if window is active. if not, nothing need to be drawn
bool CIrrDeviceSDL2::isWindowActive() const
{
	return (WindowHasFocus && !WindowMinimized);
}


//! returns if window has focus.
bool CIrrDeviceSDL2::isWindowFocused() const
{
	return WindowHasFocus;
}


//! returns if window is minimized.
bool CIrrDeviceSDL2::isWindowMinimized() const
{
	return WindowMinimized;
}


//! returns color format of the window.
video::ECOLOR_FORMAT CIrrDeviceSDL2::getColorFormat() const
{
		return CIrrDeviceStub::getColorFormat();
}


//! Sets if the window should be resizable in windowed mode.
void CIrrDeviceSDL2::setResizable(bool resize)
{
	SDL_SetWindowResizable(window, resize ? SDL_TRUE : SDL_FALSE);
}


//! Return pointer to a list with all video modes supported by the gfx adapter.
video::IVideoModeList* CIrrDeviceSDL2::getVideoModeList()
{
	s32 displayModeCount = SDL_GetNumDisplayModes(0);
	for (s32 i = 0; i < displayModeCount; i++) {
		SDL_DisplayMode mode;
		SDL_GetDesktopDisplayMode(i, &mode);

		if (i == 0) {
			VideoModeList->setDesktop(1, core::dimension2d<u32>(
					mode.w, mode.h));
		}

		VideoModeList->addMode(core::dimension2d<u32>(
			mode.w, mode.h), 1);
	}

	return VideoModeList;
}


//! Minimize window
void CIrrDeviceSDL2::minimizeWindow()
{
	SDL_MinimizeWindow(window);
}


//! Maximize window
void CIrrDeviceSDL2::maximizeWindow()
{
	SDL_MaximizeWindow(window);
}


//! Restore original window size
void CIrrDeviceSDL2::restoreWindow()
{
	SDL_RestoreWindow(window);
}


core::position2di CIrrDeviceSDL2::getWindowPosition()
{
	int x, y;
	SDL_GetWindowPosition(window, &x, &y);
	return {x, y};
}


void CIrrDeviceSDL2::createKeyMap()
{
	// I don't know if this is the best method  to create
	// the lookuptable, but I'll leave it like that until
	// I find a better version.

	KeyMap.reallocate(105);

	// buttons
	KeyMap.push_back(SKeyMap(SDLK_AC_BACK, irr::KEY_CANCEL));

	KeyMap.push_back(SKeyMap(SDLK_BACKSPACE, irr::KEY_BACK));
	KeyMap.push_back(SKeyMap(SDLK_TAB, irr::KEY_TAB));
	KeyMap.push_back(SKeyMap(SDLK_CLEAR, irr::KEY_CLEAR));
	KeyMap.push_back(SKeyMap(SDLK_RETURN, irr::KEY_RETURN));

	// combined modifiers missing

	KeyMap.push_back(SKeyMap(SDLK_PAUSE, irr::KEY_PAUSE));
	KeyMap.push_back(SKeyMap(SDLK_CAPSLOCK, irr::KEY_CAPITAL));

	// asian letter keys missing

	KeyMap.push_back(SKeyMap(SDLK_ESCAPE, irr::KEY_ESCAPE));

	// asian letter keys missing

	KeyMap.push_back(SKeyMap(SDLK_SPACE, irr::KEY_SPACE));
	KeyMap.push_back(SKeyMap(SDLK_PAGEUP, irr::KEY_PRIOR));
	KeyMap.push_back(SKeyMap(SDLK_PAGEDOWN, irr::KEY_NEXT));
	KeyMap.push_back(SKeyMap(SDLK_END, irr::KEY_END));
	KeyMap.push_back(SKeyMap(SDLK_HOME, irr::KEY_HOME));
	KeyMap.push_back(SKeyMap(SDLK_LEFT, irr::KEY_LEFT));
	KeyMap.push_back(SKeyMap(SDLK_UP, irr::KEY_UP));
	KeyMap.push_back(SKeyMap(SDLK_RIGHT, irr::KEY_RIGHT));
	KeyMap.push_back(SKeyMap(SDLK_DOWN, irr::KEY_DOWN));

	// select missing
	KeyMap.push_back(SKeyMap(SDLK_PRINTSCREEN, irr::KEY_PRINT));
	// execute missing
	KeyMap.push_back(SKeyMap(SDLK_PRINTSCREEN, irr::KEY_SNAPSHOT));

	KeyMap.push_back(SKeyMap(SDLK_INSERT, irr::KEY_INSERT));
	KeyMap.push_back(SKeyMap(SDLK_DELETE, irr::KEY_DELETE));
	KeyMap.push_back(SKeyMap(SDLK_HELP, irr::KEY_HELP));

	KeyMap.push_back(SKeyMap(SDLK_0, irr::KEY_KEY_0));
	KeyMap.push_back(SKeyMap(SDLK_1, irr::KEY_KEY_1));
	KeyMap.push_back(SKeyMap(SDLK_2, irr::KEY_KEY_2));
	KeyMap.push_back(SKeyMap(SDLK_3, irr::KEY_KEY_3));
	KeyMap.push_back(SKeyMap(SDLK_4, irr::KEY_KEY_4));
	KeyMap.push_back(SKeyMap(SDLK_5, irr::KEY_KEY_5));
	KeyMap.push_back(SKeyMap(SDLK_6, irr::KEY_KEY_6));
	KeyMap.push_back(SKeyMap(SDLK_7, irr::KEY_KEY_7));
	KeyMap.push_back(SKeyMap(SDLK_8, irr::KEY_KEY_8));
	KeyMap.push_back(SKeyMap(SDLK_9, irr::KEY_KEY_9));

	KeyMap.push_back(SKeyMap(SDLK_a, irr::KEY_KEY_A));
	KeyMap.push_back(SKeyMap(SDLK_b, irr::KEY_KEY_B));
	KeyMap.push_back(SKeyMap(SDLK_c, irr::KEY_KEY_C));
	KeyMap.push_back(SKeyMap(SDLK_d, irr::KEY_KEY_D));
	KeyMap.push_back(SKeyMap(SDLK_e, irr::KEY_KEY_E));
	KeyMap.push_back(SKeyMap(SDLK_f, irr::KEY_KEY_F));
	KeyMap.push_back(SKeyMap(SDLK_g, irr::KEY_KEY_G));
	KeyMap.push_back(SKeyMap(SDLK_h, irr::KEY_KEY_H));
	KeyMap.push_back(SKeyMap(SDLK_i, irr::KEY_KEY_I));
	KeyMap.push_back(SKeyMap(SDLK_j, irr::KEY_KEY_J));
	KeyMap.push_back(SKeyMap(SDLK_k, irr::KEY_KEY_K));
	KeyMap.push_back(SKeyMap(SDLK_l, irr::KEY_KEY_L));
	KeyMap.push_back(SKeyMap(SDLK_m, irr::KEY_KEY_M));
	KeyMap.push_back(SKeyMap(SDLK_n, irr::KEY_KEY_N));
	KeyMap.push_back(SKeyMap(SDLK_o, irr::KEY_KEY_O));
	KeyMap.push_back(SKeyMap(SDLK_p, irr::KEY_KEY_P));
	KeyMap.push_back(SKeyMap(SDLK_q, irr::KEY_KEY_Q));
	KeyMap.push_back(SKeyMap(SDLK_r, irr::KEY_KEY_R));
	KeyMap.push_back(SKeyMap(SDLK_s, irr::KEY_KEY_S));
	KeyMap.push_back(SKeyMap(SDLK_t, irr::KEY_KEY_T));
	KeyMap.push_back(SKeyMap(SDLK_u, irr::KEY_KEY_U));
	KeyMap.push_back(SKeyMap(SDLK_v, irr::KEY_KEY_V));
	KeyMap.push_back(SKeyMap(SDLK_w, irr::KEY_KEY_W));
	KeyMap.push_back(SKeyMap(SDLK_x, irr::KEY_KEY_X));
	KeyMap.push_back(SKeyMap(SDLK_y, irr::KEY_KEY_Y));
	KeyMap.push_back(SKeyMap(SDLK_z, irr::KEY_KEY_Z));

	KeyMap.push_back(SKeyMap(SDLK_LGUI, irr::KEY_LWIN));
	KeyMap.push_back(SKeyMap(SDLK_RGUI, irr::KEY_RWIN));
	// apps missing
	KeyMap.push_back(SKeyMap(SDLK_POWER, irr::KEY_SLEEP)); //??

	KeyMap.push_back(SKeyMap(SDLK_KP_0, irr::KEY_NUMPAD0));
	KeyMap.push_back(SKeyMap(SDLK_KP_1, irr::KEY_NUMPAD1));
	KeyMap.push_back(SKeyMap(SDLK_KP_2, irr::KEY_NUMPAD2));
	KeyMap.push_back(SKeyMap(SDLK_KP_3, irr::KEY_NUMPAD3));
	KeyMap.push_back(SKeyMap(SDLK_KP_4, irr::KEY_NUMPAD4));
	KeyMap.push_back(SKeyMap(SDLK_KP_5, irr::KEY_NUMPAD5));
	KeyMap.push_back(SKeyMap(SDLK_KP_6, irr::KEY_NUMPAD6));
	KeyMap.push_back(SKeyMap(SDLK_KP_7, irr::KEY_NUMPAD7));
	KeyMap.push_back(SKeyMap(SDLK_KP_8, irr::KEY_NUMPAD8));
	KeyMap.push_back(SKeyMap(SDLK_KP_9, irr::KEY_NUMPAD9));
	KeyMap.push_back(SKeyMap(SDLK_KP_MULTIPLY, irr::KEY_MULTIPLY));
	KeyMap.push_back(SKeyMap(SDLK_KP_PLUS, irr::KEY_ADD));
//	KeyMap.push_back(SKeyMap(SDLK_KP_, irr::KEY_SEPARATOR));
	KeyMap.push_back(SKeyMap(SDLK_KP_MINUS, irr::KEY_SUBTRACT));
	KeyMap.push_back(SKeyMap(SDLK_KP_PERIOD, irr::KEY_DECIMAL));
	KeyMap.push_back(SKeyMap(SDLK_KP_DIVIDE, irr::KEY_DIVIDE));

	KeyMap.push_back(SKeyMap(SDLK_F1,  irr::KEY_F1));
	KeyMap.push_back(SKeyMap(SDLK_F2,  irr::KEY_F2));
	KeyMap.push_back(SKeyMap(SDLK_F3,  irr::KEY_F3));
	KeyMap.push_back(SKeyMap(SDLK_F4,  irr::KEY_F4));
	KeyMap.push_back(SKeyMap(SDLK_F5,  irr::KEY_F5));
	KeyMap.push_back(SKeyMap(SDLK_F6,  irr::KEY_F6));
	KeyMap.push_back(SKeyMap(SDLK_F7,  irr::KEY_F7));
	KeyMap.push_back(SKeyMap(SDLK_F8,  irr::KEY_F8));
	KeyMap.push_back(SKeyMap(SDLK_F9,  irr::KEY_F9));
	KeyMap.push_back(SKeyMap(SDLK_F10, irr::KEY_F10));
	KeyMap.push_back(SKeyMap(SDLK_F11, irr::KEY_F11));
	KeyMap.push_back(SKeyMap(SDLK_F12, irr::KEY_F12));
	KeyMap.push_back(SKeyMap(SDLK_F13, irr::KEY_F13));
	KeyMap.push_back(SKeyMap(SDLK_F14, irr::KEY_F14));
	KeyMap.push_back(SKeyMap(SDLK_F15, irr::KEY_F15));
	// no higher F-keys

	KeyMap.push_back(SKeyMap(SDLK_CAPSLOCK, irr::KEY_NUMLOCK));
	KeyMap.push_back(SKeyMap(SDLK_SCROLLLOCK, irr::KEY_SCROLL));
	KeyMap.push_back(SKeyMap(SDLK_LSHIFT, irr::KEY_LSHIFT));
	KeyMap.push_back(SKeyMap(SDLK_RSHIFT, irr::KEY_RSHIFT));
	KeyMap.push_back(SKeyMap(SDLK_LCTRL,  irr::KEY_LCONTROL));
	KeyMap.push_back(SKeyMap(SDLK_RCTRL,  irr::KEY_RCONTROL));
	KeyMap.push_back(SKeyMap(SDLK_LALT,  irr::KEY_LMENU));
	KeyMap.push_back(SKeyMap(SDLK_RALT,  irr::KEY_RMENU));

	KeyMap.push_back(SKeyMap(SDLK_PLUS,   irr::KEY_PLUS));
	KeyMap.push_back(SKeyMap(SDLK_COMMA,  irr::KEY_COMMA));
	KeyMap.push_back(SKeyMap(SDLK_MINUS,  irr::KEY_MINUS));
	KeyMap.push_back(SKeyMap(SDLK_PERIOD, irr::KEY_PERIOD));

	// some special keys missing

	KeyMap.sort();
}

bool CIrrDeviceSDL2::activateJoysticks(core::array<SJoystickInfo> & joystickInfo)
{
	joystickInfo.clear();

	// we can name up to 256 different joysticks
	const int numJoysticks = core::min_(SDL_NumJoysticks(), 256);
	Joysticks.reallocate(numJoysticks);
	joystickInfo.reallocate(numJoysticks);

	int joystick = 0;
	for (; joystick<numJoysticks; ++joystick)
	{
		Joysticks.push_back(SDL_JoystickOpen(joystick));
		SJoystickInfo info;

		info.Joystick = joystick;
		info.Axes = SDL_JoystickNumAxes(Joysticks[joystick]);
		info.Buttons = SDL_JoystickNumButtons(Joysticks[joystick]);
		info.Name = SDL_JoystickName(Joysticks[joystick]);
		info.PovHat = (SDL_JoystickNumHats(Joysticks[joystick]) > 0)
						? SJoystickInfo::POV_HAT_PRESENT : SJoystickInfo::POV_HAT_ABSENT;

		joystickInfo.push_back(info);
	}

	for(joystick = 0; joystick < (int)joystickInfo.size(); ++joystick)
	{
		char logString[256];
		(void)sprintf(logString, "Found joystick %d, %d axes, %d buttons '%s'",
		joystick, joystickInfo[joystick].Axes,
		joystickInfo[joystick].Buttons, joystickInfo[joystick].Name.c_str());
		os::Printer::log(logString, ELL_INFORMATION);
	}

	return true;
}


void CIrrDeviceSDL2::pollJoysticks()
{
	if (0 == ActiveJoysticks.size())
		return;

	for (u32 j= 0; j< ActiveJoysticks.size(); ++j)
	{
		JoystickInfo & info =  ActiveJoysticks[j];

#ifdef __FreeBSD__
		struct joystick js;
		if (read(info.fd, &js, sizeof(js)) == sizeof(js))
		{
			info.persistentData.JoystickEvent.ButtonStates = js.b1 | (js.b2 << 1); /* should be a two-bit field */
			info.persistentData.JoystickEvent.Axis[0] = js.x; /* X axis */
			info.persistentData.JoystickEvent.Axis[1] = js.y; /* Y axis */
		}
#else
		struct js_event event;
		while (sizeof(event) == read(info.fd, &event, sizeof(event)))
		{
			switch(event.type & ~JS_EVENT_INIT)
			{
			case JS_EVENT_BUTTON:
				if (event.value)
						info.persistentData.JoystickEvent.ButtonStates |= (1 << event.number);
				else
						info.persistentData.JoystickEvent.ButtonStates &= ~(1 << event.number);
				break;

			case JS_EVENT_AXIS:
				if (event.number < SEvent::SJoystickEvent::NUMBER_OF_AXES)
					info.persistentData.JoystickEvent.Axis[event.number] = event.value;
				break;

			default:
				break;
			}
		}
#endif

		// Send an irrlicht joystick event once per ::run() even if no new data were received.
		(void)postEventFromUser(info.persistentData);
	}
}


//! Set the current Gamma Value for the Display
bool CIrrDeviceSDL2::setGammaRamp( f32 red, f32 green, f32 blue, f32 brightness, f32 contrast )
{
	// TODO
	//SDL_SetWindowGammaRamp(window, red, green, blue);
	return false;
}


//! Get the current Gamma Value for the Display
bool CIrrDeviceSDL2::getGammaRamp( f32 &red, f32 &green, f32 &blue, f32 &brightness, f32 &contrast )
{
	brightness = 0.f;
	contrast = 0.f;
	// TODO
	//u16 red_[256], green_[256], blue_[256];
	//SDL_GetWindowGammaRamp(window, red_, green_, blue_);
	return false;
}

//! Remove all messages pending in the system message loop
void CIrrDeviceSDL2::clearSystemMessages()
{
}


SDL_Cursor *CIrrDeviceSDL2::TextureToMonochromeCursor(irr::video::ITexture * tex, const core::rect<s32>& sourceRect, const core::position2d<s32> &hotspot)
{
	// write texture into XImage
	video::ECOLOR_FORMAT format = tex->getColorFormat();
	u32 bytesPerPixel = video::IImage::getBitsPerPixelFromFormat(format) / 8;
	u32 bytesLeftGap = sourceRect.UpperLeftCorner.X * bytesPerPixel;
	u32 bytesRightGap = tex->getPitch() - sourceRect.LowerRightCorner.X * bytesPerPixel;
	const u8* data = (const u8*)tex->lock(video::ETLM_READ_ONLY, 0);
	data += sourceRect.UpperLeftCorner.Y*tex->getPitch();
	for ( s32 y = 0; y < sourceRect.getHeight(); ++y )
	{
		data += bytesLeftGap;
		for ( s32 x = 0; x < sourceRect.getWidth(); ++x )
		{
			data += bytesPerPixel;
		}
		data += bytesRightGap;
	}

	SDL_Cursor *cursorResult = SDL_CreateCursor(data, nullptr, sourceRect.getWidth(), sourceRect.getHeight(), 0 ,0);
	tex->unlock();

	return cursorResult;
}

#ifdef _IRR_LINUX_XCURSOR_
Cursor CIrrDeviceSDL2::TextureToARGBCursor(irr::video::ITexture * tex, const core::rect<s32>& sourceRect, const core::position2d<s32> &hotspot)
{
	XcursorImage * image = XcursorImageCreate (sourceRect.getWidth(), sourceRect.getHeight());
	image->xhot = hotspot.X;
	image->yhot = hotspot.Y;

	// write texture into XcursorImage
	video::ECOLOR_FORMAT format = tex->getColorFormat();
	u32 bytesPerPixel = video::IImage::getBitsPerPixelFromFormat(format) / 8;
	u32 bytesLeftGap = sourceRect.UpperLeftCorner.X * bytesPerPixel;
	u32 bytesRightGap = tex->getPitch() - sourceRect.LowerRightCorner.X * bytesPerPixel;
	XcursorPixel* target = image->pixels;
	const u8* data = (const u8*)tex->lock(video::ETLM_READ_ONLY, 0);
	data += sourceRect.UpperLeftCorner.Y*tex->getPitch();
	for ( s32 y = 0; y < sourceRect.getHeight(); ++y )
	{
		data += bytesLeftGap;
		for ( s32 x = 0; x < sourceRect.getWidth(); ++x )
		{
			video::SColor pixelCol;
			pixelCol.setData((const void*)data, format);
			data += bytesPerPixel;

			*target = (XcursorPixel)pixelCol.color;
			++target;
		}
		data += bytesRightGap;
	}
	tex->unlock();

	Cursor cursorResult=XcursorImageLoadCursor(display, image);

	XcursorImageDestroy(image);


	return cursorResult;
}
#endif // #ifdef _IRR_LINUX_XCURSOR_

SDL_Cursor *CIrrDeviceSDL2::TextureToCursor(irr::video::ITexture * tex, const core::rect<s32>& sourceRect, const core::position2d<s32> &hotspot)
{
#ifdef _IRR_LINUX_XCURSOR_
	return TextureToARGBCursor( tex, sourceRect, hotspot );
#else
	return TextureToMonochromeCursor( tex, sourceRect, hotspot );
#endif
}


CIrrDeviceSDL2::CCursorControl::CCursorControl(CIrrDeviceSDL2* dev, bool null)
	: Device(dev)
#ifdef _IRR_COMPILE_WITH_X11_
	, PlatformBehavior(gui::ECPB_NONE), lastQuery(0)
#endif
	, UseReferenceRect(false)
	, ActiveIcon(gui::ECI_NORMAL), ActiveIconStartTime(0)
{
	SDL_ShowCursor(SDL_ENABLE);
	initCursors();
}

CIrrDeviceSDL2::CCursorControl::~CCursorControl()
{
	// Do not clearCursors here as the display is already closed
	// TODO (cutealien): droping cursorcontrol earlier might work, not sure about reason why that's done in stub currently.
}

void CIrrDeviceSDL2::CCursorControl::clearCursors()
{
	for ( u32 i=0; i < Cursors.size(); ++i )
	{
		for ( u32 f=0; f < Cursors[i].Frames.size(); ++f )
		{
			SDL_FreeCursor(Cursors[i].Frames[f].IconHW);
		}
	}
}

void CIrrDeviceSDL2::CCursorControl::initCursors()
{
	/*Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_top_left_arrow)) ); //  (or XC_arrow?)
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_crosshair)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_hand2)) ); // (or XC_hand1? )
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_question_arrow)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_xterm)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_X_cursor)) );	//  (or XC_pirate?)
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_watch)) );	// (or XC_clock?)
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_fleur)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_top_right_corner)) );	// NESW not available in X11
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_top_left_corner)) );	// NWSE not available in X11
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_sb_v_double_arrow)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_sb_h_double_arrow)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_sb_up_arrow)) );	// (or XC_center_ptr?)
	*/
}

void CIrrDeviceSDL2::CCursorControl::update()
{
	if ( (u32)ActiveIcon < Cursors.size() && !Cursors[ActiveIcon].Frames.empty() && Cursors[ActiveIcon].FrameTime )
	{
		// update animated cursors. This could also be done by X11 in case someone wants to figure that out (this way was just easier to implement)
		u32 now = Device->getTimer()->getRealTime();
		u32 frame = ((now - ActiveIconStartTime) / Cursors[ActiveIcon].FrameTime) % Cursors[ActiveIcon].Frames.size();
		SDL_SetCursor(Cursors[ActiveIcon].Frames[frame].IconHW);
	}
}

//! Sets the active cursor icon
void CIrrDeviceSDL2::CCursorControl::setActiveIcon(gui::ECURSOR_ICON iconId)
{
	if ( iconId >= (s32)Cursors.size() )
		return;

	if ( Cursors[iconId].Frames.size() )
		SDL_SetCursor(Cursors[iconId].Frames[0].IconHW);

	ActiveIconStartTime = Device->getTimer()->getRealTime();
	ActiveIcon = iconId;
}


//! Add a custom sprite as cursor icon.
gui::ECURSOR_ICON CIrrDeviceSDL2::CCursorControl::addIcon(const gui::SCursorSprite& icon)
{
	if ( icon.SpriteId >= 0 )
	{
		CursorX11 cX11;
		cX11.FrameTime = icon.SpriteBank->getSprites()[icon.SpriteId].frameTime;
		for ( u32 i=0; i < icon.SpriteBank->getSprites()[icon.SpriteId].Frames.size(); ++i )
		{
			irr::u32 texId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].textureNumber;
			irr::u32 rectId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].rectNumber;
			irr::core::rect<s32> rectIcon = icon.SpriteBank->getPositions()[rectId];
			SDL_Cursor *cursor = Device->TextureToCursor(icon.SpriteBank->getTexture(texId), rectIcon, icon.HotSpot);
			cX11.Frames.push_back( CursorFrameX11(cursor) );
		}

		Cursors.push_back( cX11 );

		return (gui::ECURSOR_ICON)(Cursors.size() - 1);
	}
	return gui::ECI_NORMAL;
}

//! replace the given cursor icon.
void CIrrDeviceSDL2::CCursorControl::changeIcon(gui::ECURSOR_ICON iconId, const gui::SCursorSprite& icon)
{
	if ( iconId >= (s32)Cursors.size() )
		return;

	for ( u32 i=0; i < Cursors[iconId].Frames.size(); ++i )
		SDL_FreeCursor(Cursors[iconId].Frames[i].IconHW);

	if ( icon.SpriteId >= 0 )
	{
		CursorX11 cX11;
		cX11.FrameTime = icon.SpriteBank->getSprites()[icon.SpriteId].frameTime;
		for ( u32 i=0; i < icon.SpriteBank->getSprites()[icon.SpriteId].Frames.size(); ++i )
		{
			irr::u32 texId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].textureNumber;
			irr::u32 rectId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].rectNumber;
			irr::core::rect<s32> rectIcon = icon.SpriteBank->getPositions()[rectId];
			SDL_Cursor *cursor = Device->TextureToCursor(icon.SpriteBank->getTexture(texId), rectIcon, icon.HotSpot);
			cX11.Frames.push_back( CursorFrameX11(cursor) );
		}

		Cursors[iconId] = cX11;
	}
}

irr::core::dimension2di CIrrDeviceSDL2::CCursorControl::getSupportedIconSize() const
{
	// this returns the closest match that is smaller or same size, so we just pass a value which should be large enough for cursors
	unsigned int width=0, height=0;
#ifdef _IRR_COMPILE_WITH_X11_
	//XQueryBestCursor(Device->display, Device->window, 64, 64, &width, &height);
#endif
	return core::dimension2di(width, height);
}

} // end namespace
