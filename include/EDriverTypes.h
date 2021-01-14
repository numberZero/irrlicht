// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __E_DRIVER_TYPES_H_INCLUDED__
#define __E_DRIVER_TYPES_H_INCLUDED__

#include "irrTypes.h"

namespace irr
{
namespace video
{

	//! An enum for all types of drivers the Irrlicht Engine supports.
	enum E_DRIVER_TYPE
	{
		//! Null driver, useful for applications to run the engine without visualization.
		/** The null device is able to load textures, but does not
		render and display any graphics. */
		EDT_NULL = 0,

		//! OpenGL device, available on most platforms.
		/** Performs hardware accelerated rendering of 3D and 2D
		primitives. */
		EDT_OPENGL = 5,

        //! OpenGL-ES 1.x driver, for embedded and mobile systems
		EDT_OGLES1 = 6,

		//! OpenGL-ES 2.x driver, for embedded and mobile systems
		/** Supports shaders etc. */
		EDT_OGLES2 = 7,

		//! WebGL1 friendly subset of OpenGL-ES 2.x driver for Emscripten
		EDT_WEBGL1 = 8,

		//! No driver, just for counting the elements
		EDT_COUNT
	};

	const c8* const DRIVER_TYPE_NAMES[] =
	{
		"NullDriver",
		"Software Renderer",
		"Burning's Video",
		"Direct3D 8.1",
		"Direct3D 9.0c",
		"OpenGL 1.x/2.x/3.x",
		"OpenGL ES1",
		"OpenGL ES2",
		"WebGL 1",
		0
	};

	const c8* const DRIVER_TYPE_NAMES_SHORT[] =
	{
		"null",
		"software",
		"burning",
		"d3d8",
		"d3d9",
		"opengl",
		"ogles1",
		"ogles2",
		"webgl1",
		0
	};

} // end namespace video
} // end namespace irr


#endif
