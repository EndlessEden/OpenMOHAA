/*
===========================================================================
Copyright (C) 2008 Leszek Godlewski
Copyright (C) 2010-2012 su44

This file is part of OpenMoHAA source code.

OpenMoHAA source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenMoHAA source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenMoHAA source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifndef TIKI_LOCAL_H
#define TIKI_LOCAL_H

#include "tiki_public.h"
#include "qcommon.h"
#include "qfiles.h"
#include "../renderer/tr_public.h"

#define TIKI_MAX_INCLUDES			512 // 32 su44: increased for MoHAA singleplayer human models (new_generic_human.tik, etc)
#define MAX_TIKI_SURFACES			32

#define TIKI_FRAME_ENTRY	-1
#define TIKI_FRAME_EXIT		-2

typedef struct {
	int				frame;	// -1 == at entry, -2 == at exit
	char			*text;
} tikiCommand_t;

typedef struct {
	vec3_t		bounds[2];			// bounds of all surfaces of all LOD's for this frame
	float		radius;				// dist from localOrigin to corner
	vec3_t		delta;
	quat_t		*rotFKChannels; // [numRotFKChannels]
	quat_t		*rotChannels; // [numRotFKChannels]
	vec3_t		*posChannels; // [numPosFKChannels]
} tikiFrame_t;

typedef struct tikiAnim_s {
	char			alias[MAX_QPATH];
	char			fname[128]; // mohaa animation paths are sometimes longer than 64
	unsigned int	flags;
	float			crossblend;
	float			weight;
	//skcHeader_t		*motion;
	int numFrames;
	float frameTime;
	int numRotChannels;
	int numPosChannels;
	int numRotFKChannels;
	int *rotChannelNames;
	int *posChannelNames;
	int *rotFKChannelNames;
	//	quat_t *rotFKChannels; // [numFrames*numRotFKChannels]
//	quat_t *rotChannels; // [numFrames*numRotFKChannels]
//	vec3_t *posChannels; // [numFrames*numRotFKChannels]
	/*
	int *posChannelsMap;
	int *rotChannelsMap;
	int *rotFKChannelsMap;
	*/
	tikiFrame_t	*frames;
	int				numClientCmds;
	tikiCommand_t	*clientCmds;
	int				numServerCmds;
	tikiCommand_t	*serverCmds;
} tikiAnim_t;

typedef struct {
	skdJointType_t	type;
	int				parentIndex;
	short			rotIndex;
	short			posIndex;
	short			rotFKIndex;
} tikiBonePosRot_t;

typedef struct {
	skdJointType_t	type;
	int				parentIndex;
	short			rotIndex;
	vec3_t			const_offset;
} tikiBoneRotation_t;

typedef struct {
	skdJointType_t	type;
	int				parentIndex;
	short			rotFKIndex;
	float			*fk;
	vec3_t			const_offset;
} tikiBoneShoulder_t;

typedef struct {
	skdJointType_t	type;
	int				parentIndex;
	short			rotFKIndex;
	vec3_t			const_offset;
} tikiBoneElbow_t;

typedef struct {
	skdJointType_t	type;
	int				parentIndex;
	short			rotFKIndex;
	short			rotIndex;
	short			posIndex;
	vec3_t			const_offset;
} tikiBoneWrist_t;

typedef enum {
	HRTYPE_PLAIN,
	HRTYPE_ROTATEPARENT180Y,
	HRTYPE_ROTATEBOTH180Y
} hoseRotType_t;

typedef struct {
	skdJointType_t	type;
	int				parentIndex;
	int				targetIndex;
//	short			rotFKIndex;
	vec3_t			basePos;
	hoseRotType_t	hoseRotType;
	float			bendRatio;
	float			bendMax;
	float			spinRatio;
} tikiBoneHoseRot_t;

typedef struct {
	skdJointType_t	type;
	int				parentIndex;
	vec3_t			const_offset;
	float m_bone2weight; 
	int m_reference1;
	int m_reference2;
} tikiBoneAVRot_t;

typedef struct tikiInit_s {
	int numServerCmds;
	int numClientCmds;
	char **clCmds;
	char **svCmds;
} tikiInit_t;

typedef struct tiki_s {
	char			name[MAX_QPATH];

	tikiInit_t		*init;	// tiki's init section

	float			scale;
	float			lod_scale;

	int				numSurfaces;
	skdSurface_t	*surfs;
	qhandle_t			*surfShaders;
	int				numBones;

	int				*boneNames; //[numBones]
	void			**bones; // [numBones]
	//int				numHitBoxes;
	//skdHitbox_t		*hitBoxes;
	//int				numMorphTargets;
	//skdMorph_t		*morphTargets;
	int				numAnims;
	tikiAnim_t		**anims;

//	int				lodIndex[8];
	int				numIncludes;
	struct tiki_s	**includes;
	struct tiki_s	*next;
} tiki_t;

tiki_t*	TIKI_RegisterModel(const char *fname);
void TIKI_SetChannels(tiki_t *tiki, int animIndex, float animTime, float animWeight, bone_t *bones);
void TIKI_AppendFrameBoundsAndRadius( struct tiki_s *tiki, int animIndex, float animTime, float *outRadius, vec3_t outBounds[2] );
void TIKI_Animate(tiki_t *tiki, bone_t *bones);
int	TIKI_RegisterBoneName(const char *boneName);
static int	TIKI_GetAnimIndex(tiki_t *tiki, const char *animName);
void TIKI_ClearUp();
int	TIKI_RegisterBoneName(const char *boneName);
qhandle_t RE_RegisterShader( const char *name );
void TIKI_PrintBoneInfo(tiki_t *tiki, int boneNum);
int TIKI_GetBoneIndex(tiki_t *tiki, const char *boneName); // returns -1 if bone is not found
bone_t *TIKI_GetBones(int numBones);

static ID_INLINE int TIKI_GetSurfaceIndex(tiki_t *tiki, const char *surfName) {
	int i;
	skdSurface_t *surf = tiki->surfs;
	for(i = 0; i < tiki->numSurfaces; i++) {
		if(!Q_stricmp(surf->name,surfName)) {
			return i;
		}
		surf = (skdSurface_t *)( (byte *)surf + surf->ofsEnd );
	}
	return -1;
}

static ID_INLINE int TIKI_GetAnimIndex(tiki_t *tiki, const char *animAlias) {
	int i;
	tikiAnim_t **anim = tiki->anims;
	for(i = 0; i < tiki->numAnims; i++) {
		if(!Q_stricmp((*anim)->alias,animAlias)) {
			return i;
		}
		anim++;
	}
	return -1;
}

#endif // TIKI_LOCAL_H

