/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "tr_local.h"
#include "../qcommon/tiki_local.h"

/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the 
orientation of the bone in the base frame to the orientation in this
frame.

*/

/*
==============
R_AddAnimSurfaces
==============
*/
void R_AddAnimSurfaces( trRefEntity_t *ent ) {
	md4Header_t		*header;
	md4Surface_t	*surface;
	md4LOD_t		*lod;
	shader_t		*shader;
	int				i;

	header = (md4Header_t *) tr.currentModel->md4;
	lod = (md4LOD_t *)( (byte *)header + header->ofsLODs );

	surface = (md4Surface_t *)( (byte *)lod + lod->ofsSurfaces );
	for ( i = 0 ; i < lod->numSurfaces ; i++ ) {
		shader = R_GetShaderByHandle( surface->shaderIndex );
		R_AddDrawSurf( (void *)surface, shader, 0 /*fogNum*/, qfalse );
		surface = (md4Surface_t *)( (byte *)surface + surface->ofsEnd );
	}
}

/*
==============
RB_SurfaceAnim
==============
*/
void RB_SurfaceAnim( md4Surface_t *surface ) {
	int				i, j, k;
	float			frontlerp, backlerp;
	int				*triangles;
	int				indexes;
	int				baseIndex, baseVertex;
	int				numVerts;
	md4Vertex_t		*v;
	md4Bone_t		bones[MD4_MAX_BONES];
	md4Bone_t		*bonePtr, *bone;
	md4Header_t		*header;
	md4Frame_t		*frame;
	md4Frame_t		*oldFrame;
	int				frameSize;


	if (  backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
		frontlerp = 1;
	} else  {
		backlerp = backEnd.currentEntity->e.backlerp;
		frontlerp = 1.0f - backlerp;
	}
	header = (md4Header_t *)((byte *)surface + surface->ofsHeader);

	frameSize = (size_t)( &((md4Frame_t *)0)->bones[ header->numBones ] );

	frame = (md4Frame_t *)((byte *)header + header->ofsFrames + 
			backEnd.currentEntity->e.frame * frameSize );
	oldFrame = (md4Frame_t *)((byte *)header + header->ofsFrames + 
			backEnd.currentEntity->e.oldframe * frameSize );

	RB_CheckOverflow( surface->numVerts, surface->numTriangles * 3 );

	triangles = (int *) ((byte *)surface + surface->ofsTriangles);
	indexes = surface->numTriangles * 3;
	baseIndex = tess.numIndexes;
	baseVertex = tess.numVertexes;
	for (j = 0 ; j < indexes ; j++) {
		tess.indexes[baseIndex + j] = baseIndex + triangles[j];
	}
	tess.numIndexes += indexes;

	//
	// lerp all the needed bones
	//
	if ( !backlerp ) {
		// no lerping needed
		bonePtr = frame->bones;
	} else {
		bonePtr = bones;
		for ( i = 0 ; i < header->numBones*12 ; i++ ) {
			((float *)bonePtr)[i] = frontlerp * ((float *)frame->bones)[i]
					+ backlerp * ((float *)oldFrame->bones)[i];
		}
	}

	//
	// deform the vertexes by the lerped bones
	//
	numVerts = surface->numVerts;
	// FIXME
	// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
	// in for reference.
	//v = (md4Vertex_t *) ((byte *)surface + surface->ofsVerts + 12);
	v = (md4Vertex_t *) ((byte *)surface + surface->ofsVerts);
	for ( j = 0; j < numVerts; j++ ) {
		vec3_t	tempVert, tempNormal;
		md4Weight_t	*w;

		VectorClear( tempVert );
		VectorClear( tempNormal );
		w = v->weights;
		for ( k = 0 ; k < v->numWeights ; k++, w++ ) {
			bone = bonePtr + w->boneIndex;

			tempVert[0] += w->boneWeight * ( DotProduct( bone->matrix[0], w->offset ) + bone->matrix[0][3] );
			tempVert[1] += w->boneWeight * ( DotProduct( bone->matrix[1], w->offset ) + bone->matrix[1][3] );
			tempVert[2] += w->boneWeight * ( DotProduct( bone->matrix[2], w->offset ) + bone->matrix[2][3] );

			tempNormal[0] += w->boneWeight * DotProduct( bone->matrix[0], v->normal );
			tempNormal[1] += w->boneWeight * DotProduct( bone->matrix[1], v->normal );
			tempNormal[2] += w->boneWeight * DotProduct( bone->matrix[2], v->normal );
		}

		tess.xyz[baseVertex + j][0] = tempVert[0];
		tess.xyz[baseVertex + j][1] = tempVert[1];
		tess.xyz[baseVertex + j][2] = tempVert[2];

		tess.normal[baseVertex + j][0] = tempNormal[0];
		tess.normal[baseVertex + j][1] = tempNormal[1];
		tess.normal[baseVertex + j][2] = tempNormal[2];

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		// FIXME
		// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
		// in for reference.
		//v = (md4Vertex_t *)( ( byte * )&v->weights[v->numWeights] + 12 );
		v = (md4Vertex_t *)&v->weights[v->numWeights];
	}

	tess.numVertexes += surface->numVerts;
}


