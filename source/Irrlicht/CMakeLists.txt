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
	target_compile_definitions(MESHOBJ PRIVATE _IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_)
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
	CSkyBoxSceneNode.cpp
	CVolumeLightSceneNode.cpp
	CTextSceneNode.cpp
	CTriangleBBSelector.cpp
	CTriangleSelector.cpp
	CMeshCache.cpp
	CDefaultSceneNodeAnimatorFactory.cpp
	CDefaultSceneNodeFactory.cpp
	)

option(IRRLICHT_SCENE_LOADER "Build with scene load support" OFF)
if(IRRLICHT_SCENE_LOADER)
	target_sources(SCENEOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/CSceneLoaderIrr.cpp")
	target_compile_definitions(SceneManager PUBLIC "_IRR_COMPILE_WITH_IRR_SCENE_LOADER_")
endif()

function(AddSceneNode name default)
	string(REPLACE " " "" classname "${name}")
	set(classname "C${classname}SceneNode")
	string(REPLACE " " "_" ucname "${name}")
	string(TOUPPER "${ucname}" ucname)
	set(varname "IRRLICHT_SCENE_NODE_${ucname}")
	option(${varname} "Build with ${name} scene node support" ${default})
	if(${varname}) # indirect variable reference
		target_sources(SCENEOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${classname}.cpp")
		target_compile_definitions(SceneManager PRIVATE "_IRR_COMPILE_WITH_${ucname}_SCENENODE_")
	endif()
endfunction()

AddSceneNode(Billboard ON)
AddSceneNode(Cube ON)
AddSceneNode(Octree OFF)
AddSceneNode("Shadow Volume" OFF)
AddSceneNode(SkyDome OFF)
AddSceneNode(Sphere ON)
AddSceneNode(Terrain OFF)
AddSceneNode("Water Surface" OFF)

if(IRRLICHT_SCENE_NODE_TERRAIN)
	target_sources(SCENEOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/CTerrainTriangleSelector.cpp")
endif()

if(IRRLICHT_SCENE_NODE_SHADOW_VOLUME)
	target_compile_definitions(MESHOBJ PRIVATE _IRR_COMPILE_WITH_SHADOW_VOLUME_SCENENODE_)
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

add_library(Device OBJECT
	CIrrDeviceStub.cpp
	CIrrDeviceSDL2.cpp
	)

add_library(VIDEOOBJ OBJECT
	CVideoModeList.cpp
	CFPSCounter.cpp
	)

function(AddVideoDriver tag default name)
	string(TOUPPER "${tag}" ucname)
	list(SUBLIST ARGV 3 -1 files)
	list(TRANSFORM files PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
	set(varname "IRRLICHT_VIDEO_DRIVER_${ucname}")
	option(${varname} "Build with ${name} support" ${default})
	if(${varname}) # indirect variable reference
		message(NOTICE "Building with ${name} support")
		target_sources(VIDEOOBJ PUBLIC ${files})
		target_compile_definitions(Device PRIVATE "_IRR_COMPILE_WITH_${ucname}_")
	endif()
endfunction()

AddVideoDriver(WebGL1 OFF "WebGL (subset of OpenGL ES 2)"
	CWebGL1Driver.cpp
	)

if(IRRLICHT_VIDEO_DRIVER_WEBGL1)
	set(IRRLICHT_VIDEO_DRIVER_OGLES2 ON)
endif()

AddVideoDriver(OpenGL ON "desktop OpenGL"
	COpenGLCacheHandler.cpp
	COpenGLDriver.cpp
	COpenGLShaderMaterialRenderer.cpp
	COpenGLSLMaterialRenderer.cpp
	COpenGLExtensionHandler.cpp
	)

AddVideoDriver(OGLES1 ON "OpenGL ES 1"
	COGLESDriver.cpp
	COGLESExtensionHandler.cpp
	)

AddVideoDriver(OGLES2 ON "OpenGL ES 2+"
	COGLES2Driver.cpp
	COGLES2ExtensionHandler.cpp
	COGLES2FixedPipelineRenderer.cpp
	COGLES2MaterialRenderer.cpp
	COGLES2Renderer2D.cpp
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

AddImageFormat(BMP RW ON)
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
		if(NOT ("${ARGV2}" STREQUAL ""))
			set(reader_name "${ARGV2}")
		endif()
		if(reader_name)
			target_sources(IOOBJ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${reader_name}.cpp")
		endif()
		target_compile_definitions(FileSystem PRIVATE "_IRR_COMPILE_WITH_${ucname}_ARCHIVE_LOADER_")
	endif()
endfunction()

AddArchiveFormat(Mount OFF "CMountPointReader")
AddArchiveFormat(NPK OFF)
AddArchiveFormat(Pak OFF)
AddArchiveFormat(Tar OFF)
AddArchiveFormat(WAD OFF)

option(IRRLICHT_GUI "Build Irrlicht with GUI support" ON)
if(IRRLICHT_GUI)
	add_library(GUIOBJ OBJECT
		BuiltInFont.cpp
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
	if(NOT IRRLICHT_IMAGE_FORMAT_BMP)
		message(WARNING "BMP loader is required for builtin font support")
	endif()
	target_compile_definitions(Device PRIVATE "_IRR_COMPILE_WITH_GUI_")
endif()

add_library(AESGM STATIC EXCLUDE_FROM_ALL
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
	CLogger.cpp
	COSOperator.cpp
	Irrlicht.cpp
	os.cpp
	leakHunter.cpp
	CProfiler.cpp
	utf8.cpp
	)

target_sources(Irrlicht PRIVATE
	$<TARGET_OBJECTS:Device>
	$<TARGET_OBJECTS:MESHOBJ>
		$<TARGET_OBJECTS:MeshFormats>
	$<TARGET_OBJECTS:SCENEOBJ>
		$<TARGET_OBJECTS:SceneManager>
	$<TARGET_OBJECTS:PARTICLEOBJ>
	$<TARGET_OBJECTS:ANIMOBJ>
	$<TARGET_OBJECTS:VIDEOOBJ>
		$<TARGET_OBJECTS:NullDriver>
		$<TARGET_OBJECTS:IMAGEOBJ>
	$<TARGET_OBJECTS:IOOBJ>
		$<TARGET_OBJECTS:FileSystem>
	)

if(IRRLICHT_GUI)
	target_sources(Irrlicht PRIVATE $<TARGET_OBJECTS:GUIOBJ>)
endif()

AddArchiveFormat(Zip OFF FALSE)

if(IRRLICHT_ARCHIVE_FORMAT_ZIP)
	add_library(Zip OBJECT CZipReader.cpp )
	target_sources(Irrlicht PRIVATE $<TARGET_OBJECTS:Zip>)

	option(IRRLICHT_ZIP_ZLIB "Build Zip support with standard compression support" ON)
	if(IRRLICHT_ZIP_ZLIB)
		find_package(ZLIB REQUIRED)
		target_compile_definitions(Zip PRIVATE "_IRR_COMPILE_WITH_ZLIB_")
		target_link_libraries(Irrlicht PRIVATE ZLIB::ZLIB)
	endif()

	option(IRRLICHT_ZIP_BZIP2 "Build Zip support with BZip2 compression support" OFF)
	if(IRRLICHT_ZIP_BZIP2)
		find_package(BZip2 REQUIRED)
		target_compile_definitions(Zip PRIVATE "_IRR_COMPILE_WITH_BZIP2_")
		target_link_libraries(Irrlicht PRIVATE BZip2::BZip2)
	endif()

	option(IRRLICHT_ZIP_LZMA "Build Zip support with LZMA compression support" OFF)
	if(IRRLICHT_ZIP_LZMA)
		target_compile_definitions(Zip PRIVATE "_IRR_COMPILE_WITH_LZMA_")
		target_sources(Irrlicht PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lzma/LzmaDec.c")
	endif()

	option(IRRLICHT_ZIP_ENCRYPTION "Build Zip support with encrypted archive support" OFF)
	if(IRRLICHT_ZIP_ENCRYPTION)
		target_compile_definitions(Zip PRIVATE "_IRR_COMPILE_WITH_ZIP_ENCRYPTION_")
		target_link_libraries(Irrlicht PRIVATE AESGM)
	endif()
endif()

if (IRRLICHT_IMAGE_FORMAT_PNG)
	find_package(PNG REQUIRED)
	target_link_libraries(Irrlicht PUBLIC PNG::PNG)
endif()

if (IRRLICHT_IMAGE_FORMAT_JPG)
	find_package(JPEG REQUIRED)
	target_link_libraries(Irrlicht PUBLIC JPEG::JPEG)
endif()

find_package(SDL2 REQUIRED)
target_link_libraries(Irrlicht PRIVATE SDL2::SDL2)

set(VERSION "${IRRLICHT_VERSION_MAJOR}.${IRRLICHT_VERSION_MINOR}.${IRRLICHT_VERSION_RELEASE}")
set_target_properties(Irrlicht PROPERTIES
	VERSION ${VERSION}
	SOVERSION ${IRRLICHT_VERSION_MAJOR}.${IRRLICHT_VERSION_MINOR}
)
