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

#include "tiki_local.h"
#include "../client/client.h"

#define TIKI_FILE_HASH_SIZE		1024
static	tiki_t					*hashTable[TIKI_FILE_HASH_SIZE];


char *TIKI_GetBoneNameFromIndex(int globalBoneName);
// copied over from renderer/tr_shader.c
#ifdef __GNUCC__
  #warning TODO: check if long is ok here
#endif
static long generateHashValue(const char *fname, const int size) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (size-1);
	return hash;
}

qhandle_t TIKI_RE_RegisterShader( const char *name ) {
#ifdef DEDICATED
	return 0;
#else
	return re.RegisterShader( name );
#endif
}

// an utility for debuging
void TIKI_PrintBoneInfo(tiki_t *tiki, int boneNum) {
	tikiBonePosRot_t	*posrot, *bone;
	tikiBoneRotation_t	*rot;
	tikiBoneShoulder_t	*should;
	tikiBoneElbow_t		*elbow;
	tikiBoneWrist_t		*wrist;
	tikiBoneHoseRot_t	*hose;
	tikiBoneAVRot_t		*av;

	bone = (tikiBonePosRot_t*) tiki->bones[boneNum];

	switch(*(int*)tiki->bones[boneNum])
	{	
		case JT_ROTATION:
		{
			rot = (tikiBoneRotation_t*)tiki->bones[boneNum];
			Com_Printf("%s, parent %i, type 0 - JT_ROTATION, const offset %f %f %f\n",
				TIKI_GetBoneNameFromIndex(tiki->boneNames[boneNum]),rot->parentIndex,rot->const_offset[0],
				rot->const_offset[1],rot->const_offset[2]);
		}
		break;
		case JT_POSROT_SKC:
		{
			posrot = (tikiBonePosRot_t*)tiki->bones[boneNum];
			Com_Printf("%s, parent %i, type 1 - posrot\n",
				TIKI_GetBoneNameFromIndex(tiki->boneNames[boneNum]),posrot->parentIndex);

		}
		break;	
		case JT_SHOULDER:
		{
			should = (tikiBoneShoulder_t*)tiki->bones[boneNum];
			Com_Printf("%s, parent %i, type 2 - JT_SHOULDER, const offset %f %f %f\n",
				TIKI_GetBoneNameFromIndex(tiki->boneNames[boneNum]),should->parentIndex,should->const_offset[0],
				should->const_offset[1],should->const_offset[2]);

		}
		break;
		case JT_ELBOW:
		{
			elbow = (tikiBoneElbow_t*)tiki->bones[boneNum];
			Com_Printf("%s, parent %i, type 3 - JT_ELBOW, const offset %f %f %f\n",
				TIKI_GetBoneNameFromIndex(tiki->boneNames[boneNum]),elbow->parentIndex,elbow->const_offset[0],
				elbow->const_offset[1],elbow->const_offset[2]);

		}
		break;
		case JT_WRIST:
		{
			wrist = (tikiBoneWrist_t*) tiki->bones[boneNum];
			Com_Printf("%s, parent %i, type 4 - JT_WRIST, const offset %f %f %f\n",
				TIKI_GetBoneNameFromIndex(tiki->boneNames[boneNum]),wrist->parentIndex,wrist->const_offset[0],
				wrist->const_offset[1],wrist->const_offset[2]);


		}
		break;
		case JT_HOSEROT:
		{
			hose = (tikiBoneHoseRot_t*)tiki->bones[boneNum];
			Com_Printf("%s, parent %i, type 5 - JT_HOSEROT, basePos %f %f %f, hoseType %i, "
				"bendMax %f, bendRatio %f, spinRadio %f, target %i\n",
				TIKI_GetBoneNameFromIndex(tiki->boneNames[boneNum]),hose->parentIndex,hose->basePos[0],
				hose->basePos[1],hose->basePos[2],hose->hoseRotType,hose->bendMax,hose->bendRatio,
				hose->spinRatio,hose->targetIndex);

		}
		break;
		case JT_AVROT:
		{
			av = (tikiBoneAVRot_t*) tiki->bones[boneNum];
			Com_Printf("%s, parent %i, type 6 - JT_AVROT, const offset %f %f %f, ref1 %i, ref2 %i, bone2weight %f\n",
				TIKI_GetBoneNameFromIndex(tiki->boneNames[boneNum]),av->parentIndex,av->const_offset[0],
				av->const_offset[1],av->const_offset[2],av->m_reference1,av->m_reference2,av->m_bone2weight);
		}
		break;
		default:
			Com_Error(ERR_DROP,"TIKI_PrintBoneInfo: unsupported bone type %d boneNum",*((int**)tiki->bones)[boneNum]);
		break;
	}
}

// bones... called every frame from cgame, animated by CG_ModelAnim, then passed to renderer with refEntities
#define MAX_GLOBAL_BONES 4096
static bone_t globalBones[MAX_GLOBAL_BONES];
static bone_t *currBone = globalBones;
bone_t *TIKI_GetBones(int numBones) {
	int i;
	bone_t *tmp;
	if(currBone+numBones >= globalBones+MAX_GLOBAL_BONES) {
		Com_Error(ERR_DROP,"TIKI_GetBones: overflowed. Increase MAX_GLOBAL_BONES (%i) and recompile!\n",MAX_GLOBAL_BONES);
		return 0;
	}
	tmp = currBone;
	currBone+=numBones;
	for(i = 0; i < numBones; i++) {
		VectorSet(tmp[i].p,0,0,0);
		VectorSet(tmp[i].q,0,0,0);
		tmp[i].q[3] = 1;
	}
	return tmp;
}
void TIKI_ResetBones() {//called every frame before CG_AddPackEntities
	currBone = globalBones;
}
/*
===============================
ANIMATION CODE
===============================
*/
int TIKI_FindPosChannel(tikiAnim_t *anim, int globalBoneName) {
	int i;
	for(i = 0; i < anim->numPosChannels; i++) {
		if(anim->posChannelNames[i] == globalBoneName)
			return i;
	}
	return -1;
}
int TIKI_FindRotChannel(tikiAnim_t *anim, int globalBoneName) {
	int i;
	for(i = 0; i < anim->numRotChannels; i++) {
		if(anim->rotChannelNames[i] == globalBoneName)
			return i;
	}
	return -1;
}
qboolean TIKI_FindAndCopyPosChannel(tikiAnim_t *anim, int globalBoneName, tikiFrame_t *frame, vec3_t out) {
	int i;
	for(i = 0; i < anim->numPosChannels; i++) {
		if(anim->posChannelNames[i] == globalBoneName) {
			VectorCopy(frame->posChannels[i],out);
			return qtrue;
		}
	}
	return qfalse;
}
qboolean TIKI_FindAndCopyRotChannel(tikiAnim_t *anim, int globalBoneName, tikiFrame_t *frame, quat_t out) {
	int i;
	for(i = 0; i < anim->numRotChannels; i++) {
		if(anim->rotChannelNames[i] == globalBoneName) {
			VectorCopy(frame->rotChannels[i],out);
			out[3] = frame->rotChannels[i][3];
			return qtrue;
		}
	}
	return qfalse;
}
qboolean TIKI_FindAndCopyRotFKChannel(tikiAnim_t *anim, int globalBoneName, tikiFrame_t *frame, quat_t out) {
	int i;
	for(i = 0; i < anim->numRotFKChannels; i++) {
		if(anim->rotFKChannelNames[i] == globalBoneName) {
			VectorCopy(frame->rotFKChannels[i],out);
			out[3] = frame->rotFKChannels[i][3];
			return qtrue;
		}
	}
	return qfalse;
}
void TIKI_SetChannels_internal(tiki_t *tiki, bone_t *bones, tikiAnim_t *anim, tikiFrame_t *frame) {
	int i;
	int **ptr = (int**)tiki->bones;
	for(i = 0; i < tiki->numBones; i++)	{
		switch(**ptr) {
			case JT_ROTATION:
				TIKI_FindAndCopyRotChannel(anim,tiki->boneNames[i],frame,bones->q);
			break;
			case JT_POSROT_SKC:
				//rot = TIKI_FindRotChannel(anim,tiki->boneNames[i]);
				//pos = TIKI_FindRotChannel(anim,tiki->boneNames[i]);
				TIKI_FindAndCopyPosChannel(anim,tiki->boneNames[i],frame,bones->p);
				TIKI_FindAndCopyRotChannel(anim,tiki->boneNames[i],frame,bones->q);
			break;
			case JT_SHOULDER:		
				TIKI_FindAndCopyRotFKChannel(anim,tiki->boneNames[i],frame,bones->q);
			break;
			case JT_ELBOW:		
				TIKI_FindAndCopyRotFKChannel(anim,tiki->boneNames[i],frame,bones->q);
			break;
			case JT_WRIST:
				TIKI_FindAndCopyPosChannel(anim,tiki->boneNames[i],frame,bones->p);
				TIKI_FindAndCopyRotFKChannel(anim,tiki->boneNames[i],frame,bones->q);
				//TIKI_FindAndCopyRotChannel(anim,tiki->boneNames[i],frame,bones->q);
				break;
			case JT_HOSEROT:
			case JT_AVROT:
				break;
			default:
				Com_Error(ERR_DROP,"Unknwon wbonetye %i\n",**ptr);
				break;
		}
		ptr++;
		bones++;
	}
}