#ifdef RAVENMD4

// copied and adapted from tr_mesh.c

/*
=============
R_MDRCullModel
=============
*/

static int R_MDRCullModel( mdrHeader_t *header, trRefEntity_t *ent ) {
	vec3_t		bounds[2];
	mdrFrame_t	*oldFrame, *newFrame;
	int			i, frameSize;

	frameSize = (size_t)( &((mdrFrame_t *)0)->bones[ header->numBones ] );
	
	// compute frame pointers
	newFrame = ( mdrFrame_t * ) ( ( byte * ) header + header->ofsFrames + frameSize * ent->e.frame);
	oldFrame = ( mdrFrame_t * ) ( ( byte * ) header + header->ofsFrames + frameSize * ent->e.oldframe);

	// cull bounding sphere ONLY if this is not an upscaled entity
	if ( !ent->e.nonNormalizedAxes )
	{
		if ( ent->e.frame == ent->e.oldframe )
		{
			switch ( R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius ) )
			{
				// Ummm... yeah yeah I know we don't really have an md3 here.. but we pretend
				// we do. After all, the purpose of md4s are not that different, are they?
				
				case CULL_OUT:
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;

				case CULL_IN:
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;

				case CULL_CLIP:
					tr.pc.c_sphere_cull_md3_clip++;
					break;
			}
		}
		else
		{
			int sphereCull, sphereCullB;

			sphereCull  = R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius );
			if ( newFrame == oldFrame ) {
				sphereCullB = sphereCull;
			} else {
				sphereCullB = R_CullLocalPointAndRadius( oldFrame->localOrigin, oldFrame->radius );
			}

			if ( sphereCull == sphereCullB )
			{
				if ( sphereCull == CULL_OUT )
				{
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;
				}
				else if ( sphereCull == CULL_IN )
				{
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;
				}
				else
				{
					tr.pc.c_sphere_cull_md3_clip++;
				}
			}
		}
	}
	
	// calculate a bounding box in the current coordinate system
	for (i = 0 ; i < 3 ; i++) {
		bounds[0][i] = oldFrame->bounds[0][i] < newFrame->bounds[0][i] ? oldFrame->bounds[0][i] : newFrame->bounds[0][i];
		bounds[1][i] = oldFrame->bounds[1][i] > newFrame->bounds[1][i] ? oldFrame->bounds[1][i] : newFrame->bounds[1][i];
	}

	switch ( R_CullLocalBox( bounds ) )
	{
		case CULL_IN:
			tr.pc.c_box_cull_md3_in++;
			return CULL_IN;
		case CULL_CLIP:
			tr.pc.c_box_cull_md3_clip++;
			return CULL_CLIP;
		case CULL_OUT:
		default:
			tr.pc.c_box_cull_md3_out++;
			return CULL_OUT;
	}
}

