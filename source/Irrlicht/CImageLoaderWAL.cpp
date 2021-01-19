// Copyright (C) 2004 Murphy McCauley
// Copyright (C) 2007-2012 Christian Stehno
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CImageLoaderWAL.h"

#include "CColorConverter.h"
#include "CImage.h"
#include "os.h"
#include "dimension2d.h"
#include "IVideoDriver.h"
#include "IFileSystem.h"
#include "IReadFile.h"
#include "irrString.h"

namespace irr
{
namespace video
{

// Palette quake2 demo pics/colormap.pcx, last is transparent
static const u32 colormap_pcx[256] = {
	0xFF000000,0xFF0F0F0F,0xFF1F1F1F,0xFF2F2F2F,0xFF3F3F3F,0xFF4B4B4B,0xFF5B5B5B,0xFF6B6B6B,
	0xFF7B7B7B,0xFF8B8B8B,0xFF9B9B9B,0xFFABABAB,0xFFBBBBBB,0xFFCBCBCB,0xFFDBDBDB,0xFFEBEBEB,
	0xFF634B23,0xFF5B431F,0xFF533F1F,0xFF4F3B1B,0xFF47371B,0xFF3F2F17,0xFF3B2B17,0xFF332713,
	0xFF2F2313,0xFF2B1F13,0xFF271B0F,0xFF23170F,0xFF1B130B,0xFF170F0B,0xFF130F07,0xFF0F0B07,
	0xFF5F5F6F,0xFF5B5B67,0xFF5B535F,0xFF574F5B,0xFF534B53,0xFF4F474B,0xFF473F43,0xFF3F3B3B,
	0xFF3B3737,0xFF332F2F,0xFF2F2B2B,0xFF272727,0xFF232323,0xFF1B1B1B,0xFF171717,0xFF131313,
	0xFF8F7753,0xFF7B6343,0xFF735B3B,0xFF674F2F,0xFFCF974B,0xFFA77B3B,0xFF8B672F,0xFF6F5327,
	0xFFEB9F27,0xFFCB8B23,0xFFAF771F,0xFF93631B,0xFF774F17,0xFF5B3B0F,0xFF3F270B,0xFF231707,
	0xFFA73B2B,0xFF9F2F23,0xFF972B1B,0xFF8B2713,0xFF7F1F0F,0xFF73170B,0xFF671707,0xFF571300,
	0xFF4B0F00,0xFF430F00,0xFF3B0F00,0xFF330B00,0xFF2B0B00,0xFF230B00,0xFF1B0700,0xFF130700,
	0xFF7B5F4B,0xFF735743,0xFF6B533F,0xFF674F3B,0xFF5F4737,0xFF574333,0xFF533F2F,0xFF4B372B,
	0xFF433327,0xFF3F2F23,0xFF37271B,0xFF2F2317,0xFF271B13,0xFF1F170F,0xFF170F0B,0xFF0F0B07,
	0xFF6F3B17,0xFF5F3717,0xFF532F17,0xFF432B17,0xFF372313,0xFF271B0F,0xFF1B130B,0xFF0F0B07,
	0xFFB35B4F,0xFFBF7B6F,0xFFCB9B93,0xFFD7BBB7,0xFFCBD7DF,0xFFB3C7D3,0xFF9FB7C3,0xFF87A7B7,
	0xFF7397A7,0xFF5B879B,0xFF47778B,0xFF2F677F,0xFF17536F,0xFF134B67,0xFF0F435B,0xFF0B3F53,
	0xFF07374B,0xFF072F3F,0xFF072733,0xFF001F2B,0xFF00171F,0xFF000F13,0xFF00070B,0xFF000000,
	0xFF8B5757,0xFF834F4F,0xFF7B4747,0xFF734343,0xFF6B3B3B,0xFF633333,0xFF5B2F2F,0xFF572B2B,
	0xFF4B2323,0xFF3F1F1F,0xFF331B1B,0xFF2B1313,0xFF1F0F0F,0xFF130B0B,0xFF0B0707,0xFF000000,
	0xFF979F7B,0xFF8F9773,0xFF878B6B,0xFF7F8363,0xFF777B5F,0xFF737357,0xFF6B6B4F,0xFF636347,
	0xFF5B5B43,0xFF4F4F3B,0xFF434333,0xFF37372B,0xFF2F2F23,0xFF23231B,0xFF171713,0xFF0F0F0B,
	0xFF9F4B3F,0xFF934337,0xFF8B3B2F,0xFF7F3727,0xFF772F23,0xFF6B2B1B,0xFF632317,0xFF571F13,
	0xFF4F1B0F,0xFF43170B,0xFF37130B,0xFF2B0F07,0xFF1F0B07,0xFF170700,0xFF0B0000,0xFF000000,
	0xFF777BCF,0xFF6F73C3,0xFF676BB7,0xFF6363A7,0xFF5B5B9B,0xFF53578F,0xFF4B4F7F,0xFF474773,
	0xFF3F3F67,0xFF373757,0xFF2F2F4B,0xFF27273F,0xFF231F2F,0xFF1B1723,0xFF130F17,0xFF0B0707,
	0xFF9BAB7B,0xFF8F9F6F,0xFF879763,0xFF7B8B57,0xFF73834B,0xFF677743,0xFF5F6F3B,0xFF576733,
	0xFF4B5B27,0xFF3F4F1B,0xFF374313,0xFF2F3B0B,0xFF232F07,0xFF1B2300,0xFF131700,0xFF0B0F00,
	0xFF00FF00,0xFF23E70F,0xFF3FD31B,0xFF53BB27,0xFF5FA72F,0xFF5F8F33,0xFF5F7B33,0xFFFFFFFF,
	0xFFFFFFD3,0xFFFFFFA7,0xFFFFFF7F,0xFFFFFF53,0xFFFFFF27,0xFFFFEB1F,0xFFFFD717,0xFFFFBF0F,
	0xFFFFAB07,0xFFFF9300,0xFFEF7F00,0xFFE36B00,0xFFD35700,0xFFC74700,0xFFB73B00,0xFFAB2B00,
	0xFF9B1F00,0xFF8F1700,0xFF7F0F00,0xFF730700,0xFF5F0000,0xFF470000,0xFF2F0000,0xFF1B0000,
	0xFFEF0000,0xFF3737FF,0xFFFF0000,0xFF0000FF,0xFF2B2B23,0xFF1B1B17,0xFF13130F,0xFFEB977F,
	0xFFC37353,0xFF9F5733,0xFF7B3F1B,0xFFEBD3C7,0xFFC7AB9B,0xFFA78B77,0xFF876B57,0x009F5B53
};

/*!
	Halflife
*/
bool CImageLoaderWAL2::isALoadableFileExtension(const io::path& filename) const
{
	// embedded in Wad(WAD3 format). originally it has no extension
	return core::hasFileExtension ( filename, "wal2" );
}


bool CImageLoaderWAL2::isALoadableFileFormat(irr::io::IReadFile* file) const
{
	return false;
}

/*
	Halflite Texture WAD
*/
IImage* CImageLoaderWAL2::loadImage(irr::io::IReadFile* file) const
{
	miptex_halflife header;

	file->seek(0);
	file->read(&header, sizeof(header));

#ifdef __BIG_ENDIAN__
	header.width = os::Byteswap::byteswap(header.width);
	header.height = os::Byteswap::byteswap(header.height);
#endif

	// palette
	//u32 paletteofs = header.mipmap[0] + ((rawtexsize * 85) >> 6) + 2;
	u32 *pal = new u32 [ 192 + 256 ];
	u8 *s = (u8*) pal;

	file->seek ( file->getSize() - 768 - 2 );
	file->read ( s, 768 );
	u32 i;

	for ( i = 0; i < 256; ++i, s+= 3 )
	{
		pal [ 192 + i ] = 0xFF000000 | s[0] << 16 | s[1] << 8 | s[2];
	}

	ECOLOR_FORMAT format = ECF_R8G8B8;

	// transparency in filename;-) funny. rgb:0x0000FF is colorkey
	if ( file->getFileName().findFirst ( '{' ) >= 0 )
	{
		format = ECF_A8R8G8B8;
		pal [ 192 + 255 ] &= 0x00FFFFFF;
	}

	u32 rawtexsize = header.width * header.height;
	u8 *rawtex = new u8 [ rawtexsize ];

	file->seek ( header.mipmap[0] );
	file->read(rawtex, rawtexsize);

	IImage* image = new CImage(format, core::dimension2d<u32>(header.width, header.height));

	switch ( format )
	{
	case ECF_R8G8B8:
		CColorConverter::convert8BitTo24Bit(rawtex, (u8*)image->getData(), header.width, header.height, (u8*) pal + 768, 0, false);
		break;
	case ECF_A8R8G8B8:
		CColorConverter::convert8BitTo32Bit(rawtex, (u8*)image->getData(), header.width, header.height, (u8*) pal + 768, 0, false);
		break;
	default:
		// Assuming there are no other color formats (I found no information about this format)
		break;
	}

	delete [] rawtex;
	delete [] pal;

	return image;
}

bool CImageLoaderWAL::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "wal" );
}