void TIKI_SetChannels_interpolate(tiki_t *tiki, bone_t *bones, tikiAnim_t *anim, tikiFrame_t *frame, tikiFrame_t *nextFrame, float f) {
	int i;
	quat_t q1,q2;
	vec3_t v1, v2;
	int **ptr;

	ptr = (int**)tiki->bones;
	for(i = 0; i < tiki->numBones; i++)	{
		switch(**ptr) {
			case JT_ROTATION:
				if(TIKI_FindAndCopyRotChannel(anim,tiki->boneNames[i],frame,q1)) {
					TIKI_FindAndCopyRotChannel(anim,tiki->boneNames[i],nextFrame,q2);
					QuatSlerp(q1,q2,f,bones->q);
				}
			break;
			case JT_POSROT_SKC:
				if(TIKI_FindAndCopyPosChannel(anim,tiki->boneNames[i],frame,v1)) {
					TIKI_FindAndCopyPosChannel(anim,tiki->boneNames[i],nextFrame,v2);
					VectorLerp(v1,v2,f,bones->p);
				}
				if(TIKI_FindAndCopyRotChannel(anim,tiki->boneNames[i],frame,q1)) {
					TIKI_FindAndCopyRotChannel(anim,tiki->boneNames[i],nextFrame,q2);
					QuatSlerp(q1,q2,f,bones->q);
				}
			break;
			case JT_SHOULDER:		
				if(TIKI_FindAndCopyRotFKChannel(anim,tiki->boneNames[i],frame,q1)) {
					TIKI_FindAndCopyRotFKChannel(anim,tiki->boneNames[i],nextFrame,q2);
					QuatSlerp(q1,q2,f,bones->q);	
				}
				break;
			case JT_ELBOW:		
				if(TIKI_FindAndCopyRotFKChannel(anim,tiki->boneNames[i],frame,q1)) {
					TIKI_FindAndCopyRotFKChannel(anim,tiki->boneNames[i],nextFrame,q2);
					QuatSlerp(q1,q2,f,bones->q);
				}
			break;
			case JT_WRIST:
				if(TIKI_FindAndCopyPosChannel(anim,tiki->boneNames[i],frame,v1)) {
					TIKI_FindAndCopyPosChannel(anim,tiki->boneNames[i],nextFrame,v2);
					VectorLerp(v1,v2,f,bones->p);
				}

				if(TIKI_FindAndCopyRotFKChannel(anim,tiki->boneNames[i],frame,q1)) {
					TIKI_FindAndCopyRotFKChannel(anim,tiki->boneNames[i],nextFrame,q2);
					QuatSlerp(q1,q2,f,bones->q);
				}
				//TIKI_FindAndCopyRotChannel(anim,tiki->boneNames[i],frame,bones->q);
				break;
			case JT_HOSEROT:
			case JT_AVROT:
				break;
			default:
				Com_Error(ERR_DROP,"Unknwon wbonetye %i\n",**ptr);
				break;
		}
		ptr++;
		bones++;
	}
}
float TIKI_ClampAnimTime( tikiAnim_t *anim, float time ) {
	float totalTime;
	totalTime = anim->frameTime * anim->numFrames;
	while(time > totalTime) {
		time -= totalTime;
	}
	return time;
}
static void TIKI_AppendFrameBoundsAndRadiusSingle( tikiFrame_t *frame, float *outRadius, vec3_t outBounds[2]) {
	BoundsAdd(outBounds[0],outBounds[1],frame->bounds[0],frame->bounds[1]);
	if(frame->radius > *outRadius) {
		*outRadius = frame->radius;
	}
}
void TIKI_AppendFrameBoundsAndRadius( struct tiki_s *tiki, int animIndex, float animTime, float *outRadius, vec3_t outBounds[2] ) {
	tikiAnim_t *anim;
	tikiFrame_t *frame;
	int i;

	if(tiki->numAnims <= animIndex) {
		Com_Printf("TIKI_AppendFrameBoundsAndRadius: animIndex %i out of range %i\n",animIndex,tiki->numAnims);
		return;
	}
	anim = tiki->anims[animIndex];
	if(anim->numFrames == 1) {
		TIKI_AppendFrameBoundsAndRadiusSingle(anim->frames,outRadius,outBounds);
		return;
	}
#if 1
	animTime = TIKI_ClampAnimTime(anim,animTime);
#endif
	i = 1;
	frame = anim->frames;
	while(animTime > anim->frameTime) {
		animTime-=anim->frameTime;
		frame++;
		i++;
		if(i == anim->numFrames) {
			TIKI_AppendFrameBoundsAndRadiusSingle(frame,outRadius,outBounds);
			return;
		}
	}
	TIKI_AppendFrameBoundsAndRadiusSingle(frame,outRadius,outBounds);
	TIKI_AppendFrameBoundsAndRadiusSingle(frame+1,outRadius,outBounds);
}
void TIKI_SetChannels(tiki_t *tiki, int animIndex, float animTime, float animWeight, bone_t *bones) {
	int i;
	skdJointType_t **ptr;
	tikiAnim_t *anim;
	tikiFrame_t *frame;

	if(tiki->numAnims <= animIndex) {
		Com_Printf("TIKI_SetChannels: animIndex %i out of range %i\n",animIndex,tiki->numAnims);
		return;
	}
	ptr = (skdJointType_t **)tiki->bones;
	anim = tiki->anims[animIndex];
	frame = anim->frames;
	if(anim->numFrames == 1) {
		//do not interpolate
		TIKI_SetChannels_internal(tiki,bones,anim,frame);
		return;
	}
	else {
#if 1
		animTime = TIKI_ClampAnimTime(anim,animTime);
#endif

		i = 1;
		while(animTime > anim->frameTime) {
			animTime-=anim->frameTime;
			frame++;
			i++;
			if(i == anim->numFrames) {
				TIKI_SetChannels_internal(tiki,bones,anim,frame); // last frame, do not interpolate
				return;
			}
		}
	}
	//interpolate between two frames
	TIKI_SetChannels_interpolate(tiki,bones,anim,frame,frame+1,animTime/anim->frameTime);
}