/*
=================
R_MDRComputeFogNum

=================
*/

int R_MDRComputeFogNum( mdrHeader_t *header, trRefEntity_t *ent ) {
	int				i, j;
	fog_t			*fog;
	mdrFrame_t		*mdrFrame;
	vec3_t			localOrigin;
	int frameSize;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}
	
	frameSize = (size_t)( &((mdrFrame_t *)0)->bones[ header->numBones ] );

	// FIXME: non-normalized axis issues
	mdrFrame = ( mdrFrame_t * ) ( ( byte * ) header + header->ofsFrames + frameSize * ent->e.frame);
	VectorAdd( ent->e.origin, mdrFrame->localOrigin, localOrigin );
	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( localOrigin[j] - mdrFrame->radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( localOrigin[j] + mdrFrame->radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}


/*
==============
R_MDRAddAnimSurfaces
==============
*/

// much stuff in there is just copied from R_AddMd3Surfaces in tr_mesh.c

void R_MDRAddAnimSurfaces( trRefEntity_t *ent ) {
	mdrHeader_t		*header;
	mdrSurface_t	*surface;
	mdrLOD_t		*lod;
	shader_t		*shader;
	skin_t		*skin;
	int				i, j;
	int				lodnum = 0;
	int				fogNum = 0;
	int				cull;
	qboolean	personalModel;

	header = (mdrHeader_t *) tr.currentModel->md4;
	
	personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal;
	
	if ( ent->e.renderfx & RF_WRAP_FRAMES )
	{
		ent->e.frame %= header->numFrames;
		ent->e.oldframe %= header->numFrames;
	}	
	
	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	if ((ent->e.frame >= header->numFrames) 
		|| (ent->e.frame < 0)
		|| (ent->e.oldframe >= header->numFrames)
		|| (ent->e.oldframe < 0) )
	{
		ri.Printf( PRINT_DEVELOPER, "R_MDRAddAnimSurfaces: no such frame %d to %d for '%s'\n",
			   ent->e.oldframe, ent->e.frame, tr.currentModel->name );
		ent->e.frame = 0;
		ent->e.oldframe = 0;
	}

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_MDRCullModel (header, ent);
	if ( cull == CULL_OUT ) {
		return;
	}	

	// figure out the current LOD of the model we're rendering, and set the lod pointer respectively.
	lodnum = R_ComputeLOD(ent);
	// check whether this model has as that many LODs at all. If not, try the closest thing we got.
	if(header->numLODs <= 0)
		return;
	if(header->numLODs <= lodnum)
		lodnum = header->numLODs - 1;

	lod = (mdrLOD_t *)( (byte *)header + header->ofsLODs);
	for(i = 0; i < lodnum; i++)
	{
		lod = (mdrLOD_t *) ((byte *) lod + lod->ofsEnd);
	}
	
	// set up lighting
	if ( !personalModel || r_shadows->integer > 1 )
	{
		R_SetupEntityLighting( &tr.refdef, ent );
	}

	// fogNum?
	fogNum = R_MDRComputeFogNum( header, ent );

	surface = (mdrSurface_t *)( (byte *)lod + lod->ofsSurfaces );

	for ( i = 0 ; i < lod->numSurfaces ; i++ )
	{
		
		if(ent->e.customShader)
			shader = R_GetShaderByHandle(ent->e.customShader);
		else if(ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins)
		{
			skin = R_GetSkinByHandle(ent->e.customSkin);
			shader = tr.defaultShader;
			
			for(j = 0; j < skin->numSurfaces; j++)
			{
				if (!strcmp(skin->surfaces[j]->name, surface->name))
				{
					shader = skin->surfaces[j]->shader;
					break;
				}
			}
		}
		else if(surface->shaderIndex > 0)
			shader = R_GetShaderByHandle( surface->shaderIndex );
		else
			shader = tr.defaultShader;

		// we will add shadows even if the main object isn't visible in the view

		// stencil shadows can't do personal models unless I polyhedron clip
		if ( !personalModel
		        && r_shadows->integer == 2
			&& fogNum == 0
			&& !(ent->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) )
			&& shader->sort == SS_OPAQUE )
		{
			R_AddDrawSurf( (void *)surface, tr.shadowShader, 0, qfalse );
		}

		// projection shadows work fine with personal models
		if ( r_shadows->integer == 3
			&& fogNum == 0
			&& (ent->e.renderfx & RF_SHADOW_PLANE )
			&& shader->sort == SS_OPAQUE )
		{
			R_AddDrawSurf( (void *)surface, tr.projectionShadowShader, 0, qfalse );
		}

		if (!personalModel)
			R_AddDrawSurf( (void *)surface, shader, fogNum, qfalse );

		surface = (mdrSurface_t *)( (byte *)surface + surface->ofsEnd );
	}
}

