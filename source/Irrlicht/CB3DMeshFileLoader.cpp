// Copyright (C) 2006-2012 Luke Hoschke
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// B3D Mesh loader
// File format designed by Mark Sibly for the Blitz3D engine and has been
// declared public domain

#ifdef _IRR_COMPILE_WITH_B3D_LOADER_

#include "CB3DMeshFileLoader.h"
#include "CMeshTextureLoader.h"

#include "IVideoDriver.h"
#include "IFileSystem.h"
#include "os.h"

#ifdef _DEBUG
#define _B3D_READER_DEBUG
#endif

namespace irr
{
namespace scene
{

//! Constructor
CB3DMeshFileLoader::CB3DMeshFileLoader(scene::ISceneManager* smgr)
: SceneManager(smgr), AnimatedMesh(0), B3DFile(0), NormalsInFile(false),
	HasVertexColors(false), ShowWarning(true)
{
	#ifdef _DEBUG
	setDebugName("CB3DMeshFileLoader");
	#endif

	TextureLoader = new CMeshTextureLoader( SceneManager->getFileSystem(), SceneManager->getVideoDriver() );
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CB3DMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "b3d" );
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* CB3DMeshFileLoader::createMesh(io::IReadFile* file)
{
	if (!file)
		return 0;

	if ( getMeshTextureLoader() )
		getMeshTextureLoader()->setMeshFile(file);

	B3DFile = file;
	AnimatedMesh = new scene::CSkinnedMesh();
	ShowWarning = true; // If true a warning is issued if too many textures are used
	VerticesStart=0;

	if ( load() )
	{
		AnimatedMesh->finalize();
	}
	else
	{
		AnimatedMesh->drop();
		AnimatedMesh = 0;
	}

	return AnimatedMesh;
}


bool CB3DMeshFileLoader::load()
{
	B3dStack.clear();

	NormalsInFile=false;
	HasVertexColors=false;

	//------ Get header ------

	SB3dChunkHeader header;
	B3DFile->read(&header, sizeof(header));
#ifdef __BIG_ENDIAN__
	header.size = os::Byteswap::byteswap(header.size);
#endif

	if ( strncmp( header.name, "BB3D", 4 ) != 0 )
	{
		os::Printer::log("File is not a b3d file. Loading failed (No header found)", B3DFile->getFileName(), ELL_ERROR);
		return false;
	}

	// Add main chunk...
	B3dStack.push_back(SB3dChunk(header, B3DFile->getPos()-8));

	// Get file version, but ignore it, as it's not important with b3d files...
	s32 fileVersion;
	B3DFile->read(&fileVersion, sizeof(fileVersion));
#ifdef __BIG_ENDIAN__
	fileVersion = os::Byteswap::byteswap(fileVersion);
#endif

	//------ Read main chunk ------

	while ( (B3dStack.getLast().startposition + B3dStack.getLast().length) > B3DFile->getPos() )
	{
		B3DFile->read(&header, sizeof(header));
#ifdef __BIG_ENDIAN__
		header.size = os::Byteswap::byteswap(header.size);
#endif
		B3dStack.push_back(SB3dChunk(header, B3DFile->getPos()-8));

		if ( strncmp( B3dStack.getLast().name, "TEXS", 4 ) == 0 )
		{
			if (!readChunkTEXS())
				return false;
		}
		else if ( strncmp( B3dStack.getLast().name, "BRUS", 4 ) == 0 )
		{
			if (!readChunkBRUS())
				return false;
		}
		else if ( strncmp( B3dStack.getLast().name, "NODE", 4 ) == 0 )
		{
			if (!readChunkNODE((CSkinnedMesh::SJoint*)0) )
				return false;
		}
		else
		{
			os::Printer::log("Unknown chunk found in mesh base - skipping");
			B3DFile->seek(B3dStack.getLast().startposition + B3dStack.getLast().length);
			B3dStack.erase(B3dStack.size()-1);
		}
	}

	B3dStack.clear();

	BaseVertices.clear();
	AnimatedVertices_VertexID.clear();
	AnimatedVertices_BufferID.clear();

	Materials.clear();
	Textures.clear();

	return true;
}


bool CB3DMeshFileLoader::readChunkNODE(CSkinnedMesh::SJoint *inJoint)
{
	CSkinnedMesh::SJoint *joint = AnimatedMesh->addJoint(inJoint);
	readString(joint->Name);

#ifdef _B3D_READER_DEBUG
	core::stringc logStr;
	for ( u32 i=1; i < B3dStack.size(); ++i )
		logStr += "-";
	logStr += "read ChunkNODE";
	os::Printer::log(logStr.c_str(), joint->Name.c_str(), ELL_DEBUG);
#endif

	f32 position[3], scale[3], rotation[4];

	readFloats(position, 3);
	readFloats(scale, 3);
	readFloats(rotation, 4);

	joint->Animatedposition = core::vector3df(position[0],position[1],position[2]) ;
	joint->Animatedscale = core::vector3df(scale[0],scale[1],scale[2]);
	joint->Animatedrotation = core::quaternion(rotation[1], rotation[2], rotation[3], rotation[0]);

	//Build LocalMatrix:

	core::matrix4 positionMatrix;
	positionMatrix.setTranslation( joint->Animatedposition );
	core::matrix4 scaleMatrix;
	scaleMatrix.setScale( joint->Animatedscale );
	core::matrix4 rotationMatrix;
	joint->Animatedrotation.getMatrix_transposed(rotationMatrix);

	joint->LocalMatrix = positionMatrix * rotationMatrix * scaleMatrix;

	if (inJoint)
		joint->GlobalMatrix = inJoint->GlobalMatrix * joint->LocalMatrix;
	else
		joint->GlobalMatrix = joint->LocalMatrix;

	while(B3dStack.getLast().startposition + B3dStack.getLast().length > B3DFile->getPos()) // this chunk repeats
	{
		SB3dChunkHeader header;
		B3DFile->read(&header, sizeof(header));
#ifdef __BIG_ENDIAN__
		header.size = os::Byteswap::byteswap(header.size);
#endif

		B3dStack.push_back(SB3dChunk(header, B3DFile->getPos()-8));

		if ( strncmp( B3dStack.getLast().name, "NODE", 4 ) == 0 )
		{
			if (!readChunkNODE(joint))
				return false;
		}
		else if ( strncmp( B3dStack.getLast().name, "MESH", 4 ) == 0 )
		{
			VerticesStart=BaseVertices.size();
			if (!readChunkMESH(joint))
				return false;
		}
		else if ( strncmp( B3dStack.getLast().name, "BONE", 4 ) == 0 )
		{
			if (!readChunkBONE(joint))
				return false;
		}
		else if ( strncmp( B3dStack.getLast().name, "KEYS", 4 ) == 0 )
		{
			if(!readChunkKEYS(joint))
				return false;
		}
		else if ( strncmp( B3dStack.getLast().name, "ANIM", 4 ) == 0 )
		{
			if (!readChunkANIM())
				return false;
		}
		else
		{
			os::Printer::log("Unknown chunk found in node chunk - skipping");
			B3DFile->seek(B3dStack.getLast().startposition + B3dStack.getLast().length);
			B3dStack.erase(B3dStack.size()-1);
		}
	}

	B3dStack.erase(B3dStack.size()-1);

	return true;
}


bool CB3DMeshFileLoader::readChunkMESH(CSkinnedMesh::SJoint *inJoint)
{
#ifdef _B3D_READER_DEBUG
	core::stringc logStr;
	for ( u32 i=1; i < B3dStack.size(); ++i )
		logStr += "-";
	logStr += "read ChunkMESH";
	os::Printer::log(logStr.c_str(), ELL_DEBUG);
#endif

	s32 brushID;
	B3DFile->read(&brushID, sizeof(brushID));
#ifdef __BIG_ENDIAN__
	brushID = os::Byteswap::byteswap(brushID);
#endif

	NormalsInFile=false;
	HasVertexColors=false;

	while((B3dStack.getLast().startposition + B3dStack.getLast().length) > B3DFile->getPos()) //this chunk repeats
	{
		SB3dChunkHeader header;
		B3DFile->read(&header, sizeof(header));
#ifdef __BIG_ENDIAN__
		header.size = os::Byteswap::byteswap(header.size);
#endif

		B3dStack.push_back(SB3dChunk(header, B3DFile->getPos()-8));

		if ( strncmp( B3dStack.getLast().name, "VRTS", 4 ) == 0 )
		{
			if (!readChunkVRTS(inJoint))
				return false;
		}
		else if ( strncmp( B3dStack.getLast().name, "TRIS", 4 ) == 0 )
		{
			scene::SSkinMeshBuffer *meshBuffer = AnimatedMesh->addMeshBuffer();

			if (brushID!=-1)
			{
				loadTextures(Materials[brushID]);
				meshBuffer->Material=Materials[brushID].Material;
			}

			if(readChunkTRIS(meshBuffer,AnimatedMesh->getMeshBuffers().size()-1, VerticesStart)==false)
				return false;

			if (!NormalsInFile)
			{
				s32 i;

				for ( i=0; i<(s32)meshBuffer->Indices.size(); i+=3)
				{
					core::plane3df p(meshBuffer->getVertex(meshBuffer->Indices[i+0])->Pos,
							meshBuffer->getVertex(meshBuffer->Indices[i+1])->Pos,
							meshBuffer->getVertex(meshBuffer->Indices[i+2])->Pos);

					meshBuffer->getVertex(meshBuffer->Indices[i+0])->Normal += p.Normal;
					meshBuffer->getVertex(meshBuffer->Indices[i+1])->Normal += p.Normal;
					meshBuffer->getVertex(meshBuffer->Indices[i+2])->Normal += p.Normal;
				}

				for ( i = 0; i<(s32)meshBuffer->getVertexCount(); ++i )
				{
					meshBuffer->getVertex(i)->Normal.normalize();
					BaseVertices[VerticesStart+i].Normal=meshBuffer->getVertex(i)->Normal;
				}
			}
		}
		else
		{
			os::Printer::log("Unknown chunk found in mesh - skipping");
			B3DFile->seek(B3dStack.getLast().startposition + B3dStack.getLast().length);
			B3dStack.erase(B3dStack.size()-1);
		}
	}

	B3dStack.erase(B3dStack.size()-1);

	return true;
}


/*
VRTS:
  int flags                   ;1=normal values present, 2=rgba values present
  int tex_coord_sets          ;texture coords per vertex (eg: 1 for simple U/V) max=8
				but we only support 3
  int tex_coord_set_size      ;components per set (eg: 2 for simple U/V) max=4
  {
  float x,y,z                 ;always present
  float nx,ny,nz              ;vertex normal: present if (flags&1)
  float red,green,blue,alpha  ;vertex color: present if (flags&2)
  float tex_coords[tex_coord_sets][tex_coord_set_size]	;tex coords
  }
*/
bool CB3DMeshFileLoader::readChunkVRTS(CSkinnedMesh::SJoint *inJoint)
{
#ifdef _B3D_READER_DEBUG
	core::stringc logStr;
	for ( u32 i=1; i < B3dStack.size(); ++i )
		logStr += "-";
	logStr += "ChunkVRTS";
	os::Printer::log(logStr.c_str(), ELL_DEBUG);
#endif

	const s32 max_tex_coords = 3;
	s32 flags, tex_coord_sets, tex_coord_set_size;

	B3DFile->read(&flags, sizeof(flags));
	B3DFile->read(&tex_coord_sets, sizeof(tex_coord_sets));
	B3DFile->read(&tex_coord_set_size, sizeof(tex_coord_set_size));
#ifdef __BIG_ENDIAN__
	flags = os::Byteswap::byteswap(flags);
	tex_coord_sets = os::Byteswap::byteswap(tex_coord_sets);
	tex_coord_set_size = os::Byteswap::byteswap(tex_coord_set_size);
#endif

	if (tex_coord_sets >= max_tex_coords || tex_coord_set_size >= 4) // Something is wrong
	{
		os::Printer::log("tex_coord_sets or tex_coord_set_size too big", B3DFile->getFileName(), ELL_ERROR);
		return false;
	}

	//------ Allocate Memory, for speed -----------//

	s32 numberOfReads = 3;

	if (flags & 1)
	{
		NormalsInFile = true;
		numberOfReads += 3;
	}
	if (flags & 2)
	{
		numberOfReads += 4;
		HasVertexColors=true;
	}

	numberOfReads += tex_coord_sets*tex_coord_set_size;

	const s32 memoryNeeded = (B3dStack.getLast().length / sizeof(f32)) / numberOfReads;

	BaseVertices.reallocate(memoryNeeded + BaseVertices.size() + 1);
	AnimatedVertices_VertexID.reallocate(memoryNeeded + AnimatedVertices_VertexID.size() + 1);

	//--------------------------------------------//

	while( (B3dStack.getLast().startposition + B3dStack.getLast().length) > B3DFile->getPos()) // this chunk repeats
	{
		f32 position[3];
		f32 normal[3]={0.f, 0.f, 0.f};
		f32 color[4]={1.0f, 1.0f, 1.0f, 1.0f};
		f32 tex_coords[max_tex_coords][4];

		readFloats(position, 3);

		if (flags & 1)
			readFloats(normal, 3);
		if (flags & 2)
			readFloats(color, 4);

		for (s32 i=0; i<tex_coord_sets; ++i)
			readFloats(tex_coords[i], tex_coord_set_size);

		f32 tu=0.0f, tv=0.0f;
		if (tex_coord_sets >= 1 && tex_coord_set_size >= 2)
		{
			tu=tex_coords[0][0];
			tv=tex_coords[0][1];
		}

		f32 tu2=0.0f, tv2=0.0f;
		if (tex_coord_sets>1 && tex_coord_set_size>1)
		{
			tu2=tex_coords[1][0];
			tv2=tex_coords[1][1];
		}

		// Create Vertex...
		video::S3DVertex2TCoords Vertex(position[0], position[1], position[2],
				normal[0], normal[1], normal[2],
				video::SColorf(color[0], color[1], color[2], color[3]).toSColor(),
				tu, tv, tu2, tv2);

		// Transform the Vertex position by nested node...
		inJoint->GlobalMatrix.transformVect(Vertex.Pos);
		inJoint->GlobalMatrix.rotateVect(Vertex.Normal);

		//Add it...
		BaseVertices.push_back(Vertex);

		AnimatedVertices_VertexID.push_back(-1);
		AnimatedVertices_BufferID.push_back(-1);
	}

	B3dStack.erase(B3dStack.size()-1);

	return true;
}


bool CB3DMeshFileLoader::readChunkTRIS(scene::SSkinMeshBuffer *meshBuffer, u32 meshBufferID, s32 vertices_Start)
{
#ifdef _B3D_READER_DEBUG
	core::stringc logStr;
	for ( u32 i=1; i < B3dStack.size(); ++i )
		logStr += "-";
	logStr += "ChunkTRIS";
	os::Printer::log(logStr.c_str(), ELL_DEBUG);
#endif

	bool showVertexWarning=false;

	s32 triangle_brush_id; // Note: Irrlicht can't have different brushes for each triangle (using a workaround)
	B3DFile->read(&triangle_brush_id, sizeof(triangle_brush_id));
#ifdef __BIG_ENDIAN__
	triangle_brush_id = os::Byteswap::byteswap(triangle_brush_id);
#endif

	SB3dMaterial *B3dMaterial;

	if (triangle_brush_id != -1)
	{
		loadTextures(Materials[triangle_brush_id]);
		B3dMaterial = &Materials[triangle_brush_id];
		meshBuffer->Material = B3dMaterial->Material;
	}
	else
		B3dMaterial = 0;

	const s32 memoryNeeded = B3dStack.getLast().length / sizeof(s32);
	meshBuffer->Indices.reallocate(memoryNeeded + meshBuffer->Indices.size() + 1);

	while((B3dStack.getLast().startposition + B3dStack.getLast().length) > B3DFile->getPos()) // this chunk repeats
	{
		s32 vertex_id[3];

		B3DFile->read(vertex_id, 3*sizeof(s32));
#ifdef __BIG_ENDIAN__
		vertex_id[0] = os::Byteswap::byteswap(vertex_id[0]);
		vertex_id[1] = os::Byteswap::byteswap(vertex_id[1]);
		vertex_id[2] = os::Byteswap::byteswap(vertex_id[2]);
#endif

		//Make Ids global:
		vertex_id[0] += vertices_Start;
		vertex_id[1] += vertices_Start;
		vertex_id[2] += vertices_Start;

		for(s32 i=0; i<3; ++i)
		{
			if ((u32)vertex_id[i] >= AnimatedVertices_VertexID.size())
			{
				os::Printer::log("Illegal vertex index found", B3DFile->getFileName(), ELL_ERROR);
				return false;
			}

			if (AnimatedVertices_VertexID[ vertex_id[i] ] != -1)
			{
				if ( AnimatedVertices_BufferID[ vertex_id[i] ] != (s32)meshBufferID ) //If this vertex is linked in a different meshbuffer
				{
					AnimatedVertices_VertexID[ vertex_id[i] ] = -1;
					AnimatedVertices_BufferID[ vertex_id[i] ] = -1;
					showVertexWarning=true;
				}
			}
			if (AnimatedVertices_VertexID[ vertex_id[i] ] == -1) //If this vertex is not in the meshbuffer
			{
				//Check for lightmapping:
				if (BaseVertices[ vertex_id[i] ].TCoords2 != core::vector2df(0.f,0.f))
					meshBuffer->convertTo2TCoords(); //Will only affect the meshbuffer the first time this is called

				//Add the vertex to the meshbuffer:
				if (meshBuffer->VertexType == video::EVT_STANDARD)
					meshBuffer->Vertices_Standard.push_back( BaseVertices[ vertex_id[i] ] );
				else
					meshBuffer->Vertices_2TCoords.push_back(BaseVertices[ vertex_id[i] ] );

				//create vertex id to meshbuffer index link:
				AnimatedVertices_VertexID[ vertex_id[i] ] = meshBuffer->getVertexCount()-1;
				AnimatedVertices_BufferID[ vertex_id[i] ] = meshBufferID;

				if (B3dMaterial)
				{
					// Apply Material/Color/etc...
					video::S3DVertex *Vertex=meshBuffer->getVertex(meshBuffer->getVertexCount()-1);

					if (!HasVertexColors)
						Vertex->Color=B3dMaterial->Material.DiffuseColor;
					else if (Vertex->Color.getAlpha() == 255)
						Vertex->Color.setAlpha( (s32)(B3dMaterial->alpha * 255.0f) );

					// Use texture's scale
					if (B3dMaterial->Textures[0])
					{
						Vertex->TCoords.X *= B3dMaterial->Textures[0]->Xscale;
						Vertex->TCoords.Y *= B3dMaterial->Textures[0]->Yscale;
					}
					/*
					if (B3dMaterial->Textures[1])
					{
						Vertex->TCoords2.X *=B3dMaterial->Textures[1]->Xscale;
						Vertex->TCoords2.Y *=B3dMaterial->Textures[1]->Yscale;
					}
					*/
				}
			}
		}

		meshBuffer->Indices.push_back( AnimatedVertices_VertexID[ vertex_id[0] ] );
		meshBuffer->Indices.push_back( AnimatedVertices_VertexID[ vertex_id[1] ] );
		meshBuffer->Indices.push_back( AnimatedVertices_VertexID[ vertex_id[2] ] );
	}

	B3dStack.erase(B3dStack.size()-1);

	if (showVertexWarning)
		os::Printer::log("B3dMeshLoader: Warning, different meshbuffers linking to the same vertex, this will cause problems with animated meshes");

	return true;
}


bool CB3DMeshFileLoader::readChunkBONE(CSkinnedMesh::SJoint *inJoint)
{
#ifdef _B3D_READER_DEBUG
	core::stringc logStr;
	for ( u32 i=1; i < B3dStack.size(); ++i )
		logStr += "-";
	logStr += "read ChunkBONE";
	os::Printer::log(logStr.c_str(), ELL_DEBUG);
#endif

	if (B3dStack.getLast().length > 8)
	{
		while((B3dStack.getLast().startposition + B3dStack.getLast().length) > B3DFile->getPos()) // this chunk repeats
		{
			u32 globalVertexID;
			f32 strength;
			B3DFile->read(&globalVertexID, sizeof(globalVertexID));
			B3DFile->read(&strength, sizeof(strength));
#ifdef __BIG_ENDIAN__
			globalVertexID = os::Byteswap::byteswap(globalVertexID);
			strength = os::Byteswap::byteswap(strength);
#endif
			globalVertexID += VerticesStart;

			if (AnimatedVertices_VertexID[globalVertexID]==-1)
			{
				os::Printer::log("B3dMeshLoader: Weight has bad vertex id (no link to meshbuffer index found)");
			}
			else if (strength >0)
			{
				CSkinnedMesh::SWeight *weight=AnimatedMesh->addWeight(inJoint);
				weight->strength=strength;
				//Find the meshbuffer and Vertex index from the Global Vertex ID:
				weight->vertex_id = AnimatedVertices_VertexID[globalVertexID];
				weight->buffer_id = AnimatedVertices_BufferID[globalVertexID];
			}
		}
	}

	B3dStack.erase(B3dStack.size()-1);
	return true;
}


bool CB3DMeshFileLoader::readChunkKEYS(CSkinnedMesh::SJoint *inJoint)
{
#ifdef _B3D_READER_DEBUG
	// Only print first, that's just too much output otherwise
	if ( !inJoint || (inJoint->PositionKeys.empty() && inJoint->ScaleKeys.empty() && inJoint->RotationKeys.empty()) )
	{
		core::stringc logStr;
		for ( u32 i=1; i < B3dStack.size(); ++i )
			logStr += "-";
		logStr += "read ChunkKEYS";
		os::Printer::log(logStr.c_str(), ELL_DEBUG);
	}
#endif

	s32 flags;
	B3DFile->read(&flags, sizeof(flags));
#ifdef __BIG_ENDIAN__
	flags = os::Byteswap::byteswap(flags);
#endif

	CSkinnedMesh::SPositionKey *oldPosKey=0;
	core::vector3df oldPos[2];
	CSkinnedMesh::SScaleKey *oldScaleKey=0;
	core::vector3df oldScale[2];
	CSkinnedMesh::SRotationKey *oldRotKey=0;
	core::quaternion oldRot[2];
	bool isFirst[3]={true,true,true};
	while((B3dStack.getLast().startposition + B3dStack.getLast().length) > B3DFile->getPos()) //this chunk repeats
	{
		s32 frame;

		B3DFile->read(&frame, sizeof(frame));
		#ifdef __BIG_ENDIAN__
		frame = os::Byteswap::byteswap(frame);
		#endif

		// Add key frames, frames in Irrlicht are zero-based
		f32 data[4];
		if (flags & 1)
		{
			readFloats(data, 3);
			if ((oldPosKey!=0) && (oldPos[0]==oldPos[1]))
			{
				const core::vector3df pos(data[0], data[1], data[2]);
				if (oldPos[1]==pos)
					oldPosKey->frame = (f32)frame-1;
				else
				{
					oldPos[0]=oldPos[1];
					oldPosKey=AnimatedMesh->addPositionKey(inJoint);
					oldPosKey->frame = (f32)frame-1;
					oldPos[1].set(oldPosKey->position.set(pos));
				}
			}
			else if (oldPosKey==0 && isFirst[0])
			{
				oldPosKey=AnimatedMesh->addPositionKey(inJoint);
				oldPosKey->frame = (f32)frame-1;
				oldPos[0].set(oldPosKey->position.set(data[0], data[1], data[2]));
				oldPosKey=0;
				isFirst[0]=false;
			}
			else
			{
				if (oldPosKey!=0)
					oldPos[0]=oldPos[1];
				oldPosKey=AnimatedMesh->addPositionKey(inJoint);
				oldPosKey->frame = (f32)frame-1;
				oldPos[1].set(oldPosKey->position.set(data[0], data[1], data[2]));
			}
		}
		if (flags & 2)
		{
			readFloats(data, 3);
			if ((oldScaleKey!=0) && (oldScale[0]==oldScale[1]))
			{
				const core::vector3df scale(data[0], data[1], data[2]);
				if (oldScale[1]==scale)
					oldScaleKey->frame = (f32)frame-1;
				else
				{
					oldScale[0]=oldScale[1];
					oldScaleKey=AnimatedMesh->addScaleKey(inJoint);
					oldScaleKey->frame = (f32)frame-1;
					oldScale[1].set(oldScaleKey->scale.set(scale));
				}
			}
			else if (oldScaleKey==0 && isFirst[1])
			{
				oldScaleKey=AnimatedMesh->addScaleKey(inJoint);
				oldScaleKey->frame = (f32)frame-1;
				oldScale[0].set(oldScaleKey->scale.set(data[0], data[1], data[2]));
				oldScaleKey=0;
				isFirst[1]=false;
			}
			else
			{
				if (oldScaleKey!=0)
					oldScale[0]=oldScale[1];
				oldScaleKey=AnimatedMesh->addScaleKey(inJoint);
				oldScaleKey->frame = (f32)frame-1;
				oldScale[1].set(oldScaleKey->scale.set(data[0], data[1], data[2]));
			}
		}
		if (flags & 4)
		{
			readFloats(data, 4);
			if ((oldRotKey!=0) && (oldRot[0]==oldRot[1]))
			{
				// meant to be in this order since b3d stores W first
				const core::quaternion rot(data[1], data[2], data[3], data[0]);
				if (oldRot[1]==rot)
					oldRotKey->frame = (f32)frame-1;
				else
				{
					oldRot[0]=oldRot[1];
					oldRotKey=AnimatedMesh->addRotationKey(inJoint);
					oldRotKey->frame = (f32)frame-1;
					oldRot[1].set(oldRotKey->rotation.set(data[1], data[2], data[3], data[0]));
					oldRot[1].normalize();
				}
			}
			else if (oldRotKey==0 && isFirst[2])
			{
				oldRotKey=AnimatedMesh->addRotationKey(inJoint);
				oldRotKey->frame = (f32)frame-1;
				// meant to be in this order since b3d stores W first
				oldRot[0].set(oldRotKey->rotation.set(data[1], data[2], data[3], data[0]));
				oldRot[0].normalize();
				oldRotKey=0;
				isFirst[2]=false;
			}
			else
			{
				if (oldRotKey!=0)
					oldRot[0]=oldRot[1];
				oldRotKey=AnimatedMesh->addRotationKey(inJoint);
				oldRotKey->frame = (f32)frame-1;
				// meant to be in this order since b3d stores W first
				oldRot[1].set(oldRotKey->rotation.set(data[1], data[2], data[3], data[0]));
				oldRot[1].normalize();
			}
		}
	}

	B3dStack.erase(B3dStack.size()-1);
	return true;
}


bool CB3DMeshFileLoader::readChunkANIM()
{
#ifdef _B3D_READER_DEBUG
	core::stringc logStr;
	for ( u32 i=1; i < B3dStack.size(); ++i )
		logStr += "-";
	logStr += "read ChunkANIM";
	os::Printer::log(logStr.c_str(), ELL_DEBUG);
#endif

	s32 animFlags; //not stored\used
	s32 animFrames;//not stored\used
	f32 animFPS; //not stored\used

	B3DFile->read(&animFlags, sizeof(s32));
	B3DFile->read(&animFrames, sizeof(s32));
	readFloats(&animFPS, 1);
	if (animFPS>0.f)
		AnimatedMesh->setAnimationSpeed(animFPS);
	os::Printer::log("FPS", io::path((double)animFPS), ELL_DEBUG);

	#ifdef __BIG_ENDIAN__
		animFlags = os::Byteswap::byteswap(animFlags);
		animFrames = os::Byteswap::byteswap(animFrames);
	#endif

	B3dStack.erase(B3dStack.size()-1);
	return true;
}


bool CB3DMeshFileLoader::readChunkTEXS()
{
#ifdef _B3D_READER_DEBUG
	core::stringc logStr;
	for ( u32 i=1; i < B3dStack.size(); ++i )
		logStr += "-";
	logStr += "read ChunkTEXS";
	os::Printer::log(logStr.c_str(), ELL_DEBUG);
#endif

	while((B3dStack.getLast().startposition + B3dStack.getLast().length) > B3DFile->getPos()) //this chunk repeats
	{
		Textures.push_back(SB3dTexture());
		SB3dTexture& B3dTexture = Textures.getLast();

		readString(B3dTexture.TextureName);
		B3dTexture.TextureName.replace('\\','/');
#ifdef _B3D_READER_DEBUG
		os::Printer::log("read Texture", B3dTexture.TextureName.c_str(), ELL_DEBUG);
#endif

		B3DFile->read(&B3dTexture.Flags, sizeof(s32));
		B3DFile->read(&B3dTexture.Blend, sizeof(s32));
#ifdef __BIG_ENDIAN__
		B3dTexture.Flags = os::Byteswap::byteswap(B3dTexture.Flags);
		B3dTexture.Blend = os::Byteswap::byteswap(B3dTexture.Blend);
#endif
#ifdef _B3D_READER_DEBUG
		os::Printer::log("Flags", core::stringc(B3dTexture.Flags).c_str(), ELL_DEBUG);
		os::Printer::log("Blend", core::stringc(B3dTexture.Blend).c_str(), ELL_DEBUG);
#endif
		readFloats(&B3dTexture.Xpos, 1);
		readFloats(&B3dTexture.Ypos, 1);
		readFloats(&B3dTexture.Xscale, 1);
		readFloats(&B3dTexture.Yscale, 1);
		readFloats(&B3dTexture.Angle, 1);
	}

	B3dStack.erase(B3dStack.size()-1);

	return true;
}


bool CB3DMeshFileLoader::readChunkBRUS()
{
#ifdef _B3D_READER_DEBUG
	core::stringc logStr;
	for ( u32 i=1; i < B3dStack.size(); ++i )
		logStr += "-";
	logStr += "read ChunkBRUS";
	os::Printer::log(logStr.c_str(), ELL_DEBUG);
#endif

	u32 n_texs;
	B3DFile->read(&n_texs, sizeof(u32));
#ifdef __BIG_ENDIAN__
	n_texs = os::Byteswap::byteswap(n_texs);
#endif

	// number of texture ids read for Irrlicht
	const u32 num_textures = core::min_(n_texs, video::MATERIAL_MAX_TEXTURES);
	// number of bytes to skip (for ignored texture ids)
	const u32 n_texs_offset = (num_textures<n_texs)?(n_texs-num_textures):0;

	while((B3dStack.getLast().startposition + B3dStack.getLast().length) > B3DFile->getPos()) //this chunk repeats
	{
		// This is what blitz basic calls a brush, like a Irrlicht Material

		core::stringc name;
		readString(name);
#ifdef _B3D_READER_DEBUG
		os::Printer::log("read Material", name, ELL_DEBUG);
#endif
		Materials.push_back(SB3dMaterial());
		SB3dMaterial& B3dMaterial=Materials.getLast();

		readFloats(&B3dMaterial.red, 1);
		readFloats(&B3dMaterial.green, 1);
		readFloats(&B3dMaterial.blue, 1);
		readFloats(&B3dMaterial.alpha, 1);
		readFloats(&B3dMaterial.shininess, 1);

		B3DFile->read(&B3dMaterial.blend, sizeof(B3dMaterial.blend));
		B3DFile->read(&B3dMaterial.fx, sizeof(B3dMaterial.fx));
#ifdef __BIG_ENDIAN__
		B3dMaterial.blend = os::Byteswap::byteswap(B3dMaterial.blend);
		B3dMaterial.fx = os::Byteswap::byteswap(B3dMaterial.fx);
#endif
#ifdef _B3D_READER_DEBUG
		os::Printer::log("Blend", core::stringc(B3dMaterial.blend).c_str(), ELL_DEBUG);
		os::Printer::log("FX", core::stringc(B3dMaterial.fx).c_str(), ELL_DEBUG);
#endif

		u32 i;
		for (i=0; i<num_textures; ++i)
		{
			s32 texture_id=-1;
			B3DFile->read(&texture_id, sizeof(s32));
#ifdef __BIG_ENDIAN__
			texture_id = os::Byteswap::byteswap(texture_id);
#endif
			//--- Get pointers to the texture, based on the IDs ---
			if ((u32)texture_id < Textures.size())
			{
				B3dMaterial.Textures[i]=&Textures[texture_id];
#ifdef _B3D_READER_DEBUG
				os::Printer::log("Layer", core::stringc(i).c_str(), ELL_DEBUG);
				os::Printer::log("using texture", Textures[texture_id].TextureName.c_str(), ELL_DEBUG);
#endif
			}
			else
				B3dMaterial.Textures[i]=0;
		}
		// skip other texture ids
		for (i=0; i<n_texs_offset; ++i)
		{
			s32 texture_id=-1;
			B3DFile->read(&texture_id, sizeof(s32));
#ifdef __BIG_ENDIAN__
			texture_id = os::Byteswap::byteswap(texture_id);
#endif
			if (ShowWarning && (texture_id != -1) && (n_texs>video::MATERIAL_MAX_TEXTURES))
			{
				os::Printer::log("Too many textures used in one material", B3DFile->getFileName(), ELL_WARNING);
				ShowWarning = false;
			}
		}

		//Fixes problems when the lightmap is on the first texture:
		if (B3dMaterial.Textures[0] != 0)
		{
			if (B3dMaterial.Textures[0]->Flags & 65536) // 65536 = secondary UV
			{
				SB3dTexture *TmpTexture;
				TmpTexture = B3dMaterial.Textures[1];
				B3dMaterial.Textures[1] = B3dMaterial.Textures[0];
				B3dMaterial.Textures[0] = TmpTexture;
			}
		}

		//If a preceeding texture slot is empty move the others down:
		for (i=num_textures; i>0; --i)
		{
			for (u32 j=i-1; j<num_textures-1; ++j)
			{
				if (B3dMaterial.Textures[j+1] != 0 && B3dMaterial.Textures[j] == 0)
				{
					B3dMaterial.Textures[j] = B3dMaterial.Textures[j+1];
					B3dMaterial.Textures[j+1] = 0;
				}
			}
		}

		//------ Convert blitz flags/blend to irrlicht -------

		//Two textures:
		if (B3dMaterial.Textures[1])
		{
			if (B3dMaterial.alpha==1.f)
			{
				if (B3dMaterial.Textures[1]->Blend == 5) //(Multiply 2)
					B3dMaterial.Material.MaterialType = video::EMT_LIGHTMAP_M2;
				else
					B3dMaterial.Material.MaterialType = video::EMT_LIGHTMAP;
				B3dMaterial.Material.Lighting = false;
			}
			else
			{
				B3dMaterial.Material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
				B3dMaterial.Material.ZWriteEnable = video::EZW_OFF;
			}
		}
		else if (B3dMaterial.Textures[0]) //One texture:
		{
			// Flags & 0x1 is usual SOLID, 0x8 is mipmap (handled before)
			if (B3dMaterial.Textures[0]->Flags & 0x2) //(Alpha mapped)
			{
				B3dMaterial.Material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
				B3dMaterial.Material.ZWriteEnable = video::EZW_OFF;
			}
			else if (B3dMaterial.Textures[0]->Flags & 0x4) //(Masked)
				B3dMaterial.Material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF; // TODO: create color key texture
			else if (B3dMaterial.Textures[0]->Flags & 0x40)
				B3dMaterial.Material.MaterialType = video::EMT_SPHERE_MAP;
			else if (B3dMaterial.Textures[0]->Flags & 0x80)
				B3dMaterial.Material.MaterialType = video::EMT_SPHERE_MAP; // TODO: Should be cube map
			else if (B3dMaterial.alpha == 1.f)
				B3dMaterial.Material.MaterialType = video::EMT_SOLID;
			else
			{
				B3dMaterial.Material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
				B3dMaterial.Material.ZWriteEnable = video::EZW_OFF;
			}
		}
		else //No texture:
		{
			if (B3dMaterial.alpha == 1.f)
				B3dMaterial.Material.MaterialType = video::EMT_SOLID;
			else
			{
				B3dMaterial.Material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
				B3dMaterial.Material.ZWriteEnable = video::EZW_OFF;
			}
		}

		B3dMaterial.Material.DiffuseColor = video::SColorf(B3dMaterial.red, B3dMaterial.green, B3dMaterial.blue, B3dMaterial.alpha).toSColor();
		B3dMaterial.Material.ColorMaterial=video::ECM_NONE;

		//------ Material fx ------

		if (B3dMaterial.fx & 1) //full-bright
		{
			B3dMaterial.Material.AmbientColor = video::SColor(255, 255, 255, 255);
			B3dMaterial.Material.Lighting = false;
		}
		else
			B3dMaterial.Material.AmbientColor = B3dMaterial.Material.DiffuseColor;

		if (B3dMaterial.fx & 2) //use vertex colors instead of brush color
			B3dMaterial.Material.ColorMaterial=video::ECM_DIFFUSE_AND_AMBIENT;

		if (B3dMaterial.fx & 4) //flatshaded
			B3dMaterial.Material.GouraudShading = false;

		if (B3dMaterial.fx & 16) //disable backface culling
			B3dMaterial.Material.BackfaceCulling = false;

		if (B3dMaterial.fx & 32) //force vertex alpha-blending
		{
			B3dMaterial.Material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
			B3dMaterial.Material.ZWriteEnable = video::EZW_OFF;
		}

		B3dMaterial.Material.Shininess = B3dMaterial.shininess;
	}

	B3dStack.erase(B3dStack.size()-1);

	return true;
}


void CB3DMeshFileLoader::loadTextures(SB3dMaterial& material) const
{
	if ( getMeshTextureLoader() )
	{
		if ( SceneManager->getParameters()->existsAttribute(B3D_TEXTURE_PATH) )
			getMeshTextureLoader()->setTexturePath( SceneManager->getParameters()->getAttributeAsString(B3D_TEXTURE_PATH) );
	}

	const bool previous32BitTextureFlag = SceneManager->getVideoDriver()->getTextureCreationFlag(video::ETCF_ALWAYS_32_BIT);
	SceneManager->getVideoDriver()->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, true);

