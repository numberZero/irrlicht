// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

static const char* const copyright = "Irrlicht Engine (c) 2002-2017 Nikolaus Gebhardt";	// put string in binary

#include "irrlicht.h"
#include "CIrrDeviceSDL2.h"

namespace irr
{
	//! stub for calling createDeviceEx
	IRRLICHT_API IrrlichtDevice* IRRCALLCONV createDevice(video::E_DRIVER_TYPE driverType,
			const core::dimension2d<u32>& windowSize,
			u32 bits, bool fullscreen,
			bool stencilbuffer, bool vsync, IEventReceiver* res)
	{
		(void)copyright;	// prevent unused variable warning

		SIrrlichtCreationParameters p;
		p.DriverType = driverType;
		p.WindowSize = windowSize;
		p.Bits = (u8)bits;
		p.Fullscreen = fullscreen;
		p.Stencilbuffer = stencilbuffer;
		p.Vsync = vsync;
		p.EventReceiver = res;

		return createDeviceEx(p);
	}

	extern "C" IRRLICHT_API IrrlichtDevice* IRRCALLCONV createDeviceEx(const SIrrlichtCreationParameters& params)
	{
		CIrrDeviceSDL2 *dev = new CIrrDeviceSDL2(params);
		if (!dev->getVideoDriver()) {
			dev->closeDevice();
			dev->run();
			dev->drop();
			dev = nullptr;
		}
		return dev;
	}

namespace core
{
	const matrix4 IdentityMatrix(matrix4::EM4CONST_IDENTITY);
	irr::core::stringc LOCALE_DECIMAL_POINTS(".");
}

namespace video
{
	SMaterial IdentityMaterial;
	u32 MATERIAL_MAX_TEXTURES_USED = MATERIAL_MAX_TEXTURES;
}

} // end namespace irr