/*
==============
RB_MDRSurfaceAnim
==============
*/
void RB_MDRSurfaceAnim( md4Surface_t *surface )
{
	int				i, j, k;
	float			frontlerp, backlerp;
	int				*triangles;
	int				indexes;
	int				baseIndex, baseVertex;
	int				numVerts;
	mdrVertex_t		*v;
	mdrHeader_t		*header;
	mdrFrame_t		*frame;
	mdrFrame_t		*oldFrame;
	mdrBone_t		bones[MD4_MAX_BONES], *bonePtr, *bone;

	int			frameSize;

	// don't lerp if lerping off, or this is the only frame, or the last frame...
	//
	if (backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame) 
	{
		backlerp	= 0;	// if backlerp is 0, lerping is off and frontlerp is never used
		frontlerp	= 1;
	} 
	else  
	{
		backlerp	= backEnd.currentEntity->e.backlerp;
		frontlerp	= 1.0f - backlerp;
	}

	header = (mdrHeader_t *)((byte *)surface + surface->ofsHeader);

	frameSize = (size_t)( &((mdrFrame_t *)0)->bones[ header->numBones ] );

	frame = (mdrFrame_t *)((byte *)header + header->ofsFrames +
		backEnd.currentEntity->e.frame * frameSize );
	oldFrame = (mdrFrame_t *)((byte *)header + header->ofsFrames +
		backEnd.currentEntity->e.oldframe * frameSize );

	RB_CheckOverflow( surface->numVerts, surface->numTriangles );

	triangles	= (int *) ((byte *)surface + surface->ofsTriangles);
	indexes		= surface->numTriangles * 3;
	baseIndex	= tess.numIndexes;
	baseVertex	= tess.numVertexes;
	
	// Set up all triangles.
	for (j = 0 ; j < indexes ; j++) 
	{
		tess.indexes[baseIndex + j] = baseVertex + triangles[j];
	}
	tess.numIndexes += indexes;

	//
	// lerp all the needed bones
	//
	if ( !backlerp ) 
	{
		// no lerping needed
		bonePtr = frame->bones;
	} 
	else 
	{
		bonePtr = bones;
		
		for ( i = 0 ; i < header->numBones*12 ; i++ ) 
		{
			((float *)bonePtr)[i] = frontlerp * ((float *)frame->bones)[i] + backlerp * ((float *)oldFrame->bones)[i];
		}
	}

	//
	// deform the vertexes by the lerped bones
	//
	numVerts = surface->numVerts;
	v = (mdrVertex_t *) ((byte *)surface + surface->ofsVerts);
	for ( j = 0; j < numVerts; j++ ) 
	{
		vec3_t	tempVert, tempNormal;
		mdrWeight_t	*w;

		VectorClear( tempVert );
		VectorClear( tempNormal );
		w = v->weights;
		for ( k = 0 ; k < v->numWeights ; k++, w++ ) 
		{
			bone = bonePtr + w->boneIndex;
			
			tempVert[0] += w->boneWeight * ( DotProduct( bone->matrix[0], w->offset ) + bone->matrix[0][3] );
			tempVert[1] += w->boneWeight * ( DotProduct( bone->matrix[1], w->offset ) + bone->matrix[1][3] );
			tempVert[2] += w->boneWeight * ( DotProduct( bone->matrix[2], w->offset ) + bone->matrix[2][3] );
			
			tempNormal[0] += w->boneWeight * DotProduct( bone->matrix[0], v->normal );
			tempNormal[1] += w->boneWeight * DotProduct( bone->matrix[1], v->normal );
			tempNormal[2] += w->boneWeight * DotProduct( bone->matrix[2], v->normal );
		}

		tess.xyz[baseVertex + j][0] = tempVert[0];
		tess.xyz[baseVertex + j][1] = tempVert[1];
		tess.xyz[baseVertex + j][2] = tempVert[2];

		tess.normal[baseVertex + j][0] = tempNormal[0];
		tess.normal[baseVertex + j][1] = tempNormal[1];
		tess.normal[baseVertex + j][2] = tempNormal[2];

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		v = (mdrVertex_t *)&v->weights[v->numWeights];
	}

	tess.numVertexes += surface->numVerts;
}