void TIKI_Animate(tiki_t *tiki, bone_t *bones)
{
	int i;
	tikiBonePosRot_t	*posrot, *bone;
	tikiBoneRotation_t	*rot;
	tikiBoneShoulder_t	*should;
	tikiBoneElbow_t		*elbow;
	tikiBoneWrist_t		*wrist;
	tikiBoneHoseRot_t	*hose;
	tikiBoneAVRot_t		*av;
	for(i = 0; i < tiki->numBones;i++) {
		bone = (tikiBonePosRot_t*) tiki->bones[i];
	
		if(bone->parentIndex==-1) {
			QuatInverse(bones[i].q);
		}
		else {

			switch(*(int*)tiki->bones[i])
			{	
				case JT_ROTATION:
				{
					quat_t	pos,res,temp;
					rot = (tikiBoneRotation_t*)tiki->bones[i];
					//const_ofs instead of channel p
					VectorCopy(rot->const_offset,pos);
					pos[3]=0;
					QuaternionMultiply(temp,pos,bones[rot->parentIndex].q);
					QuatInverse(bones[rot->parentIndex].q);
					QuaternionMultiply(res,bones[rot->parentIndex].q,temp);
					QuatInverse(bones[rot->parentIndex].q);
					bones[i].p[0] = res[0] + bones[rot->parentIndex].p[0];
					bones[i].p[1] = res[1] + bones[rot->parentIndex].p[1];
					bones[i].p[2] = res[2] + bones[rot->parentIndex].p[2];
					QuatCopy(bones[i].q,temp);
					QuatInverse(temp);
					QuaternionMultiply(bones[i].q,temp,bones[rot->parentIndex].q);
					QuatNormalize(bones[i].q);

				}
				break;
				case JT_POSROT_SKC:
				{
					quat_t	pos,res,temp;
					posrot = (tikiBonePosRot_t*)tiki->bones[i];
					VectorCopy(bones[i].p,pos);
					pos[3]=0;
					QuaternionMultiply(temp,pos,bones[posrot->parentIndex].q);
					QuatInverse(bones[posrot->parentIndex].q);
					QuaternionMultiply(res,bones[posrot->parentIndex].q,temp);
					QuatInverse(bones[posrot->parentIndex].q);
					bones[i].p[0] = res[0] +bones[posrot->parentIndex].p[0];
					bones[i].p[1] = res[1] +bones[posrot->parentIndex].p[1];
					bones[i].p[2] = res[2] +bones[posrot->parentIndex].p[2];
					QuatCopy(bones[i].q,temp);
					QuatInverse(temp);
					QuaternionMultiply(bones[i].q,temp,bones[posrot->parentIndex].q);
					QuatNormalize(bones[i].q);

				}
				break;	
				case JT_SHOULDER:
				{
					//Com_Printf("should %i",tiki->name);
					quat_t	pos,res,temp;
					should = (tikiBoneShoulder_t*)tiki->bones[i];
					//const_ofs instead of channel p
					VectorCopy(should->const_offset,pos);

					//we're using its rotfk channel instead of rot
					pos[3]=0;
					QuaternionMultiply(temp,pos,bones[should->parentIndex].q);
					QuatInverse(bones[should->parentIndex].q);
					QuaternionMultiply(res,bones[should->parentIndex].q,temp);
					QuatInverse(bones[should->parentIndex].q);
					bones[i].p[0] = res[0] +bones[should->parentIndex].p[0];
					bones[i].p[1] = res[1] +bones[should->parentIndex].p[1];
					bones[i].p[2] = res[2] +bones[should->parentIndex].p[2];
					QuatInverse(bones[i].q);
					QuatCopy(bones[i].q,temp);
					QuatInverse(bones[i].q);
					QuaternionMultiply(bones[i].q,temp,bones[should->parentIndex].q);
					QuatNormalize(bones[i].q);

				}
				break;
				case JT_ELBOW:
				{
					quat_t	pos,res,temp;


					elbow = (tikiBoneElbow_t*)tiki->bones[i];
					//Com_Printf("elb %i",tiki->name);

					//const_ofs instead of channel p
					VectorCopy(elbow->const_offset,pos);

					//like in ikshoulder,
					//we're using rotfk channel instead of rot ???
					pos[3]=0;
					QuaternionMultiply(temp,pos,bones[elbow->parentIndex].q);
					QuatInverse(bones[elbow->parentIndex].q);
					QuaternionMultiply(res,bones[elbow->parentIndex].q,temp);
					QuatInverse(bones[elbow->parentIndex].q);
					bones[i].p[0] = res[0] +bones[elbow->parentIndex].p[0];
					bones[i].p[1] = res[1] +bones[elbow->parentIndex].p[1];
					bones[i].p[2] = res[2] +bones[elbow->parentIndex].p[2];
					QuatInverse(bones[i].q);
					QuatCopy(bones[i].q,temp);
					QuatInverse(bones[i].q);
					QuaternionMultiply(bones[i].q,temp,bones[elbow->parentIndex].q);
					QuatNormalize(bones[i].q);
				}
				break;
				case JT_WRIST:
				{
					quat_t	pos,res,temp;
					wrist = (tikiBoneWrist_t*) tiki->bones[i];

					//const_ofs instead of channel p
					VectorCopy(wrist->const_offset,pos);
					//Com_Printf("wrist  %i",tiki->name);
					//like in ikshoulder, nad elbow
					//we're using its rotfk channel instead of rot
					pos[3]=0;
					QuaternionMultiply(temp,pos,bones[wrist->parentIndex].q);
					QuatInverse(bones[wrist->parentIndex].q);
					QuaternionMultiply(res,bones[wrist->parentIndex].q,temp);
					QuatInverse(bones[wrist->parentIndex].q);
					bones[i].p[0] = res[0] +bones[wrist->parentIndex].p[0];
					bones[i].p[1] = res[1] +bones[wrist->parentIndex].p[1];
					bones[i].p[2] = res[2] +bones[wrist->parentIndex].p[2];
					QuatInverse(bones[i].q);
					QuatCopy(bones[i].q,temp);
					QuatInverse(bones[i].q);
					QuaternionMultiply(bones[i].q,temp,bones[wrist->parentIndex].q);
					QuatNormalize(bones[i].q);
				}
				break;
				case JT_HOSEROT:
				{
					cvar_t *su44 = Cvar_Get("su44","0",CVAR_ARCHIVE);
					hose = (tikiBoneHoseRot_t*)tiki->bones[i];
			//		Com_Printf("hoserot const ofs %f %f %f for ",hose->const_offset[0],hose->const_offset[1],hose->const_offset[2]);

					if(su44->integer && hose->hoseRotType == HRTYPE_ROTATEBOTH180Y) {	
						// it fixes human model legs but screws viewmodelanims
						quat_t	pos,res,temp;
						quat_t hq;
			
						// su44: try to construct hose quat
						// (bendMax is in degrees, 180, etc ?);
						// We need to rotate arount Y axis
						QuatSet(hq,0,hose->bendMax/180,0,0);
						QuatCalcW(hq); // compute w componten

						// su44: use hose baseposs
						VectorCopy(hose->basePos,pos);

						pos[3]=0;
						QuaternionMultiply(temp,pos,bones[hose->parentIndex].q);
						QuatInverse(bones[hose->parentIndex].q);
						QuaternionMultiply(res,bones[hose->parentIndex].q,temp);
						QuatInverse(bones[hose->parentIndex].q);
						bones[i].p[0] = res[0] + bones[hose->parentIndex].p[0];
						bones[i].p[1] = res[1] + bones[hose->parentIndex].p[1];
						bones[i].p[2] = res[2] + bones[hose->parentIndex].p[2];
						// here we're using new quat that he have just calculated instead of 
						// one provided by "TIKI_SetChannels" (which is infact always identity
						// because there are no channel data for hoserot bones)
						QuatInverse(hq);
						QuatCopy(hq,temp);
						QuatInverse(hq);
						QuaternionMultiply(bones[i].q,temp,bones[hose->parentIndex].q);
						QuatNormalize(bones[i].q);

					} else {
						QuatCopy(bones[hose->targetIndex].q,bones[i].q);
						VectorCopy(bones[hose->targetIndex].p,bones[i].p); 
					}
				}
				break;
				case JT_AVROT:
				{
					quat_t	pos,res,temp;
					av = (tikiBoneAVRot_t*) tiki->bones[i];

					QuatSlerp(bones[av->m_reference1].q,bones[av->m_reference2].q,av->m_bone2weight,bones[i].q);
					/*
					bones[i].p[0] = bones[av->m_reference1].p[0] + av->m_bone2weight*(bones[av->m_reference2].p[0] - bones[av->m_reference1].p[0]);
					bones[i].p[1] = bones[av->m_reference1].p[1] + av->m_bone2weight*(bones[av->m_reference2].p[1] - bones[av->m_reference1].p[1]);
					bones[i].p[2] = bones[av->m_reference1].p[2] + av->m_bone2weight*(bones[av->m_reference2].p[2] - bones[av->m_reference1].p[2]);
					*/
					VectorCopy(av->const_offset,pos);
					pos[3]=0;
					QuaternionMultiply(temp,pos,bones[av->parentIndex].q);
					QuatInverse(bones[av->parentIndex].q);
					QuaternionMultiply(res,bones[av->parentIndex].q,temp);
					QuatInverse(bones[av->parentIndex].q);
					bones[i].p[0] = res[0] +bones[av->parentIndex].p[0];
					bones[i].p[1] = res[1] +bones[av->parentIndex].p[1];
					bones[i].p[2] = res[2] +bones[av->parentIndex].p[2];
				}
				break;
				default:
					Com_Error(ERR_DROP,"TIKI_Animate: unsupported bone type %i",*((int**)tiki->bones)[i]);
				break;
			}
		}
	}
#if 0
	if(tiki->numAnims>456) {
		for(i = 0; i < tiki->numBones; i++)	{
			Com_DPrintf("bone %i of %i (%s) - %f %f %f\n",i,tiki->numBones,TIKI_GetBoneNameFromIndex(tiki->boneNames[i]),bones[i].p[0],bones[i].p[1],bones[i].p[2]);
		}
	}
#endif
}

/*
===============================
SK* FILES LOADING
===============================
*/
typedef struct boneName_s
{
	char *name;
	struct boneName_s *next;
} boneName_t;
#define TIKI_BONE_HASH_SIZE 128
boneName_t globalBoneNames[4096];
boneName_t *freeBoneName = globalBoneNames;
boneName_t *boneHashTable[TIKI_BONE_HASH_SIZE];
int	TIKI_RegisterBoneName(const char *boneName) {
	boneName_t	*bone;
	int		hash = generateHashValue(boneName, TIKI_BONE_HASH_SIZE);

	for (bone = boneHashTable[hash]; bone; bone = bone->next) {
		if (!Q_stricmp(bone->name, boneName))
			return bone - globalBoneNames;
	}
	freeBoneName++;
	bone = freeBoneName;
	if (boneHashTable[hash]) {
		bone->next = boneHashTable[hash];
	}
	boneHashTable[hash] = bone;
	bone->name = Hunk_Alloc(strlen(boneName)+1,h_low);
	strcpy(bone->name,boneName);
	return bone - globalBoneNames;
}
char *TIKI_GetBoneNameFromIndex(int globalBoneName) {
	return globalBoneNames[globalBoneName].name;
}
int TIKI_GetLocalBoneIndex2(tiki_t *out, int globalBoneName) {
	int b;
	for(b = 0; b < out->numBones; b++) {
		if(out->boneNames[b]==globalBoneName)
			return b;
	}
	Com_Error(ERR_DROP,"TIKI_GetLocalBoneIndex failed for tiki %s, boneName %i \n",out->name,globalBoneName);
	return -1;
}
int TIKI_GetLocalBoneIndex(tiki_t *out, char *boneName) {
	int globalIndex;
	if(!strcmp(boneName,"worldbone")) {
		return -1;
	}
	globalIndex = TIKI_RegisterBoneName(boneName);
	return TIKI_GetLocalBoneIndex2(out,globalIndex);
}
qboolean TIKI_HasBone(tiki_t *out, int globalBoneName) {
	int i;
	for(i = 0; i < out->numBones; i++) {
		if(out->boneNames[i] == globalBoneName) {
			return qtrue;
		}
	}
	return qfalse;
}
int TIKI_GetBoneIndex(tiki_t *tiki, const char *boneName) {
	int i;
	int boneNameIndex = TIKI_RegisterBoneName(boneName);
	for(i = 0; i < tiki->numBones; i++) {
		if(tiki->boneNames[i] == boneNameIndex) {
			return i;
		}
	}
	return -1;
}

