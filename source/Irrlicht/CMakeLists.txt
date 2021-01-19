include_directories(${CMAKE_SOURCE_DIR}/include)
add_definitions(-DIRRLICHT_EXPORTS=1)
if(GCC OR CLANG)
	add_compile_options(-Wall -pipe -fno-exceptions -fno-rtti -fstrict-aliasing -fvisibility=hidden)
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

add_library(MESHLOADER OBJECT
	CBSPMeshFileLoader.cpp
	CMD2MeshFileLoader.cpp
	CMD3MeshFileLoader.cpp
	CMS3DMeshFileLoader.cpp
	CB3DMeshFileLoader.cpp
	C3DSMeshFileLoader.cpp
	COgreMeshFileLoader.cpp
	COBJMeshFileLoader.cpp
	CColladaFileLoader.cpp
	CCSMLoader.cpp
	CDMFLoader.cpp
	CLMTSMeshFileLoader.cpp
	CMY3DMeshFileLoader.cpp
	COCTLoader.cpp
	CXMeshFileLoader.cpp
	CIrrMeshFileLoader.cpp
	CSTLMeshFileLoader.cpp
	CLWOMeshFileLoader.cpp
	CPLYMeshFileLoader.cpp
	CSMFMeshFileLoader.cpp
	CMeshTextureLoader.cpp
	)

add_library(MESHWRITER OBJECT
	CColladaMeshWriter.cpp
	CIrrMeshWriter.cpp
	CSTLMeshWriter.cpp
	COBJMeshWriter.cpp
	CPLYMeshWriter.cpp
	CB3DMeshWriter.cpp
	)

add_library(MESHOBJ OBJECT
	CSkinnedMesh.cpp
	CBoneSceneNode.cpp
	CMeshSceneNode.cpp
	CAnimatedMeshSceneNode.cpp
	CAnimatedMeshMD2.cpp
	CAnimatedMeshMD3.cpp
	CQ3LevelMesh.cpp
	CQuake3ShaderSceneNode.cpp
	CAnimatedMeshHalfLife.cpp
	)

add_library(SCENEOBJ OBJECT
	CBillboardSceneNode.cpp
	CCameraSceneNode.cpp
	CDummyTransformationSceneNode.cpp
	CEmptySceneNode.cpp
	CGeometryCreator.cpp
	CLightSceneNode.cpp
	CMeshManipulator.cpp
	CMetaTriangleSelector.cpp
	COctreeSceneNode.cpp
	COctreeTriangleSelector.cpp
	CSceneCollisionManager.cpp
	CSceneManager.cpp
	CShadowVolumeSceneNode.cpp
	CSkyBoxSceneNode.cpp
	CSkyDomeSceneNode.cpp
	CTerrainSceneNode.cpp
	CTerrainTriangleSelector.cpp
	CVolumeLightSceneNode.cpp
	CCubeSceneNode.cpp
	CSphereSceneNode.cpp
	CTextSceneNode.cpp
	CTriangleBBSelector.cpp
	CTriangleSelector.cpp
	CWaterSurfaceSceneNode.cpp
	CMeshCache.cpp
	CDefaultSceneNodeAnimatorFactory.cpp
	CDefaultSceneNodeFactory.cpp
	CSceneLoaderIrr.cpp
	)

add_library(PARTICLEOBJ OBJECT
	CParticleAnimatedMeshSceneNodeEmitter.cpp
	CParticleBoxEmitter.cpp
	CParticleCylinderEmitter.cpp
	CParticleMeshEmitter.cpp
	CParticlePointEmitter.cpp
	CParticleRingEmitter.cpp
	CParticleSphereEmitter.cpp
	CParticleAttractionAffector.cpp
	CParticleFadeOutAffector.cpp
	CParticleGravityAffector.cpp
	CParticleRotationAffector.cpp
	CParticleSystemSceneNode.cpp
	CParticleScaleAffector.cpp
	)

add_library(ANIMOBJ OBJECT
	CSceneNodeAnimatorCameraFPS.cpp
	CSceneNodeAnimatorCameraMaya.cpp
	CSceneNodeAnimatorCollisionResponse.cpp
	CSceneNodeAnimatorDelete.cpp
	CSceneNodeAnimatorFlyCircle.cpp
	CSceneNodeAnimatorFlyStraight.cpp
	CSceneNodeAnimatorFollowSpline.cpp
	CSceneNodeAnimatorRotation.cpp
	CSceneNodeAnimatorTexture.cpp
	)

add_library(NullDriver OBJECT
	CNullDriver.cpp
	)

add_library(DRVROBJ OBJECT
	COpenGLCacheHandler.cpp
	COpenGLDriver.cpp
	COpenGLShaderMaterialRenderer.cpp
	COpenGLSLMaterialRenderer.cpp
	COpenGLExtensionHandler.cpp
	COGLESDriver.cpp
	COGLESExtensionHandler.cpp
	COGLES2Driver.cpp
	COGLES2ExtensionHandler.cpp
	COGLES2FixedPipelineRenderer.cpp
	COGLES2MaterialRenderer.cpp
	COGLES2Renderer2D.cpp
	CWebGL1Driver.cpp
	)

add_library(IMAGEOBJ OBJECT
	CColorConverter.cpp
	CImage.cpp
	)