#define MC_MASK_X ((1<<(MC_BITS_X))-1)
#define MC_MASK_Y ((1<<(MC_BITS_Y))-1)
#define MC_MASK_Z ((1<<(MC_BITS_Z))-1)
#define MC_MASK_VECT ((1<<(MC_BITS_VECT))-1)

#define MC_SCALE_VECT (1.0f/(float)((1<<(MC_BITS_VECT-1))-2))

#define MC_POS_X (0)
#define MC_SHIFT_X (0)

#define MC_POS_Y ((((MC_BITS_X))/8))
#define MC_SHIFT_Y ((((MC_BITS_X)%8)))

#define MC_POS_Z ((((MC_BITS_X+MC_BITS_Y))/8))
#define MC_SHIFT_Z ((((MC_BITS_X+MC_BITS_Y)%8)))

#define MC_POS_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z))/8))
#define MC_SHIFT_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z)%8)))

#define MC_POS_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT))/8))
#define MC_SHIFT_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT)%8)))

#define MC_POS_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2))/8))
#define MC_SHIFT_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2)%8)))

#define MC_POS_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3))/8))
#define MC_SHIFT_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3)%8)))

#define MC_POS_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4))/8))
#define MC_SHIFT_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4)%8)))

#define MC_POS_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5))/8))
#define MC_SHIFT_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5)%8)))

#define MC_POS_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6))/8))
#define MC_SHIFT_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6)%8)))

#define MC_POS_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7))/8))
#define MC_SHIFT_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7)%8)))

#define MC_POS_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8))/8))
#define MC_SHIFT_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8)%8)))

void MC_UnCompress(float mat[3][4],const unsigned char * comp)
{
	int val;

	val=(int)((unsigned short *)(comp))[0];
	val-=1<<(MC_BITS_X-1);
	mat[0][3]=((float)(val))*MC_SCALE_X;

	val=(int)((unsigned short *)(comp))[1];
	val-=1<<(MC_BITS_Y-1);
	mat[1][3]=((float)(val))*MC_SCALE_Y;

	val=(int)((unsigned short *)(comp))[2];
	val-=1<<(MC_BITS_Z-1);
	mat[2][3]=((float)(val))*MC_SCALE_Z;

	val=(int)((unsigned short *)(comp))[3];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[4];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[5];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[6];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[7];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[8];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[9];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[10];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[11];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][2]=((float)(val))*MC_SCALE_VECT;
}
#endif