// what a mess!
// but at least now it works...
void TIKI_MergeSKD(tiki_t *out, char *fname) {
	int				filesize;
	char			*buf = NULL;
	skdHeader_t		*header;
	skdBone_t		*bone;
	skdSurface_t	*surf, *outSurfs;
	float			*ptr;
	char			*bytePtr;
	int				i,j,k;
	skdVertex_t		*vert;
	skdWeight_t		*weight;
	tikiBonePosRot_t*posrot;
	tikiBoneRotation_t*rot;
	tikiBoneShoulder_t	*should;
	tikiBoneElbow_t		*elbow;
	tikiBoneWrist_t		*wrist;
	tikiBoneHoseRot_t	*hose;
	tikiBoneAVRot_t		*av;
	int					outSurfacesSize;
	int					numAddBones;
	int					addBonesIndexes[128];
	byte				notInBoneList[128];
	int					headerBoneNames[128];
	int *newBoneNames;
	intptr_t *newBoneList;

	//Com_Printf("TIKI_MergeSKD for %s, skd name  %s\n", out->name, fname);
	filesize = FS_ReadFile(fname, (void **)&buf);
	if(!buf) {
		Com_Printf ("TIKI_MergeSKD: couldn't load %s\n", fname);
		return;
	}
	header = (skdHeader_t *)buf;
	if (header->version != SKD_VERSION) {
		Com_Printf( "TIKI_MergeSKD: %s has wrong version (%i should be %i)\n",
				 fname, header->version, SKD_VERSION);
		return;
	}
	if (header->ident != SKD_IDENT) {
		Com_Printf( "TIKI_MergeSKD: %s has wrong ident (%d should be %d)\n",
				 fname, header->ident, SKD_IDENT);
		return;
	}

	numAddBones = 0;
	//first check do we have to realloc the bones?
	bone = (skdBone_t *) ( (byte *)header + header->ofsBones );
	for ( i = 0 ; i < header->numBones ; i++) {
		headerBoneNames[i] = TIKI_RegisterBoneName(bone->name);
		notInBoneList[i] = TIKI_HasBone(out,headerBoneNames[i]);
		if(notInBoneList[i]==qfalse) {
			addBonesIndexes[numAddBones] = headerBoneNames[i];
			numAddBones++;
		}
		bone = (skdBone_t *)( (byte *)bone + bone->ofsEnd );
	}
	if(numAddBones) {
		newBoneNames = Z_TagMalloc((numAddBones + out->numBones) * sizeof(int),TAG_TIKI);
		memcpy(newBoneNames,out->boneNames,out->numBones * sizeof(int));
		memcpy(&newBoneNames[out->numBones],addBonesIndexes,numAddBones * sizeof(int));
		Z_Free(out->boneNames);
		out->boneNames = newBoneNames;

		newBoneList = Z_TagMalloc((out->numBones + numAddBones) * sizeof(intptr_t),TAG_TIKI);
		memcpy(newBoneList,out->bones,out->numBones * sizeof(intptr_t));
		Z_Free(out->bones);
		out->bones = (void**)newBoneList;
		newBoneList+=out->numBones;
		out->numBones+=numAddBones;
		
		bone = (skdBone_t *) ( (byte *)header + header->ofsBones );
		for ( i = 0 ; i < header->numBones ; i++) {
			if(notInBoneList[i]==qfalse) { //bone i is not present in out->bones array, we have to load it from this skd file
				switch(bone->jointType)	{
					case JT_ROTATION:
					{
						*newBoneList = (intptr_t)Hunk_Alloc(sizeof(tikiBoneRotation_t),h_low);
						rot = (tikiBoneRotation_t*)*newBoneList;
						rot->type=bone->jointType;
						rot->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
						ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
						VectorScale(ptr,out->scale,rot->const_offset);
					}
					break;
					case JT_POSROT_SKC:
					{
						*newBoneList = (intptr_t)Hunk_Alloc(sizeof(tikiBonePosRot_t),h_low);
						posrot = (tikiBonePosRot_t*)*newBoneList;
						posrot->type=bone->jointType;
						posrot->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
					}
					break;
					case JT_SHOULDER:
					{
						*newBoneList = (intptr_t)Hunk_Alloc(sizeof(tikiBoneShoulder_t),h_low);
						should = (tikiBoneShoulder_t*)*newBoneList;
						should->type=bone->jointType;
						should->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
						ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
						ptr+=(4);
						VectorScale(ptr,out->scale,should->const_offset);
					}
					break;
					case JT_ELBOW:
					{
						*newBoneList = (intptr_t)Hunk_Alloc(sizeof(tikiBoneElbow_t),h_low);
						elbow = (tikiBoneElbow_t*)*newBoneList;
						elbow->type=bone->jointType;
						elbow->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
						ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
						VectorScale(ptr,out->scale,ptr);
						VectorCopy(ptr,elbow->const_offset);
					}
					break;
					case JT_WRIST:
					{
						*newBoneList = (intptr_t)Hunk_Alloc(sizeof(tikiBoneWrist_t),h_low);
						wrist = (tikiBoneWrist_t*)*newBoneList;
						wrist->type=bone->jointType;
						wrist->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
						ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
						VectorScale(ptr,out->scale,wrist->const_offset);
					}
					break;
					case JT_HOSEROT:
					{
						*newBoneList = (intptr_t)Hunk_Alloc(sizeof(tikiBoneHoseRot_t),h_low);
						hose = (tikiBoneHoseRot_t*)*newBoneList;
						hose->type=bone->jointType;
						hose->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
						ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
//						Com_Printf("HOSEROT bendRatio %f \n",*ptr);
						hose->bendRatio = *ptr;
						ptr++;
//						Com_Printf("HOSEROT bendMax %f \n",*ptr);
						hose->bendMax = *ptr;
						ptr++;
//						Com_Printf("HOSEROT spinRatio %f \n",*ptr);
						hose->spinRatio = *ptr;
						ptr++;
						VectorScale(ptr,out->scale,hose->basePos);
						ptr+=3;
						//Com_Printf("%f %f %f \n",ptr[0],ptr[1],ptr[2]);
						ptr+=3; // skip 1.f 1.f 1.f
//						Com_Printf("HOSEROT hoseRotType %i \n",*(int*)ptr);
						hose->hoseRotType = *(hoseRotType_t*)ptr;
						ptr++;
						//Com_Printf("\n HOSEref <%s>\n", (char *)ptr);
						hose->targetIndex = TIKI_GetLocalBoneIndex(out, (char *)ptr);
					}
					break;
					case JT_AVROT: //
					{
						//intptr_t size;
						*newBoneList = (intptr_t)Hunk_Alloc(sizeof(tikiBoneAVRot_t),h_low);
						av = (tikiBoneAVRot_t*)*newBoneList;
						av->type=bone->jointType;
						av->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
						ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
		//				Com_Printf("AVROT data weight %f offs %f %f %f unknown %f %f %f %f", ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7],ptr[8]);
						av->m_bone2weight = *ptr;
						ptr++;
						VectorScale(ptr,out->scale,ptr);
						VectorCopy(ptr,av->const_offset);
						ptr+=3;
						ptr+=3; //skip 1 1 1
						//size = (intptr_t)bone + bone->ofsEnd - (intptr_t)ptr;
						//Com_Printf("reference 1 %s ",(char*)ptr);
						bytePtr = (char*)ptr;
						av->m_reference1 = TIKI_GetLocalBoneIndex(out,bytePtr); 
						while(*bytePtr != 0)
							bytePtr++;
						bytePtr++;
						//Com_Printf("reference 2 %s\n",(char*)ptr);
						av->m_reference2 = TIKI_GetLocalBoneIndex(out,bytePtr); 
					}
					break;
					default:
						Com_Error(1,"unkwno nbone type %i",bone->jointType);
					break;
				}
				newBoneList++;
			}
			bone = (skdBone_t *)( (byte *)bone + bone->ofsEnd );
		}
	}
	outSurfacesSize = 0; // size of out->surfaces
	surf = out->surfs;
	for ( i = 0; i < out->numSurfaces; i++) {
		outSurfacesSize+=surf->ofsEnd;
		surf = (skdSurface_t *)( (byte *)surf + surf->ofsEnd );
	}
	
	outSurfs = Z_TagMalloc(outSurfacesSize+(header->ofsBoxes-header->ofsSurfaces),TAG_TIKI);
	memcpy(outSurfs,out->surfs,outSurfacesSize);
	out->numSurfaces+=header->numSurfaces;
	if(out->numSurfaces >= MAX_TIKI_SURFACES) {
		Com_Error(ERR_DROP,"TIKI_MergeSKD: MAX_TIKI_SURFACES (%i) exceed while merging skelmodel %s with tiki %s\n",
			MAX_TIKI_SURFACES,fname,out->name);
	}
	Z_Free(out->surfs);
	out->surfs = outSurfs;

	outSurfs = (skdSurface_t*)(((byte*)outSurfs)+outSurfacesSize);
	memcpy(outSurfs,( ((byte *)header) + header->ofsSurfaces ),header->ofsBoxes-header->ofsSurfaces);
	surf = outSurfs;
	for ( i = 0 ; i < header->numSurfaces ; i++) {
		surf->ident = 12; //SF_SKD;
		vert = (skdVertex_t *) ( (byte *)surf + surf->ofsVerts );
		for ( j = 0 ; j < surf->numVerts ; j++) {
			weight = (skdWeight_t *) ( (byte *)vert + sizeof(skdVertex_t)+(sizeof(skdMorph_t)*vert->numMorphs));
			for ( k = 0 ; k < vert->numWeights ; k++) {
				VectorScale(weight->offset,out->scale,weight->offset);
				weight->boneIndex = TIKI_GetLocalBoneIndex2(out,headerBoneNames[weight->boneIndex]);
				weight = (skdWeight_t *) ( (byte *)weight + sizeof(skdWeight_t));
			}
			vert = (skdVertex_t *) ( (byte *)vert + sizeof(skdVertex_t) + vert->numWeights*sizeof(skdWeight_t) + vert->numMorphs*sizeof(skdMorph_t) );
		}
		surf = (skdSurface_t *)( (byte *)surf + surf->ofsEnd );
	}
	FS_FreeFile(buf);
#if 0
	surf = out->surfs;
	for ( i = 0; i < out->numSurfaces ; i++) {
		Com_Printf("Surf %i of %i - %s \n",i,out->numSurfaces,surf->name);
		surf = (skdSurface_t *)( (byte *)surf + surf->ofsEnd );
	}
#endif
}
void TIKI_AppendSKD(tiki_t *out, char *fname) {
	int				filesize;
	char			*buf;
	skdHeader_t		*header;
	skdBone_t		*bone;
	skdSurface_t	*surf;
	float			*ptr;
	char			*bytePtr;
	int				i,j,k;
	skdVertex_t		*vert;
	skdWeight_t		*weight;
	tikiBonePosRot_t*posrot;
	tikiBoneRotation_t*rot;
	tikiBoneShoulder_t	*should;
	tikiBoneElbow_t		*elbow;
	tikiBoneWrist_t		*wrist;
	tikiBoneHoseRot_t	*hose;
	tikiBoneAVRot_t		*av;

	//Com_Printf("TIKI_AppendSKD for %s, skd name  %s\n", out->name, fname);
	filesize = FS_ReadFile(fname, (void **)&buf);
	if(!buf) {
		Com_Printf ("IKI_AppendSkd: couldn't load %s\n", fname);
		return;
	}
	header = (skdHeader_t *)buf;
	if (header->version != SKD_VERSION) {
		Com_Printf( "TIKI_AppendSKD: %s has wrong version (%i should be %i)\n",
				 fname, header->version, SKD_VERSION);
		return;
	}
	if (header->ident != SKD_IDENT) {
		Com_Printf( "TIKI_AppendSKD: %s has wrong ident (%d should be %d)\n",
				 fname, header->ident, SKD_IDENT);
		return;
	}
	if(header->ofsBones != sizeof(skdHeader_t)) {
		Com_Printf( "TIKI_AppendSKD: strange ofsBones %i in %s\n", header->ofsBones, fname);
	}
	//Com_Printf("SKD file %s data, ident %i, v %i, nae %s, nbones %i, surfs %i ",fname, header->ident,header->version,header->name,header->numBones, header->numSurfaces);
	//Com_Printf("ofs bones %i , surfaces %i, boxes %i, end %i, lods %i, morpgh %i",header->ofsBones, header->ofsSurfaces, header->ofsBoxes,header->ofsEnd,header->ofsLODs, header->ofsMorphTargets);

	//names array
	out->boneNames = Z_TagMalloc(header->numBones * sizeof(int),TAG_TIKI);//Hunk_Alloc((header->numBones * sizeof(int)),h_high);
	bone = (skdBone_t *) ( (byte *)header + header->ofsBones );
	for ( i = 0 ; i < header->numBones ; i++) {
		out->boneNames[i] = TIKI_RegisterBoneName(bone->name);
#if 1
		if(bone->ofsValues != sizeof(skdBone_t)) {
			Com_Error(ERR_DROP,"TIKI_AppendSKD: strange bone->ofsValues %i in model %s\n",bone->ofsValues,fname);
		}
#endif
//		Com_Printf("TIKI_RegisterBoneName returned %i for bone name %s\n",out->boneNames[i],bone->name);
		bone = (skdBone_t *)( (byte *)bone + bone->ofsEnd );
	}

	out->numBones = header->numBones;
	out->bones = Z_TagMalloc(header->numBones*sizeof(int *),TAG_TIKI);//Hunk_Alloc(header->numBones*sizeof(int *),h_high);
	bone = (skdBone_t *) ( (byte *)header + header->ofsBones );
	for ( i = 0 ; i < header->numBones ; i++) {
		switch(bone->jointType)	{
			case JT_ROTATION:
			{
				out->bones[i] = Hunk_Alloc(sizeof(tikiBoneRotation_t),h_low);
				rot = (tikiBoneRotation_t*)out->bones[i];
				rot->type=bone->jointType;
				rot->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
				ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
				VectorScale(ptr,out->scale,rot->const_offset);
			}
			break;
			case JT_POSROT_SKC:
			{
				out->bones[i] = Hunk_Alloc(sizeof(tikiBonePosRot_t),h_low);
				posrot = (tikiBonePosRot_t*)out->bones[i];
				posrot->type=bone->jointType;
				posrot->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
			}
			break;
			case JT_SHOULDER:
			{
				out->bones[i] = Hunk_Alloc(sizeof(tikiBoneShoulder_t),h_low);
				should = (tikiBoneShoulder_t*)out->bones[i];
				should->type=bone->jointType;
				should->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
				ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
				ptr+=(4);
				VectorScale(ptr,out->scale,should->const_offset);
			}
			break;
			case JT_ELBOW:
			{
				out->bones[i] = Hunk_Alloc(sizeof(tikiBoneElbow_t),h_low);
				elbow = (tikiBoneElbow_t*)out->bones[i];
				elbow->type=bone->jointType;
				elbow->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
				ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
				VectorScale(ptr,out->scale,elbow->const_offset);
			}
			break;
			case JT_WRIST:
			{
				out->bones[i] = Hunk_Alloc(sizeof(tikiBoneWrist_t),h_low);
				wrist = (tikiBoneWrist_t*)out->bones[i];
				wrist->type=bone->jointType;
				wrist->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
				ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
				VectorScale(ptr,out->scale,wrist->const_offset);	
			}
			break;
			case JT_HOSEROT:
			{
				out->bones[i] = Hunk_Alloc(sizeof(tikiBoneHoseRot_t),h_low);
				hose = (tikiBoneHoseRot_t*)out->bones[i];
				hose->type=bone->jointType;
				hose->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
				ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
//				Com_Printf("HOSEROT bendRatio %f \n",*ptr);
				hose->bendRatio = *ptr;
				ptr++;
//				Com_Printf("HOSEROT bendMax %f \n",*ptr);
				hose->bendMax = *ptr;
				ptr++;
//				Com_Printf("HOSEROT spinRatio %f \n",*ptr);
				hose->spinRatio = *ptr;
				ptr++;
				VectorScale(ptr,out->scale,hose->basePos);
				ptr+=3;
				//Com_Printf("%f %f %f \n",ptr[0],ptr[1],ptr[2]);
				ptr+=3; // skip 1.f 1.f 1.f
//				Com_Printf("HOSEROT hoseRotType %i \n",*(int*)ptr);
				hose->hoseRotType = *(hoseRotType_t*)ptr;
				ptr++;
				//Com_Printf("\n HOSEref <%s>\n",(char *)ptr);
				hose->targetIndex = TIKI_GetLocalBoneIndex(out, (char *)ptr);
			}
			break;
			case JT_AVROT: //
			{
				//intptr_t size;
				out->bones[i] = Hunk_Alloc(sizeof(tikiBoneAVRot_t),h_low);
				av = (tikiBoneAVRot_t*)out->bones[i];
				av->type=bone->jointType;
				av->parentIndex = TIKI_GetLocalBoneIndex(out,bone->parent);
				ptr = (float *) ( (byte *)bone + sizeof(skdBone_t) );
//				Com_Printf("AVROT data weight %f offs %f %f %f unknown %f %f %f %f", ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7],ptr[8]);
				av->m_bone2weight = *ptr;
				ptr++;
				VectorScale(ptr,out->scale,av->const_offset);
				ptr+=3;
				ptr+=3; //skip 1 1 1
				//size = (intptr_t)bone + bone->ofsEnd - (intptr_t)ptr;
	//			Com_Printf("reference 1 %s ",(char*)ptr);
				bytePtr = (char*)ptr;
				av->m_reference1 = TIKI_GetLocalBoneIndex(out,bytePtr); 
				while(*bytePtr != 0)
					bytePtr++;
				bytePtr++;
	//			Com_Printf("reference 2 %s\n",(char*)ptr);
				av->m_reference2 = TIKI_GetLocalBoneIndex(out,bytePtr); 
			}
			break;
			default:
				Com_Error(1,"unknown nbone type %i",bone->jointType);
			break;
		}
		bone = (skdBone_t *)( (byte *)bone + bone->ofsEnd );
	}

	out->numSurfaces = header->numSurfaces;
	out->surfs = Z_TagMalloc( header->ofsBoxes-header->ofsSurfaces,TAG_TIKI);
	memcpy(out->surfs,( ((byte *)header) + header->ofsSurfaces ),header->ofsBoxes-header->ofsSurfaces);
	surf = out->surfs;
	for ( i = 0 ; i < header->numSurfaces ; i++) {
#if 1
		if(surf->staticSurfProcessed != 0) {
			Com_DPrintf("Surface %s of %s has staticSurfProcessed %i\n",surf->name,fname,surf->staticSurfProcessed);
		}
		if(surf->ofsTriangles != sizeof(skdSurface_t)) {
			Com_Error(ERR_DROP,"TIKI_AppendSKD: strange surf->ofsTriangles %i in model %s\n",surf->ofsTriangles,fname);
		}
#endif
		surf->ident = 12; //SF_SKD;
		vert = (skdVertex_t *) ( (byte *)surf + surf->ofsVerts );
		for ( j = 0 ; j < surf->numVerts ; j++) {
			weight = (skdWeight_t *) ( (byte *)vert + sizeof(skdVertex_t)+(sizeof(skdMorph_t)*vert->numMorphs));
			for ( k = 0 ; k < vert->numWeights ; k++) {
				VectorScale(weight->offset,out->scale,weight->offset);
				if(weight->boneWeight<0 || weight->boneWeight>1) {
					Com_Error(ERR_DROP, "invalid bone weight %f in %s \n",weight->boneWeight,out->name);
				}
				weight = (skdWeight_t *) ( (byte *)weight + sizeof(skdWeight_t));
			}
			vert = (skdVertex_t *) ( (byte *)vert + sizeof(skdVertex_t) + vert->numWeights*sizeof(skdWeight_t) + vert->numMorphs*sizeof(skdMorph_t) );
		}
		surf = (skdSurface_t *)( (byte *)surf + surf->ofsEnd );
	}
	FS_FreeFile(buf);
}

