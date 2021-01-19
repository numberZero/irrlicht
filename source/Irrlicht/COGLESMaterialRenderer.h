// Copyright (C) 2002-2008 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_OGLES1_MATERIAL_RENDERER_H_INCLUDED__
#define __C_OGLES1_MATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_OGLES1_

#include "COGLESDriver.h"
#include "IMaterialRenderer.h"

namespace irr
{
namespace video
{

//! Base class for all internal OGLES1 material renderers
class COGLES1MaterialRenderer : public IMaterialRenderer
{
public:

	//! Constructor
	COGLES1MaterialRenderer(video::COGLES1Driver* driver) : Driver(driver)
	{
	}

protected:

	video::COGLES1Driver* Driver;
};


//! Solid material renderer
class COGLES1MaterialRenderer_SOLID : public COGLES1MaterialRenderer
{
public:

	COGLES1MaterialRenderer_SOLID(video::COGLES1Driver* d)
		: COGLES1MaterialRenderer(d) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
		bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		if (resetAllRenderstates || (material.MaterialType != lastMaterial.MaterialType))
		{
			// thanks to Murphy, the following line removed some
			// bugs with several OGLES1 implementations.
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
	}
};


//! Transparent add color material renderer
class COGLES1MaterialRenderer_TRANSPARENT_ADD_COLOR : public COGLES1MaterialRenderer
{
public:

	COGLES1MaterialRenderer_TRANSPARENT_ADD_COLOR(video::COGLES1Driver* d)
		: COGLES1MaterialRenderer(d) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
		bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		Driver->getCacheHandler()->setBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
		Driver->getCacheHandler()->setBlend(true);

		if ((material.MaterialType != lastMaterial.MaterialType) || resetAllRenderstates)
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	virtual void OnUnsetMaterial()
	{
		Driver->getCacheHandler()->setBlend(false);
	}

	//! Returns if the material is transparent.
	virtual bool isTransparent() const
	{
		return true;
	}
};


//! Transparent vertex alpha material renderer
class COGLES1MaterialRenderer_TRANSPARENT_VERTEX_ALPHA : public COGLES1MaterialRenderer
{
public:

	COGLES1MaterialRenderer_TRANSPARENT_VERTEX_ALPHA(video::COGLES1Driver* d)
		: COGLES1MaterialRenderer(d) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
		bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		Driver->getCacheHandler()->setBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		Driver->getCacheHandler()->setBlend(true);

		if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
		{
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PRIMARY_COLOR );

			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR );
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);
		}
	}

	virtual void OnUnsetMaterial()
	{
		// default values
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE );
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PREVIOUS );
		glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE );
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);

		Driver->getCacheHandler()->setBlend(false);
	}

	//! Returns if the material is transparent.
	virtual bool isTransparent() const
	{
		return true;
	}
};


//! Transparent alpha channel material renderer
class COGLES1MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL : public COGLES1MaterialRenderer
{
public:

	COGLES1MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL(video::COGLES1Driver* d)
		: COGLES1MaterialRenderer(d) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
		bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		Driver->getCacheHandler()->setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		Driver->getCacheHandler()->setBlend(true);

		if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates
			|| material.MaterialTypeParam != lastMaterial.MaterialTypeParam )
		{
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);

			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);

			glEnable(GL_ALPHA_TEST);

			glAlphaFunc(GL_GREATER, material.MaterialTypeParam);
		}
	}

	virtual void OnUnsetMaterial()
	{
		glDisable(GL_ALPHA_TEST);
		Driver->getCacheHandler()->setBlend(false);
	}

	//! Returns if the material is transparent.
	virtual bool isTransparent() const
	{
		return true;
	}
};


//! Transparent alpha channel material renderer
class COGLES1MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL_REF : public COGLES1MaterialRenderer
{
public:

	COGLES1MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL_REF(video::COGLES1Driver* d)
		: COGLES1MaterialRenderer(d) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
		bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
		{
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.5f);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
	}

	virtual void OnUnsetMaterial()
	{
		glDisable(GL_ALPHA_TEST);
	}

	//! Returns if the material is transparent.
	virtual bool isTransparent() const
	{
		return false;  // this material is not really transparent because it does no blending.
	}
};

} // end namespace video
} // end namespace irr

#endif
#endif
