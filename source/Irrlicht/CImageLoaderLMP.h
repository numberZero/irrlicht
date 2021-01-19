// Copyright (C) 2004 Murphy McCauley
// Copyright (C) 2007-2012 Christian Stehno
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
/*
 Thanks to:
 Max McGuire for his Flipcode article about WAL textures
 Nikolaus Gebhardt for the Irrlicht 3D engine
*/

#ifndef __C_IMAGE_LOADER_WAL_H_INCLUDED__
#define __C_IMAGE_LOADER_WAL_H_INCLUDED__

#include "IImageLoader.h"

namespace irr
{
namespace video
{

// byte-align structures
#include "irrpack.h"

	struct SLMPHeader {
		u32	width;	// width
		u32	height;	// height
		// variably sized
	} PACK_STRUCT;

// Default alignment
#include "irrunpack.h"

//! An Irrlicht image loader for Quake1,2 engine lmp textures/palette
class CImageLoaderLMP : public irr::video::IImageLoader
{
public:
	virtual bool isALoadableFileExtension(const io::path& filename) const _IRR_OVERRIDE_;
	virtual bool isALoadableFileFormat(irr::io::IReadFile* file) const _IRR_OVERRIDE_;
	virtual irr::video::IImage* loadImage(irr::io::IReadFile* file) const _IRR_OVERRIDE_;
};

}
}

#endif

