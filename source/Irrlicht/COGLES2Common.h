// Copyright (C) 2015 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_OGLES2_COMMON_H_INCLUDED__
#define __C_OGLES2_COMMON_H_INCLUDED__



#ifndef _IRR_OGLES1_USE_EXTPOINTER_
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_opengles2.h>

#ifndef GL_BGRA
#define GL_BGRA 0x80E1;
#endif

// FBO definitions.

#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 1
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 2
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS 3

// to check if this header is in the current compile unit (different GL implementation used different "GLCommon" headers in Irrlicht
#define IRR_COMPILE_GLES2_COMMON

namespace irr
{
namespace video
{

	// Forward declarations.

	class COpenGLCoreFeature;

	template <class TOpenGLDriver>
	class COpenGLCoreTexture;

	template <class TOpenGLDriver, class TOpenGLTexture>
	class COpenGLCoreRenderTarget;

	template <class TOpenGLDriver, class TOpenGLTexture>
	class COpenGLCoreCacheHandler;

	class COGLES2Driver;
	typedef COpenGLCoreTexture<COGLES2Driver> COGLES2Texture;
	typedef COpenGLCoreRenderTarget<COGLES2Driver, COGLES2Texture> COGLES2RenderTarget;
	typedef COpenGLCoreCacheHandler<COGLES2Driver, COGLES2Texture> COGLES2CacheHandler;

}
}

#endif
