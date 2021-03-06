// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "CIrrDeviceStub.h"
#include "IrrlichtDevice.h"
#include "IImagePresenter.h"
#include "ICursorControl.h"
#include "os.h"

#include "SDL.h"

#ifdef _IRR_COMPILE_WITH_OPENGL_
#include <GL/gl.h>
#define GLX_GLXEXT_LEGACY 1
#include <GL/glx.h>
#ifdef _IRR_OPENGL_USE_EXTPOINTER_
#include "glxext.h"
#endif
#endif

#ifdef _IRR_COMPILE_WITH_X11_
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#else
#define KeySym s32
#endif

namespace irr
{
	namespace video
	{
		class COpenGLDriver;
	}

	class CIrrDeviceSDL2 : public CIrrDeviceStub, public video::IImagePresenter
	{
	public:

		//! constructor
		CIrrDeviceSDL2(const SIrrlichtCreationParameters& param);

		//! destructor
		~CIrrDeviceSDL2() override;

		//! runs the device. Returns false if device wants to be deleted
		bool run() override;

		//! Cause the device to temporarily pause execution and let other processes to run
		// This should bring down processor usage without major performance loss for Irrlicht
		void yield() override;

		//! Pause execution and let other processes to run for a specified amount of time.
		void sleep(u32 timeMs, bool pauseTimer) override;

		//! sets the caption of the window
		void setWindowCaption(const wchar_t* text) override;

		//! returns if window is active. if not, nothing need to be drawn
		bool isWindowActive() const override;

		//! returns if window has focus.
		bool isWindowFocused() const override;

		//! returns if window is minimized.
		bool isWindowMinimized() const override;

		//! returns color format of the window.
		video::ECOLOR_FORMAT getColorFormat() const override;

		//! presents a surface in the client area
		bool present(video::IImage* surface, void* windowId=0, core::rect<s32>* src=0 ) override;

		//! notifies the device that it should close itself
		void closeDevice() override;

		//! \return Returns a pointer to a list with all video modes
		//! supported by the gfx adapter.
		video::IVideoModeList* getVideoModeList() override;

		//! Sets if the window should be resizable in windowed mode.
		void setResizable(bool resize=false) override;

		//! Minimizes the window.
		void minimizeWindow() override;

		//! Maximizes the window.
		void maximizeWindow() override;

		//! Restores the window size.
		void restoreWindow() override;

		//! Get the position of the frame on-screen
		core::position2di getWindowPosition() override;

		//! Activate any joysticks, and generate events for them.
		bool activateJoysticks(core::array<SJoystickInfo> & joystickInfo) override;

		//! Set the current Gamma Value for the Display
		bool setGammaRamp( f32 red, f32 green, f32 blue, f32 brightness, f32 contrast ) override;

		//! Get the current Gamma Value for the Display
		bool getGammaRamp( f32 &red, f32 &green, f32 &blue, f32 &brightness, f32 &contrast ) override;

		//! Remove all messages pending in the system message loop
		void clearSystemMessages() override;

		//! Get the device type
		E_DEVICE_TYPE getType() const override
		{
				return EIDT_X11;
		}

		// convert an Irrlicht texture to a X11 cursor
		SDL_Cursor *TextureToCursor(irr::video::ITexture * tex, const core::rect<s32>& sourceRect, const core::position2d<s32> &hotspot);
		SDL_Cursor *TextureToMonochromeCursor(irr::video::ITexture * tex, const core::rect<s32>& sourceRect, const core::position2d<s32> &hotspot);
#ifdef _IRR_LINUX_XCURSOR_
		Cursor TextureToARGBCursor(irr::video::ITexture * tex, const core::rect<s32>& sourceRect, const core::position2d<s32> &hotspot);
#endif

	private:

		//! create the driver
		void createDriver();

		bool createWindow();

		void createKeyMap();

		void pollJoysticks();

		bool switchToFullscreen(bool reset=false);

		//! Implementation of the linux cursor control
		class CCursorControl : public gui::ICursorControl
		{
		public:

			CCursorControl(CIrrDeviceSDL2* dev, bool null);

			~CCursorControl();

			//! Changes the visible state of the mouse cursor.
			void setVisible(bool visible) override
			{
				if (visible)
					SDL_ShowCursor(SDL_ENABLE);
				else
					SDL_ShowCursor(SDL_DISABLE);
			}

			//! Returns if the cursor is currently visible.
			bool isVisible() const override
			{
				return SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE;
			}

			//! Sets the new position of the cursor.
			void setPosition(const core::position2d<f32> &pos) override
			{
				setPosition(pos.X, pos.Y);
			}

			//! Sets the new position of the cursor.
			void setPosition(f32 x, f32 y) override
			{
				setPosition((s32)(x*Device->Width), (s32)(y*Device->Height));
			}

			//! Sets the new position of the cursor.
			void setPosition(const core::position2d<s32> &pos) override
			{
				setPosition(pos.X, pos.Y);
			}

			//! Sets the new position of the cursor.
			void setPosition(s32 x, s32 y) override
			{
				if (UseReferenceRect)
					SDL_WarpMouseGlobal(ReferenceRect.UpperLeftCorner.X + x, ReferenceRect.UpperLeftCorner.Y + y);
				else
					SDL_WarpMouseInWindow(Device->window, x, y);
				CursorPos.X = x;
				CursorPos.Y = y;
			}

			//! Returns the current position of the mouse cursor.
			const core::position2d<s32>& getPosition(bool updateCursor=true) override
			{
				if (updateCursor)
					updateCursorPos();
				return CursorPos;
			}