/*
==============
RB_SurfaceSkeleton
==============
*/
void RB_SurfaceSkeleton( skdSurface_t *sf ) {
	int i;
	tiki_t		*tiki;
	if(!backEnd.currentEntity->e.bones)
		return;
	tiki = R_GetModelByHandle(backEnd.currentEntity->e.hModel)->tiki;
	if(r_showSkeleton->integer==1) {
		vec3_t forward,right,up,angles;
		bone_t	*bones;
		tikiBonePosRot_t*bone;

		bones = backEnd.currentEntity->e.bones;
		GL_Bind(tr.whiteImage);
		qglLineWidth(3);
		qglBegin(GL_LINES);
		for (i = 0; i < tiki->numBones; i++) {
			QuatToAngles(bones[i].q,angles);
			AngleVectors(angles, forward, right, up);
			bone = (tikiBonePosRot_t*)tiki->bones[i];
			

			if(bone->parentIndex==-1) {
				// draw bone
				qglColor3f(1, 1, 1);
				qglVertex3f(0, 0, 0);
				qglVertex3fv(bones[i].p);
				
				// draw bone axis
				qglColor3f(1, 0, 0);
				qglVertex3f(0, 0, 0);
				qglVertex3fv(forward);
				qglVertex3fv(forward);
				qglVertex3fv(bones[i].p);
				
				qglColor3f(0, 1, 0);
				qglVertex3f(0, 0, 0);
				qglVertex3fv(right);
				qglVertex3fv(right);
				qglVertex3fv(bones[i].p);
				
				qglColor3f(0, 0, 1);
				qglVertex3f(0, 0, 0);
				qglVertex3fv(up);
				qglVertex3fv(up);
				qglVertex3fv(bones[i].p);
			}
			else {
					// draw bone
				qglColor3f(1, 1, 1);
				qglVertex3fv(bones[bone->parentIndex].p);
				qglVertex3fv(bones[i].p);
				
				// draw bone axis
				qglColor3f(1, 0, 0);
				qglVertex3fv(bones[bone->parentIndex].p);
				VectorAdd(bones[bone->parentIndex].p, forward, forward);
				qglVertex3fv(forward);
				qglVertex3fv(forward);
				qglVertex3fv(bones[i].p);
				
				qglColor3f(0, 1, 0);
				qglVertex3fv(bones[bone->parentIndex].p);
				VectorAdd(bones[bone->parentIndex].p, right, right);
				qglVertex3fv(right);
				qglVertex3fv(right);
				qglVertex3fv(bones[i].p);
				
				qglColor3f(0, 0, 1);
				qglVertex3fv(bones[bone->parentIndex].p);
				VectorAdd(bones[bone->parentIndex].p, up, up);
				qglVertex3fv(up);
				qglVertex3fv(up);
				qglVertex3fv(bones[i].p);
			}

		}
		qglEnd();
		qglLineWidth(1);
		return;
	}
	else if(r_showSkeleton->integer==2) {
		bone_t	*bones;
		tikiBonePosRot_t*bone;

		bones =backEnd.currentEntity->e.bones;
		GL_Bind(tr.whiteImage);
		qglLineWidth(3);
		qglBegin(GL_LINES);
		for (i = 0; i < tiki->numBones; i++) {
			bone = (tikiBonePosRot_t*)tiki->bones[i];
			if(bone->parentIndex>0)	{
				qglColor3f(1, 1, 1);
				qglVertex3fv(bones[bone->parentIndex].p);
				qglVertex3fv(bones[i].p);
			}

		}
		qglEnd();

		

		qglBegin(GL_LINES);
		for (i = 0; i < tiki->numBones; i++) {
			vec3_t up,down,front;
			VectorCopy(bones[i].p,up);
			up[1]+=5;
			VectorCopy(bones[i].p,down);
			down[1]+=5;
			VectorCopy(bones[i].p,front);
			front[0]+=5;
			bone = (tikiBonePosRot_t*)tiki->bones[i];
			qglColor3f(1, 0, 1);
			qglVertex3fv(front);
			qglVertex3fv(bones[i].p);
			qglVertex3fv(down);
			qglVertex3fv(bones[i].p);
			qglVertex3fv(up);
			qglVertex3fv(bones[i].p);
			//qglIndexi(

		}
		qglEnd();
		qglLineWidth(1);
		return;
	}
}
/*
=============
R_CullTIKI
=============
*/
static int R_CullTIKI( tiki_t *tiki, trRefEntity_t *ent ) {
	vec3_t		bounds[2];
	md3Frame_t	*oldFrame, *newFrame;
	int			i;

	//// cull bounding sphere ONLY if this is not an upscaled entity
	if ( !ent->e.nonNormalizedAxes )
	{
		switch ( R_CullPointAndRadius( ent->e.origin, ent->e.radius ) )
		{
		case CULL_OUT:
			tr.pc.c_sphere_cull_tiki_out++;
			return CULL_OUT;

		case CULL_IN:
			tr.pc.c_sphere_cull_tiki_in++;
			return CULL_IN;

		case CULL_CLIP:
			tr.pc.c_sphere_cull_tiki_clip++;
			break;
		}
	}

	switch ( R_CullLocalBox( ent->e.bounds ) )
	{
	case CULL_IN:
		tr.pc.c_box_cull_tiki_in++;
		return CULL_IN;
	case CULL_CLIP:
		tr.pc.c_box_cull_tiki_clip++;
		return CULL_CLIP;
	case CULL_OUT:
	default:
		tr.pc.c_box_cull_tiki_out++;
		return CULL_OUT;
	}
}
/*
==============
R_AddTIKISurfaces
==============
*/
void R_AddTIKISurfaces( trRefEntity_t *ent ) {
	int i;
	skdSurface_t *sf;
	tiki_t		*tiki = tr.currentModel->tiki;
	sf = tiki->surfs;
#if 0
	if(!strcmp("models/weapons/steilhandgranate.tik",tiki->name)) {	
		ri.Printf(0,"..");
	}
#endif
	if(R_CullTIKI(tiki,tr.currentEntity)==CULL_OUT) {
		return;
	}
	for(i = 0; i < tiki->numSurfaces; i++) {
		if(!(tr.currentEntity->e.surfaceBits & (1<<i))) {
			R_AddDrawSurf( (void *)sf, R_GetShaderByHandle(tiki->surfShaders[i]), 0 /*fogNum*/, qfalse );
		}
		sf = (skdSurface_t*)(((byte*)sf)+sf->ofsEnd);
	}
}