tikiAnim_t *TIKI_CacheAnim(char *fname, float scale) {
	int				filesize;
	char			*buf,*c;
	skcHeader_t		*header;
	skcFrame_t		*frame;
	int				i,j;
	tikiAnim_t		*anim;
	char			channelTypes[256];
	int				nameIndexes[256];
	int				*channelNames;

	//Com_Printf("TIKI_CacheAnim for %s, \n",  fname);
	filesize = FS_ReadFile(fname, (void **)&buf);
	if(!buf) {
		Com_Printf ("TIKI_CacheAnim: couldn't load %s\n", fname);
		return 0;
	}
	header = (skcHeader_t *)buf;
	if (header->version != SKC_VERSION) {
		Com_Printf( "TIKI_CacheAnim: %s has wrong version (%i should be %i)\n",
				 fname, header->version, SKC_VERSION);
		return 0;
	}
	if (header->ident != SKC_IDENT) {
		Com_Printf( "TIKI_CacheAnim: %s has wrong ident (%i should be %i)\n",
				 fname, header->ident, SKC_IDENT);
		return 0;
	}
	if(header->type != 0) {
		//	32 in "models/human/animation/weapon_pistol/walks_runs/pistol_alert_walk_forward.skc"
		// "models/human/animation/weapon_pistol/walks_runs/pistol_alert_walk_backwards.skc"
		Com_Printf("Unusual skc type/flags %i in %s\n",header->type,fname);
	}
	anim = Hunk_Alloc( sizeof(*anim),h_high);
	strcpy(anim->fname,fname); 
	anim->frameTime = header->frameTime;
	anim->numFrames = header->numFrames;

	c = (char *) ( (byte *)header + header->ofsChannels );
	for(i = 0; i < header->numChannels; i++) {
		int len = strlen(c);
		if(!memcmp((c+(len-3)),"pos",3)) {
			channelTypes[i] = 'p';
			anim->numPosChannels++;
			c[len-4]=0;
			nameIndexes[i] = TIKI_RegisterBoneName(c);
		}
		else if(!memcmp((c+(len-2)),"FK",2)) {
			channelTypes[i] = 'f';
			anim->numRotFKChannels++;
			c[len-6]=0;
			nameIndexes[i] = TIKI_RegisterBoneName(c);
		}
		else if(!memcmp((c+(len-3)),"rot",3)) {
			channelTypes[i] = 'r';
			anim->numRotChannels++;
			c[len-4]=0;
			nameIndexes[i] = TIKI_RegisterBoneName(c);
		}
		c+=32;
	}
	channelNames = Hunk_Alloc(header->numChannels*4,h_high);
	anim->posChannelNames = channelNames;
	anim->rotChannelNames = channelNames + anim->numPosChannels;
	anim->rotFKChannelNames = anim->rotChannelNames + anim->numRotChannels;

	anim->numPosChannels = 0;
	anim->numRotChannels = 0;
	anim->numRotFKChannels = 0;
	for(i = 0; i < header->numChannels; i++) {
		if(channelTypes[i] == 'p') {
			anim->posChannelNames[anim->numPosChannels] = nameIndexes[i];
			anim->numPosChannels++;
		}
		else if(channelTypes[i] == 'r')	{
			anim->rotChannelNames[anim->numRotChannels] = nameIndexes[i];
			anim->numRotChannels++;
		}
		else if(channelTypes[i] == 'f')	{
			anim->rotFKChannelNames[anim->numRotFKChannels] = nameIndexes[i];
			anim->numRotFKChannels++;
		}
	}

	anim->frames = Hunk_Alloc(header->numFrames*sizeof(tikiFrame_t),h_low );
	for(i = 0; i < header->numFrames; i++) {
		anim->frames[i].posChannels = Hunk_Alloc(sizeof(vec3_t)*anim->numPosChannels,h_high);
		anim->frames[i].rotChannels = Hunk_Alloc(sizeof(quat_t)*anim->numRotChannels,h_high);
		if(anim->numRotFKChannels)
			anim->frames[i].rotFKChannels = Hunk_Alloc(sizeof(quat_t)*anim->numRotFKChannels,h_high);
	}
	frame = (skcFrame_t *)( (byte *)header + sizeof(*header) );
	for(i = 0; i < header->numFrames; i++) {
		float *values = (float*)((byte *)header+frame->ofsValues);

		//if(frame->unknown != 0.f) {
		//	Com_DPrintf("frame->unknown != 0 (its %f) in skc file %s\n",frame->unknown,fname);
		//}

		anim->numPosChannels = 0;
		anim->numRotChannels = 0;
		anim->numRotFKChannels = 0;
		for(j = 0; j < header->numChannels; j++) {
			if(channelTypes[j] == 'p')	{
				VectorScale(values,scale,anim->frames[i].posChannels[anim->numPosChannels]);
				anim->numPosChannels++;
			}
			else if(channelTypes[j] == 'r')	{
				VectorCopy(values,anim->frames[i].rotChannels[anim->numRotChannels]);
				anim->frames[i].rotChannels[anim->numRotChannels][3] = values[3]; 
				anim->numRotChannels++;
			}
			else if(channelTypes[j] == 'f')	{
				VectorCopy(values,anim->frames[i].rotFKChannels[anim->numRotFKChannels]);
				anim->frames[i].rotFKChannels[anim->numRotFKChannels][3] = values[3];
				anim->numRotFKChannels++;
			}
			values += 4;
		}
		anim->frames[i].radius = frame->radius;
		VectorCopy(frame->bounds[0],anim->frames[i].bounds[0]);
		VectorCopy(frame->bounds[1],anim->frames[i].bounds[1]);
		frame++;
	}
	FS_FreeFile(buf);
	return anim;
}
/*
===============================
TIKI FILES LOADING AND PARSING
===============================
*/
int TIKI_CompareAnimName(const void *v1, const void *v2) {
	tikiAnim_t *anim1;
	tikiAnim_t *anim2;
	anim1 = *(tikiAnim_t**)v1;
	anim2 = *(tikiAnim_t**)v2;
	return strcmp((anim1)->alias,(anim2)->alias);
}
char	*sections[] = {
	"SECTION_ROOT",
	"SECTION_SETUP",
	"SECTION_INIT",
	"SECTION_ANIMATIONS"
};