bool CImageLoaderWAL::isALoadableFileFormat(irr::io::IReadFile* file) const
{
	return false;
}


/*!
	quake2
*/
IImage* CImageLoaderWAL::loadImage(irr::io::IReadFile* file) const
{
	miptex_quake2 header;

	file->seek(0);
	file->read(&header, sizeof(header));

#ifdef __BIG_ENDIAN__
	header.width = os::Byteswap::byteswap(header.width);
	header.height = os::Byteswap::byteswap(header.height);
#endif

	u32 rawtexsize = header.width * header.height;

	u8 *rawtex = new u8 [ rawtexsize ];

	file->seek ( header.mipmap[0] );
	file->read(rawtex, rawtexsize);

	IImage* image = new CImage(ECF_A8R8G8B8, core::dimension2d<u32>(header.width, header.height));

	CColorConverter::convert8BitTo32Bit(rawtex, (u8*)image->getData(), header.width, header.height, (u8*) colormap_pcx, 0, false);

	delete [] rawtex;

	return image;
}

IImageLoader* createImageLoaderWAL()
{
	return new irr::video::CImageLoaderWAL();
}

IImageLoader* createImageLoaderHalfLife()
{
	return new irr::video::CImageLoaderWAL2();
}

} // end namespace video
} // end namespace irr