option(IRRLICHT_IMAGE_WRITERS "Build with image writing support" ON)
option(IRRLICHT_MESH_WRITERS "Build with mesh writing support" OFF)

function(AddImageFormat name mode default)
	set(varname "IRRLICHT_IMAGE_FORMAT_${name}")
	option(${varname} "Build with ${name} image format support" ${default})
	if(${varname}) # indirect variable reference
		target_sources(IMAGEOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/CImageLoader${name}.cpp")
		target_compile_definitions(NullDriver PUBLIC "_IRR_COMPILE_WITH_${name}_LOADER_")
		if(mode STREQUAL "RW")
			if(IRRLICHT_IMAGE_WRITERS)
				target_sources(IMAGEOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/CImageWriter${name}.cpp")
				target_compile_definitions(NullDriver PUBLIC "_IRR_COMPILE_WITH_${name}_WRITER_")
			endif()
		elseif(mode STREQUAL "RO")
		else()
			message(SEND_ERROR "Invalid image format support mode: [${mode}]")
		endif()
	endif()
endfunction()

AddImageFormat(BMP RW OFF)
AddImageFormat(DDS RO OFF) # _IRR_COMPILE_WITH_DDS_DECODER_LOADER_ patended
AddImageFormat(JPG RW ON)
AddImageFormat(LMP RO OFF)
AddImageFormat(PCX RW OFF)
AddImageFormat(PNG RW ON)
AddImageFormat(PPM RW OFF)
AddImageFormat(PSD RW OFF)
AddImageFormat(PVR RO OFF)
AddImageFormat(RGB RO OFF)
AddImageFormat(TGA RW OFF)
AddImageFormat(WAL RO OFF)

add_library(VIDEOOBJ OBJECT
	CVideoModeList.cpp
	CFPSCounter.cpp
	)

add_library(IOOBJ OBJECT
	CFileList.cpp
	CFileSystem.cpp
	CLimitReadFile.cpp
	CMemoryFile.cpp
	CReadFile.cpp
	CWriteFile.cpp
	CXMLReader.cpp
	CXMLWriter.cpp
	CWADReader.cpp
	CZipReader.cpp
	CPakReader.cpp
	CNPKReader.cpp
	CTarReader.cpp
	CMountPointReader.cpp
	irrXML.cpp
	CAttributes.cpp
	lzma/LzmaDec.c
	)

add_library(GUIOBJ OBJECT
	CGUIButton.cpp
	CGUICheckBox.cpp
	CGUIComboBox.cpp
	CGUIContextMenu.cpp
	CGUIEditBox.cpp
	CGUIEnvironment.cpp
	CGUIFileOpenDialog.cpp
	CGUIFont.cpp
	CGUIImage.cpp
	CGUIInOutFader.cpp
	CGUIListBox.cpp
	CGUIMenu.cpp
	CGUIMeshViewer.cpp
	CGUIMessageBox.cpp
	CGUIModalScreen.cpp
	CGUIScrollBar.cpp
	CGUISpinBox.cpp
	CGUISkin.cpp
	CGUIStaticText.cpp
	CGUITabControl.cpp
	CGUITable.cpp
	CGUIToolBar.cpp
	CGUIWindow.cpp
	CGUIColorSelectDialog.cpp
	CDefaultGUIElementFactory.cpp
	CGUISpriteBank.cpp
	CGUIImageList.cpp
	CGUITreeView.cpp
	CGUIProfiler.cpp
	)

add_library(AESGM STATIC
	aesGladman/aescrypt.cpp
	aesGladman/aeskey.cpp
	aesGladman/aestab.cpp
	aesGladman/fileenc.cpp
	aesGladman/hmac.cpp
	aesGladman/prng.cpp
	aesGladman/pwd2key.cpp
	aesGladman/sha1.cpp
	aesGladman/sha2.cpp
	)

add_library(Irrlicht SHARED
	CIrrDeviceSDL2.cpp
	CIrrDeviceStub.cpp
	CLogger.cpp
	COSOperator.cpp
	Irrlicht.cpp
	os.cpp
	leakHunter.cpp
	CProfiler.cpp
	utf8.cpp
	)

target_sources(Irrlicht PRIVATE
	$<TARGET_OBJECTS:MESHOBJ>
		$<TARGET_OBJECTS:MESHLOADER>
		$<TARGET_OBJECTS:MESHWRITER>
	$<TARGET_OBJECTS:SCENEOBJ>
	$<TARGET_OBJECTS:PARTICLEOBJ>
	$<TARGET_OBJECTS:ANIMOBJ>
	$<TARGET_OBJECTS:VIDEOOBJ>
		$<TARGET_OBJECTS:NullDriver>
		$<TARGET_OBJECTS:DRVROBJ>
		$<TARGET_OBJECTS:IMAGEOBJ>
	$<TARGET_OBJECTS:IOOBJ>
	$<TARGET_OBJECTS:GUIOBJ>
	)

target_link_libraries(Irrlicht PRIVATE
	AESGM
	z
	jpeg
	png
	bz2
	SDL2
)

set(VERSION "${IRRLICHT_VERSION_MAJOR}.${IRRLICHT_VERSION_MINOR}.${IRRLICHT_VERSION_RELEASE}")
set_target_properties(Irrlicht PROPERTIES
	VERSION ${VERSION}
	SOVERSION ${IRRLICHT_VERSION_MAJOR}.${IRRLICHT_VERSION_MINOR}
)