static tiki_t *TIKI_Load(const char *fname) {
	tiki_t			*out;
	char			*buf,*text, *token;
	char			tmp[128], tmp2[128];
	char			path[128] = "";
	skdSurface_t	*sf;
	int				len;
	int				i, j, level = 0;
	tikiAnim_t		*anim;
	tikiAnim_t		*anims[2048];
	int				numCmds;
	tikiCommand_t	cmds[512];
	char			*cmdTxts[512];
	char			cmdText[2048];
	tiki_t			*includes[TIKI_MAX_INCLUDES];
	qboolean insideIncludesBlock = qfalse;
	qboolean headskinCaseFound = qfalse;
	qboolean headmodelCaseFound = qfalse;
	struct
	{
		char surface[64];
		char shader[64];
	} surfShaders[32];
	int numSurfShaders;
	enum {
		SECTION_ROOT,
		SECTION_SETUP,
		SECTION_INIT,
		SECTION_ANIMATIONS
	}				section = SECTION_ROOT;
	len = FS_ReadFile(fname, (void **)&buf);
	if (!buf) {
		Com_Printf("WARNING: TIKI_Load: Failed to open \"%s\"\n", fname);
		return 0;
	}
	numSurfShaders = 0;

	out = Hunk_Alloc(sizeof(*out), h_high);
	out->scale = 0.52f; // set default scale to 16/30.5 since world 
				// is in 16 units per foot and model is in cm's
	strcpy(out->name,fname);
	text = buf;
	COM_BeginParseSession(fname);
	token = COM_ParseExt(&text, qtrue);
	if(Q_stricmp(token,"TIKI")) {
	//	Com_Printf("WARNING: TIKI_Load: tiki file %s has wrong ident %s, should be TIKI\n",fname,token);
		goto again;
	}

	while (*token) {
		token = COM_ParseExt(&text, qtrue);
again:
		if(*token==0) {
			token = COM_ParseExt(&text, qtrue);
			if(*token==0) {
				break; // end of file
			}
		}
		if (!Q_stricmp(token, "path") || !Q_stricmp(token, "$path")) {
			token = COM_ParseExt(&text, qtrue);
			strcpy(path,token);
		}
		else if(!Q_stricmp(token, "$define")) {
			token = COM_ParseExt(&text, qtrue);
			// TODO
		}
		else if (!Q_stricmp(token, "$include")) {
			// su44: moved it here, because in singleplayer human models
			// "$include" keyword is used in "setup" section as well.
			// see models/human/coxswain.tik
			token = COM_ParseExt(&text, qtrue);
			strcpy(tmp,token);
			includes[out->numIncludes] = TIKI_RegisterModel(tmp);
			if(includes[out->numIncludes]) {
				out->numIncludes++;
			} else {
				Com_Printf("TIKI_Load: failed to include %s in %s\n",tmp,fname);
			}
			goto again;
		}
		else if(!Q_stricmp(token, "includes")) {
			// su44: "includes" keyword is used only by singleplayer human models
			// see models/human/new_generic_human.tik
			qboolean load = qfalse;
			const char *mapName;

			mapName = Cvar_VariableString("mapname");
			while(token[0] && token[0] != '{') {
				token = COM_ParseExt(&text, qfalse);
				if(!Q_stricmpn(mapName,token,strlen(mapName))) {
					load = qtrue;
				}
			}
			if(token[0] != '{')
				token = COM_ParseExt(&text, qtrue);
			if(token[0] != '{') {
				Com_Error(ERR_DROP,"TIKI_Load: Expected { to follow 'includes', found %s",token);
			}
			if(token[1])
				token++;
			else
				token = COM_ParseExt(&text, qtrue);

			if(load == qfalse) {
				int tmpLevel;
				
				tmpLevel = 1;
				// skip block between curly braces
				while(1) {
					if(token[0] == '}') {
						tmpLevel--;
						if(tmpLevel == 0)
							break;
					} else if(token[0] == '{') {
						tmpLevel++;
					}
					token = COM_ParseExt(&text, qtrue);
				}
			} else {
				insideIncludesBlock = qtrue;
				goto again;
			}
		}
		else if(token[0]=='}') {
			if(level == 0 && insideIncludesBlock) {
				insideIncludesBlock = qfalse;
			} else {
				level--;
				if(level == 0) {
					section = SECTION_ROOT;
				}
			}
			token++;
			goto again;
		}
		else if(!Q_stricmp(token, "skelmodel")) {
			// another hack for singleplayer human models
			goto parseSkelmodel;
		}
		else if(!Q_stricmp(token, "surface")) {
			// another hack for singleplayer human models
			goto parseSurface;
		}
		else if(!Q_stricmp(token, "case")) {
			// used only in singleplayer human models
			token = COM_ParseExt(&text, qtrue);
			if(!Q_stricmp(token, "headmodel")) {
				// there can be more than one argument
				while(token[0]) {
					token = COM_ParseExt(&text, qfalse);
				}
				token = COM_ParseExt(&text, qtrue);
				if(token[0] != '{') {
					Com_Error(ERR_DROP,"TIKI_Load: Expected { to follow 'case headmodel', found %s",token);
				}
				if(token[1])
					token++;
				else
					token = COM_ParseExt(&text, qtrue);
				// HACK - always load first case
				if(headmodelCaseFound) {
					int tmpLevel;
					
					tmpLevel = 1;
					// skip block between curly braces
					while(1) {
						if(token[0] == '}') {
							tmpLevel--;
							if(tmpLevel == 0)
								break;
						} else if(token[0] == '{') {
							tmpLevel++;
						}
						token = COM_ParseExt(&text, qtrue);
					}
				} else {
					headmodelCaseFound = qtrue;
				}
				goto again;
			} else if(!Q_stricmp(token, "headskin")) {
				// there can be more than one argument
				while(token[0]) {
					token = COM_ParseExt(&text, qfalse);
				}
				token = COM_ParseExt(&text, qtrue);
				if(token[0] != '{') {
					Com_Error(ERR_DROP,"TIKI_Load: Expected { to follow 'case headskin', found %s",token);
				}
				if(token[1])
					token++;
				else
					token = COM_ParseExt(&text, qtrue);
				// HACK - always load first case
				if(headskinCaseFound) {
					int tmpLevel;
					
					tmpLevel = 1;
					// skip block between curly braces
					while(1) {
						if(token[0] == '}') {
							tmpLevel--;
							if(tmpLevel == 0)
								break;
						} else if(token[0] == '{') {
							tmpLevel++;
						}
						token = COM_ParseExt(&text, qtrue);
					}					
				} else {
					headskinCaseFound = qtrue;
					
				}
				goto again;
			} else if(!Q_stricmp(token, "weapon")) {

			} else {
				Com_Error(ERR_DROP,"TIKI_Load: unknown 'case' %s in TIKI file %s",token,fname);
			}
		}
		else {
			switch (section) {
				case SECTION_ROOT: 
					if (!Q_stricmp(token, "setup"))	{
						token = COM_ParseExt(&text, qtrue);
						if(token[0]!='{')
							Com_Error(1,"expected { to follow setup, found %s",token);
						token++;
						level++;
						section = SECTION_SETUP;
						goto again;
					}
					else if (!Q_stricmp(token, "init")) {
						token = COM_ParseExt(&text, qtrue);
						if(token[0]!='{')
							Com_Error(1,"expected { to follow init, found %s",token);
						token++;
						level++;
						section = SECTION_INIT;
						goto again;
					}
					else if (!Q_stricmp(token, "animations")) {
						token = COM_ParseExt(&text, qtrue);
						if(token[0]!='{')
							Com_Error(1,"expected { to follow animations, found %s",token);
						token++;
						level++;
						section = SECTION_ANIMATIONS;
						goto again;
					}
					else {
						Com_Printf("Unknown token %s in section %s of file %s\n",token,sections[section],fname);
					}
				break;
				case SECTION_SETUP:
					if (!Q_stricmp(token, "skelmodel")) {
parseSkelmodel:
						token = COM_ParseExt(&text, qtrue);
						strcpy(tmp,path);
						i = strlen(tmp);
						if(i && tmp[i-1] != '/')
							strcat(tmp,"/");
						strcat(tmp,token);
						if(out->surfs == 0)
							TIKI_AppendSKD(out,tmp);
#if 1
						else
							TIKI_MergeSKD(out,tmp);
#endif
					}
					else if (!Q_stricmp(token, "surface")) {
parseSurface:
						token = COM_ParseExt(&text, qtrue);
						strcpy(tmp,token);
						token = COM_ParseExt(&text, qtrue);
						if (!Q_stricmp(token, "shader")) {
							strcpy(surfShaders[numSurfShaders].surface,tmp);
							token = COM_ParseExt(&text, qtrue);
							strcpy(surfShaders[numSurfShaders].shader,token);

							numSurfShaders++;
						}
					}
					else if (!Q_stricmp(token, "scale")) {
						token = COM_ParseExt(&text, qtrue);
						out->scale = atof(token);
					}
					else if (!Q_stricmp(token, "lod_scale")) {
						token = COM_ParseExt(&text, qtrue);
						out->lod_scale = atof(token);
					}
					else if (!Q_stricmp(token, "ischaracter")) {

					}
					else if (!Q_stricmp(token, "radius")) {
						token = COM_ParseExt(&text, qtrue);
						
					}
					else {
						Com_Printf("Unknown token %s in section %s of file %s\n",token,sections[section],fname);
					}
				break;
				case SECTION_INIT:
					out->init = Hunk_Alloc(sizeof(tikiInit_t),h_low);
					if (!Q_stricmp(token, "server")) {
						token = COM_ParseExt(&text, qtrue);
						if(token[0]!='{')
							Com_Printf("expected { to follow server, found %s\n",token);

						while(token[0]) {
							token = COM_ParseExt(&text, qtrue);
							if(token[0] == '}') 
								break;
						}
					} else if (!Q_stricmp(token, "client")) {
						char **cmd = cmdTxts;
						numCmds = 0;
						token = COM_ParseExt(&text, qtrue);
						if(token[0]!='{')
							Com_Printf("expected { to follow server, found %s\n",token);

						while(1) {
							cmdText[0] = 0;
							while(1) {
								token = COM_ParseExt(&text, qfalse);
								if(token[0]==0 || token[0] == '}') {
									break;
								}
								if(cmdText[0]!=0) {
									strcat(cmdText," ");
								}
								strcat(cmdText,token);
							}
							if(cmdText[0]) {
								*cmd = Hunk_Alloc(strlen(cmdText)+1,h_low);
								strcpy(*cmd,cmdText);
								//Com_Printf("Event text: %s\n",*cmd);
								// next command
								cmd++;
								numCmds++;
							}
							if(token[0] == '}') 
								break;
						}
						if(numCmds) {
							out->init->numClientCmds = numCmds;
							out->init->clCmds = Hunk_Alloc(sizeof(char*)*numCmds,h_low);
							memcpy(out->init->clCmds,cmdTxts,sizeof(char*)*numCmds);
						}
					} else if(token[0]) {
						Com_Printf("Unknown token %s in section %s of file %s\n",token,sections[section],fname);
					}
				break;
				case SECTION_ANIMATIONS:
					strcpy(tmp,token);
					token = COM_ParseExt(&text, qtrue);
					strcpy(tmp2,path);
					i = strlen(tmp2);
					if(tmp2[i-1] != '/')
						strcat(tmp2,"/");
					strcat(tmp2,token);
					anim = TIKI_CacheAnim(tmp2,out->scale);
					if(anim) {
						strcpy(anim->alias,tmp);
						anims[out->numAnims] = anim;
						out->numAnims++;
						anim->flags = 0;
						while(1) {
							token = COM_ParseExt(&text, qfalse);
							if(!*token)
								break;
							if(!Q_stricmp(token,"autosteps_run")) {
								anim->flags |= TAF_AUTOSTEPS_RUN;
							} else if(!Q_stricmp(token,"autosteps_walk")) {
								anim->flags |= TAF_AUTOSTEPS_WALK;
							} else if(!Q_stricmp(token,"random")) {
								anim->flags |= TAF_RANDOM;
							} else if(!Q_stricmp(token,"dontrepeate")) {
								anim->flags |= TAF_DONTREPEAT;
							} else if(!Q_stricmp(token,"deltadriven")) {
								anim->flags |= TAF_DELTADRIVEN;
							} else if(!Q_stricmp(token,"default_angles")) {
								anim->flags |= TAF_DEFAULTANGLES;
							} else if(!Q_stricmp(token,"weight")) {
								token = COM_ParseExt(&text, qfalse);
								anim->weight = atof(token);
							} else if(!Q_stricmp(token,"crossblend")) {
								token = COM_ParseExt(&text, qfalse);
								anim->crossblend = atof(token);
							} else {
								Com_Printf("Unknown animation flag %s in file %s\n",token,fname);
							}
						}
					} else {
						while(1) {
							token = COM_ParseExt(&text, qfalse);
							if(!*token)
								break;
						}
					}

					token = COM_ParseExt(&text, qtrue);
					if(token[0] == '{')	{
						while(1) {
							token = COM_ParseExt(&text, qtrue);
							if(token[0] == '}')
								break;
							if (!Q_stricmp(token, "server")) {
								token = COM_ParseExt(&text, qtrue);
								if(token[0]!='{')
									Com_Printf("expected { to follow server, found %s\n",token);

								while(1) {
									token = COM_ParseExt(&text, qtrue);
									if(token[0] == '}') 
										break;
								}
							} else if (!Q_stricmp(token, "client")) {
								tikiCommand_t *cmd = cmds;
								numCmds = 0;
								token = COM_ParseExt(&text, qtrue);
								if(token[0]!='{')
									Com_Printf("expected { to follow server, found %s\n",token);

								while(1) {
									cmdText[0] = 0;
									token = COM_ParseExt(&text, qtrue);
									if(token[0] == '}')
										break;
									// parse frame index
									if(!Q_stricmp(token,"entry") || !Q_stricmp(token,"enter")) {
										cmd->frame = TIKI_FRAME_ENTRY;
									} else if(!Q_stricmp(token,"exit")) {
										cmd->frame = TIKI_FRAME_EXIT;
									} else if(!Q_stricmp(token,"first")) {
										cmd->frame = 0;
									} else if(!Q_stricmp(token,"last")) {
										cmd->frame = anim->numFrames-1;
									} else {
										cmd->frame = atoi(token);
									}
									// parse event
									while(1) {
										token = COM_ParseExt(&text, qfalse);
										if(token[0]==0 || token[0] == '}') {
											break;
										}

#if 1
										// HACK - MoHAA allows linebreaks in tagspawn commands
										if(!Q_stricmp(token,"tagspawn") || !Q_stricmp(token,"tagspawnlinked")
											 || !Q_stricmp(token,"originspawn")) {
											if(cmdText[0]!=0) {
												strcat(cmdText," ");
											}
											strcat(cmdText,token);
											do {
												token = COM_ParseExt(&text, qtrue);
												if(token[0]==0) {
													Com_Printf("Unexpected end of file found in tagspawn command block in tiki file %s\n",fname);
													break;
												}
												if(cmdText[0]!=0) {
													strcat(cmdText," ");
												}
												strcat(cmdText,token);
											} while(token[0] != ')');
										}
#endif
										if(cmdText[0]!=0) {
											strcat(cmdText," ");
										}
										strcat(cmdText,token);
									}
									if(cmdText[0]) {
										cmd->text = Hunk_Alloc(strlen(cmdText)+1,h_low);
										strcpy(cmd->text,cmdText);
										//Com_Printf("Event text: %s\n",cmd->text);
										// next command
										cmd++;
										numCmds++;
									}
									if(token[0] == '}') 
										break;
								}
								if(numCmds && anim) {
									anim->numClientCmds = numCmds;
									anim->clientCmds = Hunk_Alloc(sizeof(tikiCommand_t)*numCmds,h_low);
									memcpy(anim->clientCmds,cmds,sizeof(tikiCommand_t)*numCmds);
								}
							} else {
								Com_Printf("Unknown token %s in section %s of file %s\n",token,sections[section],fname);
							}	
						}
					}
					else {
						if(*token==0)
							break;
						goto again;
					}

				break;
			}
		}
	}

	out->includes = Hunk_Alloc(sizeof(tiki_t*)*out->numIncludes,h_high);
	memcpy(out->includes,includes,sizeof(tiki_t*)*out->numIncludes);

	// merge surfaces of all includes
	for(i = 0; i < out->numIncludes; i++) {
		if(out->includes[i]->surfs) {
#if 1 //FIXME!
			if(!out->surfs)	{
				out->surfs = out->includes[i]->surfs;
				out->numSurfaces = out->includes[i]->numSurfaces;
				out->surfShaders = out->includes[i]->surfShaders;
				out->boneNames =  out->includes[i]->boneNames;
				out->bones =  out->includes[i]->bones;
				out->numBones =  out->includes[i]->numBones;
			} else {
				// TODO: code this, without the function below singleplayer
				// human head models won't show up.
				/// TIKI_MergeSKD2(out->surfs,out->includes[i]->surfs);
			}
#endif
		}
	}

	// build anim list
	for(i = 0; i < out->numIncludes; i++) {
		tikiAnim_t **freeAnimPtr = &anims[out->numAnims];
		out->numAnims+=out->includes[i]->numAnims;
		memcpy(freeAnimPtr,out->includes[i]->anims,out->includes[i]->numAnims*sizeof(tikiAnim_t*));
	}
	qsort(anims,out->numAnims,sizeof(tikiAnim_t*),TIKI_CompareAnimName);
#if 0
	{
		char str[512];
		FILE *f = fopen("anims_sorted.txt","w");
		sprintf(str,"Sorted anims for tiki file %s (total anims %i)\n",out->name,out->numAnims);
		fwrite(str,strlen(str),1,f);
		for(i = 0; i < out->numAnims; i++) {
			sprintf(str,"%i: %s (%s)\n",i,anims[i]->alias,anims[i]->fname);
			fwrite(str,strlen(str),1,f);		
		}
		fclose(f);
	}
#endif
	out->anims = Hunk_Alloc(sizeof(tikiAnim_t*)*out->numAnims,h_high);
	memcpy(out->anims,anims,sizeof(tikiAnim_t*)*out->numAnims);

	if(!out->surfShaders) {
		out->surfShaders = Hunk_Alloc(sizeof(qhandle_t)*out->numSurfaces,h_high);
		sf = out->surfs;
		for(i = 0; i < out->numSurfaces; i++) {
			for(j = 0; j < numSurfShaders; j++) {
				// "all" keyword is used in eg. models/ammo/thompson_clip.tik
				if(!Q_stricmp(surfShaders[j].surface,"all")) {
					out->surfShaders[i] = TIKI_RE_RegisterShader(surfShaders[j].shader);
				} else {
					int testLen;
					char *star;
					// '*' is used in eg. models/weapons/mp40.tik
					star = strchr(surfShaders[j].surface,'*');
					if(star) {
						testLen = star - surfShaders[j].surface;
					} else {
						testLen = strlen(surfShaders[j].surface)+1;
					}
					if(!Q_stricmpn(sf->name,surfShaders[j].surface,testLen)) {
						out->surfShaders[i] = TIKI_RE_RegisterShader(surfShaders[j].shader);
					}
				}
			}
			sf = (skdSurface_t*)(((byte*)sf)+sf->ofsEnd);
		}
	}
	FS_FreeFile(buf);
	return out;
}