/*
=============
RB_SurfaceSKD
=============
*/
void RB_SurfaceSKD( skdSurface_t *sf ) {
	tiki_t	*tiki;
	int i,j;
	int baseIndex;
	int baseVertex;
	int numIndexes;
	int *triangles;
	skdVertex_t *vert;
	skdWeight_t *weight;

	tiki = R_GetModelByHandle(backEnd.currentEntity->e.hModel)->tiki;

	if(!backEnd.currentEntity->e.bones) {
//		ri.Printf(PRINT_DEVELOPER,"RB_SurfaceSKD: tiki %s has null bones ptr\n",tiki->name);
		return;
	}


	triangles = (int*)(((byte*)sf)+sf->ofsTriangles);
	numIndexes = sf->numTriangles*3;

	RB_CHECKOVERFLOW(sf->numVerts,numIndexes);


	baseIndex = tess.numIndexes;
	baseVertex = tess.numVertexes;
	for (i = 0 ; i < numIndexes ; i++) {
		tess.indexes[baseIndex+i] = baseVertex + triangles[i];
	}
	tess.numIndexes += numIndexes;

	vert = (skdVertex_t*)(((byte*)sf)+sf->ofsVerts);
	for(i = 0; i < sf->numVerts; i++) {
		weight = (skdWeight_t*)(((byte*)vert)+sizeof(*vert)+sizeof(skdMorph_t)*vert->numMorphs);
#if 0
		VectorCopy(weight->offset,tess.xyz[baseVertex+i]);
#else
		VectorCopy(vert->normal,tess.normal[baseVertex+i]);
		tess.texCoords[baseVertex+i][0][0] = vert->texCoords[0];
		tess.texCoords[baseVertex+i][0][1] = vert->texCoords[1];
		tess.xyz[baseVertex + i][0] = tess.xyz[baseVertex + i][1] = tess.xyz[baseVertex + i][2] = 0;
		for(j = 0; j < vert->numWeights; j++) {
			quat_t qua, que,res;
			vec3_t newnormal;
			qua[0] = weight->offset[0];
			qua[1] = weight->offset[1];
			qua[2] = weight->offset[2];
			qua[3] = 0;
			QuaternionMultiply(que,qua,backEnd.currentEntity->e.bones[weight->boneIndex].q);
			QuatInverse(backEnd.currentEntity->e.bones[weight->boneIndex].q);
			QuaternionMultiply(res,backEnd.currentEntity->e.bones[weight->boneIndex].q,que);
			QuatInverse(backEnd.currentEntity->e.bones[weight->boneIndex].q);
			tess.xyz[baseVertex + i][0] += (backEnd.currentEntity->e.bones[weight->boneIndex].p[0] + res[0])*weight->boneWeight;
			tess.xyz[baseVertex + i][1] += (backEnd.currentEntity->e.bones[weight->boneIndex].p[1] + res[1])*weight->boneWeight;
			tess.xyz[baseVertex + i][2] += (backEnd.currentEntity->e.bones[weight->boneIndex].p[2] + res[2])*weight->boneWeight;

			QuatTransformVector( backEnd.currentEntity->e.bones[weight->boneIndex].q, tess.normal[baseVertex+i], newnormal);

			VectorCopy(newnormal, tess.normal[baseVertex+i] );
			weight++;
		}
#endif
		vert = (skdVertex_t*)(((byte*)vert)+sizeof(*vert)+sizeof(skdWeight_t)*vert->numWeights+sizeof(skdMorph_t)*vert->numMorphs);
	}
#if 0
	if(backEnd.currentEntity->e.staticModelIndex) {
		mstaticModel_t *sm;
		color4ub_t *out;
		color3ub_t *in;
		int cdofs;
		skdSurface_t *sf2; 

		sm = &tr.world->staticModels[backEnd.currentEntity->e.staticModelIndex-1];

		tess.useStaticModelVertexColors = qtrue;

		cdofs = 0;
		sf2 = tiki->surfs;
		while(sf2 != sf) {
			cdofs += sf2->numVerts;
			sf2 = (skdSurface_t*)(((byte*)sf2)+sf2->ofsEnd);
		}

		in = &tr.world->smColors[sm->firstVert+cdofs];
		out = tess.vertexColors + baseVertex;
		for(i = 0; i < sf->numVerts; i++,in++,out++) {
#if 1
			(*out)[0] = (*in)[0];
			(*out)[1] = (*in)[1];
			(*out)[2] = (*in)[2];
			(*out)[3] = 255;
#elif 0
			((int*)out) = tr.identityLightByte;
#else		
			// su44: set it to something special so
			// I can debug vertex colors rendering
			(*out)[0] = 255;
			(*out)[1] = 0;
			(*out)[2] = 0;
			(*out)[3] = 255;
#endif
		}
	} //else
#endif
	//{
	//	// an attemp to fix bizarre vertex colors bug
	//	color4ub_t *col;
	//	
	//	col = &tess.vertexColors[baseVertex];
	//	for(i = 0; i < sf->numVerts; i++,col++) {
	//		(*col)[0] = 255;
	//		(*col)[1] = 255;
	//		(*col)[2] = 255;
	//		(*col)[3] = 255;
	//	}
	//}
	tess.numVertexes+= sf->numVerts;
}