			//! Returns the current position of the mouse cursor.
			core::position2d<f32> getRelativePosition(bool updateCursor=true) override
			{
				if (updateCursor)
					updateCursorPos();

				if (!UseReferenceRect)
				{
					return core::position2d<f32>(CursorPos.X / (f32)Device->Width,
						CursorPos.Y / (f32)Device->Height);
				}

				return core::position2d<f32>(CursorPos.X / (f32)ReferenceRect.getWidth(),
						CursorPos.Y / (f32)ReferenceRect.getHeight());
			}

			void setReferenceRect(core::rect<s32>* rect=0) override
			{
				if (rect)
				{
					ReferenceRect = *rect;
					UseReferenceRect = true;

					// prevent division through zero and uneven sizes

					if (!ReferenceRect.getHeight() || ReferenceRect.getHeight()%2)
						ReferenceRect.LowerRightCorner.Y += 1;

					if (!ReferenceRect.getWidth() || ReferenceRect.getWidth()%2)
						ReferenceRect.LowerRightCorner.X += 1;
				}
				else
					UseReferenceRect = false;
			}

			//! Sets the active cursor icon
			void setActiveIcon(gui::ECURSOR_ICON iconId) override;

			//! Gets the currently active icon
			gui::ECURSOR_ICON getActiveIcon() const override
			{
				return ActiveIcon;
			}

			//! Add a custom sprite as cursor icon.
			gui::ECURSOR_ICON addIcon(const gui::SCursorSprite& icon) override;

			//! replace the given cursor icon.
			void changeIcon(gui::ECURSOR_ICON iconId, const gui::SCursorSprite& icon) override;

			//! Return a system-specific size which is supported for cursors. Larger icons will fail, smaller icons might work.
			core::dimension2di getSupportedIconSize() const override;

#ifdef _IRR_COMPILE_WITH_X11_
			void setPlatformBehavior(gui::ECURSOR_PLATFORM_BEHAVIOR behavior) override {PlatformBehavior = behavior; }
			gui::ECURSOR_PLATFORM_BEHAVIOR getPlatformBehavior() const override { return PlatformBehavior; }
#endif

			void update();
			void clearCursors();
		private:

			void updateCursorPos()
			{
#ifdef _IRR_COMPILE_WITH_X11_
				if (PlatformBehavior & gui::ECPB_X11_CACHE_UPDATES && !os::Timer::isStopped() )
				{
					u32 now = os::Timer::getTime();
					if (now <= lastQuery)
						return;
					lastQuery = now;
				}
#endif
				SDL_GetMouseState(&CursorPos.X, &CursorPos.Y);

				if (CursorPos.X < 0)
					CursorPos.X = 0;
				if (CursorPos.X > (s32) Device->Width)
					CursorPos.X = Device->Width;
				if (CursorPos.Y < 0)
					CursorPos.Y = 0;
				if (CursorPos.Y > (s32) Device->Height)
					CursorPos.Y = Device->Height;
			}

			CIrrDeviceSDL2* Device;
			core::position2d<s32> CursorPos;
			core::rect<s32> ReferenceRect;

			gui::ECURSOR_PLATFORM_BEHAVIOR PlatformBehavior;
			u32 lastQuery;

			struct CursorFrameX11
			{
				CursorFrameX11() : IconHW(0) {}
				CursorFrameX11(SDL_Cursor *icon) : IconHW(icon) {}

				SDL_Cursor *IconHW;	// hardware cursor
			};

			struct CursorX11
			{
				CursorX11() {}
				explicit CursorX11(SDL_Cursor *iconHw, u32 frameTime=0) : FrameTime(frameTime)
				{
					Frames.push_back( CursorFrameX11(iconHw) );
				}
				core::array<CursorFrameX11> Frames;
				u32 FrameTime;
			};

			core::array<CursorX11> Cursors;

			void initCursors();
			bool UseReferenceRect;
			gui::ECURSOR_ICON ActiveIcon;
			u32 ActiveIconStartTime;
		};

		friend class CCursorControl;

		friend class video::COpenGLDriver;

		SDL_Window *window;
		SDL_GLContext Context;
		
		u32 Width, Height;
		bool WindowHasFocus;
		bool WindowMinimized;

		core::array<SDL_Joystick*> Joysticks;

		s32 MouseX, MouseY;
		u32 MouseButtonStates = 0;

		struct SKeyMap
		{
			SKeyMap() {}
			SKeyMap(s32 x11, s32 win32)
				: SDLKey(x11), Win32Key(win32)
			{
			}

			s32 SDLKey;
			s32 Win32Key;

			bool operator<(const SKeyMap& o) const
			{
				return SDLKey<o.SDLKey;
			}
		};

		core::array<SKeyMap> KeyMap;
		u16 KeyMode = 0;

		struct JoystickInfo
		{
			int	fd;
			int	axes;
			int	buttons;

			SEvent persistentData;

			JoystickInfo() : fd(-1), axes(0), buttons(0) { }
		};
		core::array<JoystickInfo> ActiveJoysticks;

		struct TouchInfo {
			SDL_TouchID device;
			SDL_FingerID touch;
			size_t id;
		};
		core::array<TouchInfo> CurrentTouches;
		size_t LastTouchId = 0;

		size_t findTouch(SDL_TouchID device, SDL_FingerID touch);
		size_t addTouch(SDL_TouchID device, SDL_FingerID touch);
		void removeTouch(size_t id);
	};


} // end namespace irr