// called from Hunk_Clear
void TIKI_ClearUp() {
	memset(hashTable,0,TIKI_FILE_HASH_SIZE*4);
	memset(globalBoneNames,0,sizeof(globalBoneNames));
	freeBoneName = globalBoneNames;
	memset(boneHashTable,0,sizeof(boneHashTable));
	Z_FreeTags( TAG_TIKI );
}
tiki_t	*TIKI_RegisterModel(const char *fname) {
	tiki_t	*tiki;
	int		hash;
	if(!*fname)
		return 0;
#if 0
	if(!strcmp("models/weapons/steilhandgranate.tik",fname))
		__asm int 3
#endif
	hash = generateHashValue(fname, TIKI_FILE_HASH_SIZE);
	// see if the TIKI is already loaded
	for (tiki = hashTable[hash]; tiki; tiki = tiki->next) {
		if (!Q_stricmp(tiki->name, fname))
			// match found
			return tiki;
	}
	tiki = TIKI_Load(fname);
	if (!tiki) {
		Com_Printf("WARNING: TIKI_Load: Failed to load \"%s\"\n", fname);
		return 0;
	}
	if (hashTable[hash]) {
		tiki->next = hashTable[hash];
	}
	hashTable[hash] = tiki;
	return tiki;
}

/*
===============================
DATA QUERYING API
===============================
*/
