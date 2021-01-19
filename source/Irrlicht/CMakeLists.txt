include_directories(${CMAKE_SOURCE_DIR}/include)
add_definitions(-DIRRLICHT_EXPORTS=1)
if(GCC OR CLANG)
	add_compile_options(-Wall -pipe -fno-exceptions -fno-rtti -fstrict-aliasing -fvisibility=hidden)
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

option(IRRLICHT_MESH_WRITERS "Build with mesh writing support" OFF)

function(AddMeshFormat name mode default)
	string(TOUPPER "${name}" ucname)
	set(varname "IRRLICHT_MESH_FORMAT_${ucname}")
	option(${varname} "Build with ${name} mesh format support" ${default})
	if(${varname}) # indirect variable reference
		set(loader_name "C${name}MeshFileLoader")
		set(writer_name "C${name}MeshWriter")
		if(ARGV3)
			set(loader_name "${ARGV3}")
		endif()
		target_sources(MeshFormats PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${loader_name}.cpp")
		target_compile_definitions(SceneManager PRIVATE "_IRR_COMPILE_WITH_${ucname}_LOADER_")
		if(mode STREQUAL "RW")
			if(IRRLICHT_MESH_WRITERS)
				target_sources(MeshFormats PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${writer_name}.cpp")
				target_compile_definitions(SceneManager PRIVATE "_IRR_COMPILE_WITH_${ucname}_WRITER_")
			endif()
		elseif(mode STREQUAL "RO")
		else()
			message(SEND_ERROR "Invalid mesh format support mode: [${mode}]")
		endif()
	endif()
endfunction()

add_library(MeshFormats OBJECT
	CMeshTextureLoader.cpp
	)

add_library(SceneManager OBJECT
	CSceneManager.cpp
	)

AddMeshFormat(3DS RO ON)
AddMeshFormat(B3D RW ON)
AddMeshFormat(BSP RO OFF)
AddMeshFormat(CSM RO OFF "CCSMLoader")
AddMeshFormat(Collada RW OFF "CColladaFileLoader")
AddMeshFormat(DMF RO OFF "CDMFLoader")
AddMeshFormat(Irr RW OFF)
AddMeshFormat(LMTS RO OFF)
AddMeshFormat(LWO RO OFF)
AddMeshFormat(MD2 RO OFF)
AddMeshFormat(MD3 RO OFF)
AddMeshFormat(MS3D RO OFF)
AddMeshFormat(MY3D RO OFF)
AddMeshFormat(OBJ RW ON)
AddMeshFormat(OCT RO OFF "COCTLoader")
AddMeshFormat(Ogre RO OFF)
AddMeshFormat(PLY RW OFF)
AddMeshFormat(SMF RO OFF)
AddMeshFormat(STL RW OFF)
AddMeshFormat(X RO ON)

add_library(MESHOBJ OBJECT
	CMeshSceneNode.cpp
	)

add_library(AnimatedMeshSceneNode OBJECT
	CAnimatedMeshSceneNode.cpp
	)

function(require_if option)
	if(${option})
		return()
	endif()
	list(POP_FRONT ARGV)
	foreach(requirer ${ARGV})
		if(${requirer})
			message(NOTICE "Enabling ${option} as required for ${requirer}")
			set(${option} ON PARENT_SCOPE)
		endif()
	endforeach()
endfunction()

option(IRRLICHT_SKINNED_MESH "Support skinned mesh" ON)
require_if(IRRLICHT_SKINNED_MESH IRRLICHT_MESH_FORMAT_B3D IRRLICHT_MESH_FORMAT_MS3D IRRLICHT_MESH_FORMAT_X IRRLICHT_MESH_FORMAT_OGRE)

if(IRRLICHT_SKINNED_MESH)
	target_sources(MESHOBJ PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/CSkinnedMesh.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/CBoneSceneNode.cpp"
		)
	target_compile_definitions(SceneManager PRIVATE _IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_)
	target_compile_definitions(AnimatedMeshSceneNode PRIVATE _IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_)
endif()

if(IRRLICHT_MESH_FORMAT_HALFLIFE)
	target_sources(MESHOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/CAnimatedMeshHalfLife.cpp")
endif()

if(IRRLICHT_MESH_FORMAT_MD2)
	target_sources(MESHOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/CAnimatedMeshMD2.cpp")
endif()

if(IRRLICHT_MESH_FORMAT_MD3)
	target_sources(MESHOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/CAnimatedMeshMD3.cpp")
endif()

if(IRRLICHT_MESH_FORMAT_BSP)
	target_sources(MESHOBJ PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/CQ3LevelMesh.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/CQuake3ShaderSceneNode.cpp"
		)
endif()

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
	)

option(IRRLICHT_SCENE_LOADER "Build with scene load support" OFF)
if(IRRLICHT_SCENE_LOADER)
	target_sources(SCENEOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/CSceneLoaderIrr.cpp")
	target_compile_definitions(SceneManager PUBLIC "_IRR_COMPILE_WITH_IRR_SCENE_LOADER_")
endif()

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

function(AddImageFormat name mode default)
	set(varname "IRRLICHT_IMAGE_FORMAT_${name}")
	option(${varname} "Build with ${name} image format support" ${default})
	if(${varname}) # indirect variable reference
		target_sources(IMAGEOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/CImageLoader${name}.cpp")
		target_compile_definitions(NullDriver PRIVATE "_IRR_COMPILE_WITH_${name}_LOADER_")
		if(mode STREQUAL "RW")
			if(IRRLICHT_IMAGE_WRITERS)
				target_sources(IMAGEOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/CImageWriter${name}.cpp")
				target_compile_definitions(NullDriver PRIVATE "_IRR_COMPILE_WITH_${name}_WRITER_")
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
	CLimitReadFile.cpp
	CMemoryFile.cpp
	CReadFile.cpp
	CWriteFile.cpp
	CXMLReader.cpp
	CXMLWriter.cpp
	irrXML.cpp
	CAttributes.cpp
	lzma/LzmaDec.c
	)

add_library(FileSystem OBJECT
	CFileSystem.cpp
	)

function(AddArchiveFormat name default)
	string(TOUPPER "${name}" ucname)
	set(varname "IRRLICHT_ARCHIVE_FORMAT_${ucname}")
	option(${varname} "Build with ${name} archive format support" ${default})
	if(${varname}) # indirect variable reference
		set(reader_name "C${name}Reader")
		if(ARGV2)
			set(reader_name "${ARGV2}")
		endif()
		target_sources(IOOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${reader_name}.cpp")
		target_compile_definitions(FileSystem PRIVATE "_IRR_COMPILE_WITH_${ucname}_ARCHIVE_LOADER_")
	endif()
endfunction()

AddArchiveFormat(Mount OFF "CMountPointReader")
AddArchiveFormat(NPK OFF)
AddArchiveFormat(Pak OFF)
AddArchiveFormat(Tar OFF)
AddArchiveFormat(WAD OFF)
AddArchiveFormat(Zip OFF)

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
		$<TARGET_OBJECTS:MeshFormats>
	$<TARGET_OBJECTS:SCENEOBJ>
		$<TARGET_OBJECTS:SceneManager>
		$<TARGET_OBJECTS:AnimatedMeshSceneNode>
	$<TARGET_OBJECTS:PARTICLEOBJ>
	$<TARGET_OBJECTS:ANIMOBJ>
	$<TARGET_OBJECTS:VIDEOOBJ>
		$<TARGET_OBJECTS:NullDriver>
		$<TARGET_OBJECTS:DRVROBJ>
		$<TARGET_OBJECTS:IMAGEOBJ>
	$<TARGET_OBJECTS:IOOBJ>
		$<TARGET_OBJECTS:FileSystem>
	$<TARGET_OBJECTS:GUIOBJ>
	)

target_link_libraries(Irrlicht PRIVATE
	AESGM
	z
	bz2
	SDL2
)

if (IRRLICHT_IMAGE_FORMAT_PNG)
	find_package(PNG REQUIRED)
	target_link_libraries(Irrlicht PUBLIC PNG::PNG)
endif()

if (IRRLICHT_IMAGE_FORMAT_JPG)
	find_package(JPEG REQUIRED)
	target_link_libraries(Irrlicht PUBLIC JPEG::JPEG)
endif()

set(VERSION "${IRRLICHT_VERSION_MAJOR}.${IRRLICHT_VERSION_MINOR}.${IRRLICHT_VERSION_RELEASE}")
set_target_properties(Irrlicht PROPERTIES
	VERSION ${VERSION}
	SOVERSION ${IRRLICHT_VERSION_MAJOR}.${IRRLICHT_VERSION_MINOR}
)