	// read texture from disk
	// note that mipmaps might be disabled by Flags & 0x8
	const bool doMipMaps = SceneManager->getVideoDriver()->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);

	for (u32 i=0; i<video::MATERIAL_MAX_TEXTURES; ++i)
	{
		SB3dTexture* B3dTexture = material.Textures[i];
		if (B3dTexture && B3dTexture->TextureName.size() && !material.Material.getTexture(i))
		{
			if (!SceneManager->getParameters()->getAttributeAsBool(B3D_LOADER_IGNORE_MIPMAP_FLAG))
				SceneManager->getVideoDriver()->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, (B3dTexture->Flags & 0x8) ? true:false);
			{
				video::ITexture* tex = getMeshTextureLoader() ? getMeshTextureLoader()->getTexture(B3dTexture->TextureName) : NULL;
				material.Material.setTexture(i, tex);
			}
			if (material.Textures[i]->Flags & 0x10) // Clamp U
				material.Material.TextureLayer[i].TextureWrapU=video::ETC_CLAMP;
			if (material.Textures[i]->Flags & 0x20) // Clamp V
				material.Material.TextureLayer[i].TextureWrapV=video::ETC_CLAMP;
			if (material.Textures[i]->Flags & 0x20) // Clamp R
				material.Material.TextureLayer[i].TextureWrapW=video::ETC_CLAMP;
		}
	}

	SceneManager->getVideoDriver()->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, doMipMaps);
	SceneManager->getVideoDriver()->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, previous32BitTextureFlag);
}


void CB3DMeshFileLoader::readString(core::stringc& newstring)
{
	newstring="";
	while (B3DFile->getPos() <= B3DFile->getSize())
	{
		c8 character;
		B3DFile->read(&character, sizeof(character));
		if (character==0)
			return;
		newstring.append(character);
	}
}


void CB3DMeshFileLoader::readFloats(f32* vec, u32 count)
{
	B3DFile->read(vec, count*sizeof(f32));
	#ifdef __BIG_ENDIAN__
	for (u32 n=0; n<count; ++n)
		vec[n] = os::Byteswap::byteswap(vec[n]);
	#endif
}

} // end namespace scene
} // end namespace irr


#endif // _IRR_COMPILE_WITH_B3D_LOADER_

