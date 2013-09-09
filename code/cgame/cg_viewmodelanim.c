/*
===========================================================================
Copyright (C) 2010 su44

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

#include "cg_local.h"

char itemNames[64][64];
void CG_RegisterItemName(int index, const char *str) {
	if(index >= 64 || index < 0) {
		CG_Error("CG_RegisterItemName: item index out of range 64 (%i, %s)\n",index,str);
		return;
	}
	strcpy(itemNames[index],str);
}

const char *CG_GetItemName(int index) {
	if(index >= 64 || index < 0) {
		CG_Error("CG_GetItemName: item index out of range 64 (%i)\n",index);
		return 0;
	}
	return itemNames[index];
}
//papers  colt45  p38 histandard  garand  kar98   kar98sniper springfield thompson    mp40    bar mp44    fraggrenade stielhandgranate    bazooka panzerschreck   shotgun unarmed
//Papers  Colt 45 Walther P38 Hi-Standard Silenced    M1 Garand   Mauser KAR 98K  KAR98 - Sniper  Springfield '03 Sniper  Thompson    MP40    BAR StG 44  Frag Grenade    Stielhandgranate    Bazooka Panzerschreck   Shotgun
const char *CG_GetVMAnimPrefixString(int index) {
	const char *wpn;
	wpn = CG_GetItemName(index);
	if(wpn) {
		if(!Q_stricmp(wpn,"Papers"))
			return "papers";
		if(!Q_stricmp(wpn,"Colt 45"))
			return "colt45";
		if(!Q_stricmp(wpn,"Walther P38"))
			return "p38";
		if(!Q_stricmp(wpn,"Hi-Standard Silenced"))
			return "histandard";
		if(!Q_stricmp(wpn,"M1 Garand"))
			return "garand";
		if(!Q_stricmp(wpn,"Mauser KAR 98K"))
			return "kar98";
		if(!Q_stricmp(wpn,"KAR98 - Sniper"))
			return "kar98sniper";
		if(!Q_stricmp(wpn,"Springfield '03 Sniper"))
			return "springfield";
		if(!Q_stricmp(wpn,"Thompson"))
			return "thompson";
		if(!Q_stricmp(wpn,"MP40"))
			return "mp40";
		if(!Q_stricmp(wpn,"BAR"))
			return "bar";
		if(!Q_stricmp(wpn,"StG 44"))
			return "mp44";
		if(!Q_stricmp(wpn,"Frag Grenade"))
			return "fraggrenade";
		if(!Q_stricmp(wpn,"Stielhandgranate"))
			return "stielhandgranate";
		if(!Q_stricmp(wpn,"Bazooka"))
			return "bazooka";
		if(!Q_stricmp(wpn,"Panzerschreck"))
			return "panzerschreck";
		if(!Q_stricmp(wpn,"Shotgun"))
			return "shotgun";
	}
	return "unarmed";
}

char	*viewModelAnimTypes[] = {
	"idle", // 0
	"charge", // 1
	"fire", // 2
	"fire_secondary", // 3
	"rechamber", // 4
	"reload", // 5
	"reload_single", // 6
	"reload_end", // 7
	"pullout", // 8
	"putaway", // 9
	"ladderstep" // 10
};
const char *CG_GetVMTypeString(int index) {
	if(index < 0 || index > VMA_NUMANIMATIONS) {
		return "idle";
	}
	return viewModelAnimTypes[index];
}
void CG_AddViewModelAnimAttachment(refEntity_t *ent, centity_t *cent) {
	vec3_t outRot;
	int boneName;
	int i;
	if(cg.renderingThirdPerson || cg.viewModelEnt.bones == 0)
		return;
	if(cg.snap->ps.stats[STAT_INZOOM])
		return;
	ent->renderfx = RF_FIRST_PERSON;
#if 0 //doesnt work, tag_num is incorrect..
	CG_BoneLocal2World(cg.viewModelEnt.bones+cent->currentState.tag_num,cg.viewModelEnt.origin,cg.refdefViewAngles,ent->origin,outRot);
#else
	if(cgs.gameTIKIs[cg_entities[cent->currentState.parent].currentState.modelindex]==0) {
		//CG_Printf("CG_AddViewModelAnimAttachment: entity %i has null tiki; cannot attach %i\n",cent->currentState.parent, cent->currentState.number);
		return;
	}
	boneName = cgs.gameTIKIs[cg_entities[cent->currentState.parent].currentState.modelindex]->boneNames[cent->currentState.tag_num];
	for(i = 0; i < cg.viewModelTiki->numBones; i++) {
		if(boneName == cg.viewModelTiki->boneNames[i])
			break;
	}
	if(i == cg.viewModelTiki->numBones)
		return;
	CG_BoneLocal2World(cg.viewModelEnt.bones+i,cg.viewModelEnt.origin,cg.refdefViewAngles,ent->origin,outRot);
#endif
	VectorCopy(ent->origin,cent->lerpOrigin);
	VectorCopy(outRot,cent->lerpAngles);
	AnglesToAxis(outRot,ent->axis);

	trap_R_AddRefEntityToScene(ent);
}
void CG_CalcViewModelMovement(float fViewBobPhase, float fViewBobAmp, 
		float *vVelocity, float *vMovement ) {
	float tmp,tmp2;
	float frac;
	vec3_t vmOfs;
	float vMovementLen;
	vec3_t vMovementNormalized;
	float *curAnimVMPosOfs;
	int i;

	vMovement[1] = sin(fViewBobPhase + 0.3141592700403172) * fViewBobAmp * vm_sway_side.value;
	vMovement[0] = vMovement[1] * vm_sway_front.value;
	tmp = sin(fViewBobPhase - 0.9424778335276408 + fViewBobPhase - 0.9424778335276408 + 3.141592653589793)
	 + sin((fViewBobPhase - 0.9424778335276408) * 4.0 + 1.570796326794897) * 0.125;
	vMovement[2] = fViewBobAmp * tmp * vm_sway_up.value;

	//if(!cg.predictedPlayerState.walking) {
	if(cg.predictedPlayerState.groundEntityNum == ENTITYNUM_NONE) {
		// player is in air
		vmOfs[0] = vm_offset_air_front.value;
		vmOfs[1] = vm_offset_air_side.value;
		vmOfs[2] = vm_offset_air_up.value;
		// append offset depending on Z velocity
		if(vVelocity[2] != 0.f) {
			vmOfs[2] -= vVelocity[2] * vm_offset_upvel.value;
		}
	} else {
		// player is walking
		if ( cg.predictedPlayerState.viewheight == 48 ) {
			// and crouching?

			/*
			// special cases for shotgun and rocked launchers - TODO
			if(using shotgun) {
				vmOfs[0] = vm_offset_shotguncrouch_front.value;
				vmOfs[1] = vm_offset_shotguncrouch_side.value;
				vmOfs[2] = vm_offset_shotguncrouch_up.value;
			} else if(using rockets) {
				vmOfs[0] = vm_offset_rocketcrouch_front.value;
				vmOfs[1] = vm_offset_rocketcrouch_side.value;
				vmOfs[2] = vm_offset_rocketcrouch_up.value;
			} else
			*/
			{
				vmOfs[0] = vm_offset_crouch_front.value;
				vmOfs[1] = vm_offset_crouch_side.value;
				vmOfs[2] = vm_offset_crouch_up.value;
			}
		} else {
			// player is standing
			VectorClear(vmOfs);
		}
		tmp = VectorLength(vVelocity) - vm_offset_vel_base.value;

		if(tmp > 0) {
			tmp2 = 250.0 - vm_offset_vel_base.value;
			if( tmp > tmp2 )
				tmp = tmp2;
			frac = tmp / tmp2;	

			vmOfs[0] += frac * vm_offset_vel_front.value;
			vmOfs[1] += frac * vm_offset_vel_side.value;
			vmOfs[2] += frac * vm_offset_vel_up.value;
		}



	}


	// for each coordinate
	curAnimVMPosOfs = &cg.g_vCurrentVMPosOffset[0];
	for(i = 0; i < 3; i++) {
		float ofsDelta;
		ofsDelta = vmOfs[i] - curAnimVMPosOfs[i];
		curAnimVMPosOfs[i] += (float)cg.frametime
			* 0.001
			* ofsDelta
			* vm_offset_speed.value;
		//if(ofsDelta < 0) {
		//	if ( vmOfs[i] <= curAnimVMPosOfs[i] ) {

		//	} else {
		//		curAnimVMPosOfs[i] = vmOfs[i];
		//	}
		//} else if(ofsDelta > 0) {
		//	if ( vmOfs[i] < curAnimVMPosOfs[i] ) {

		//	} else {
		//		curAnimVMPosOfs[i] = vmOfs[i];
		//	}
		//}
	}

	vMovement[0] = vMovement[0] + cg.g_vCurrentVMPosOffset[0];
	vMovement[1] = vMovement[1] + cg.g_vCurrentVMPosOffset[1];
	vMovement[2] = vMovement[2] + cg.g_vCurrentVMPosOffset[2];

	// add lean offset
	if(cg.predictedPlayerState.fLeanAngle != 0.f) {
		vMovement[2] -= fabs(cg.predictedPlayerState.fLeanAngle) * vm_lean_lower.value;
	}
	vMovementLen = VectorNormalize2(vMovement, vMovementNormalized);
	if(vm_offset_max.value < vMovementLen) {
		vMovement[0] = vm_offset_max.value * vMovementNormalized[0];
		vMovement[1] = vm_offset_max.value * vMovementNormalized[1];
		vMovement[2] = vm_offset_max.value * vMovementNormalized[2];
	}



}

void CG_ViewModelAnim() {
	char anim[128];
	char tmp[128];
	int i;
	char *ptr;
	tiki_t *fps, *tiki;
	refEntity_t *ent;
	bone_t *bone;
	vec3_t v,a;
	int boneName;
	matrix_t m;
	vec3_t vMovement;

	if(cg.renderingThirdPerson)
		return;

	if(cg.snap->ps.stats[STAT_INZOOM])
		return;

	ent = &cg.viewModelEnt;

	//CG_Printf("vma changed %i\n",cg.predictedPlayerState.iViewModelAnimChanged);

	// mp40_reload, mp40_fire
	if(cg.predictedPlayerState.activeItems[ITEM_WEAPON]!=-1)	{
		sprintf(anim,"%s_%s",CG_GetVMAnimPrefixString(cg.predictedPlayerState.activeItems[ITEM_WEAPON]),CG_GetVMTypeString(cg.predictedPlayerState.iViewModelAnim));
		//CG_Printf("VMA %s \n",anim);
	} else
		return;
#if 0
	for(i = 0; i < 8; i++) {
		if(cg.predictedPlayerState.activeItems[i]!= -1 && cg.predictedPlayerState.activeItems[i]) {
			CG_Printf("Item %i of 8, %s (indx %i)\n",i,itemNames[cg.predictedPlayerState.activeItems[i]],cg.predictedPlayerState.activeItems[i]);
		}
		/*
item 0 "pistol"
item 1 "Walther P38"
item 2 "Binoculars"

item 0 "mg"
item 1 "MP44"
item 2 "Binoculars"
		*/
	}
#endif
	//tiki = cgs.gameTIKIs[cg.predictedPlayerEntity.currentState.modelindex]; // doesnt work... currentState.modelindex == 0
	tiki = cgs.gameTIKIs[cg_entities[cg.clientNum].currentState.modelindex];
	if(!tiki) {
		if(cg.snap->ps.stats[STAT_TEAM]==TEAM_AXIS) {
			tiki = trap_TIKI_RegisterModel("models/player/german_wehrmacht_soldier.tik");
		} else {
			tiki = trap_TIKI_RegisterModel("models/player/american_army.tik");
		}
		if(!tiki)
			return;
	}
	strcpy(tmp,tiki->name);
	ptr = strchr(tmp,'.');
	if(!ptr)
		return;
	*ptr = 0;
	strcat(tmp,"_fps.tik");
	fps = trap_TIKI_RegisterModel(tmp);
	if(!fps) {
		CG_Printf("CG_ViewModelAnim: error, cannot register first person player model for %s\n",tiki->name);
		return;
	}
	cg.viewModelTiki = fps;
	for(i = 0; i < fps->numAnims; i++) {
//		if(!Q_stricmp(anim,fps->anims[i]->alias)) {
		if(!Q_stricmpn(anim,fps->anims[i]->alias,strlen(anim))) {
			break;
		}
	}
	if(fps->numAnims == i) {
		CG_Printf("WARNING: vma %s not found in %s\n",anim,fps->name);
		return;
	}
	cg.viewModelAnimTime += ((float)cg.frametime/2000);
	if(cg.lastViewModelAnim != i) {
		cg.viewModelAnimTime = 0;
		cg.lastViewModelAnim = i;
	} else if((fps->anims[i]->frameTime*fps->anims[i]->numFrames) < cg.viewModelAnimTime) {
		cg.viewModelAnimTime = 0;
	}
	// su44: ok, we have chosen the proper viewmodelanim
	// but ent->origin and ent->axis still needs to be adjusted
	memset(ent,0,sizeof(*ent));
	ent->renderfx = RF_FIRST_PERSON;
	ent->hModel = trap_R_RegisterModel(tmp);
	ent->bones = trap_TIKI_GetBones(fps->numBones);
	// dont bother culling hands model
	VectorSet(ent->bounds[0],-99999,-99999,-99999);
	VectorSet(ent->bounds[1],99999,99999,99999);
	ent->radius = 99999;

	// su44: kinda strange, but thats how its done by MoHAA
	// we have two different hand surfaces - one is visible only while carrying rifles,
	// second is used for the rest of weapons
	ptr = itemNames[cg.predictedPlayerState.activeItems[ITEM_WEAPON]];
	if(!Q_stricmp(ptr,"M1 Garand") || !Q_stricmp(ptr,"Springfield '03 Sniper")
		|| !Q_stricmp(ptr,"KAR98 - Sniper") || !Q_stricmp(ptr,"Mauser KAR 98K")) {
		ent->surfaceBits |= (1<<TIKI_GetSurfaceIndex(fps,"lefthand")); // hide "lefthand" surface
	} else {
		ent->surfaceBits |= (1<<TIKI_GetSurfaceIndex(fps,"garandhand")); // hide "garandhand" surface
	}

	trap_TIKI_SetChannels(fps,i,cg.viewModelAnimTime,1,ent->bones);
	trap_TIKI_Animate(fps,ent->bones);

	if ( cg.predictedPlayerState.groundEntityNum != ENTITYNUM_NONE ) {
		float vel = VectorLength(cg.predictedPlayerState.velocity);
		cg.fCurrentViewBobPhase = (((float)cg.frametime) * 0.001) * 3.141592653589793
		* (vel * 0.001500000013038516 + 0.8999999761581421) + cg.fCurrentViewBobPhase;
		if ( cg.fCurrentViewBobAmp != 0.0 )
			vel = vel * 0.5;
		cg.fCurrentViewBobAmp = vel;

		if ( cg.predictedPlayerState.fLeanAngle != 0.f )
			cg.fCurrentViewBobAmp = cg.fCurrentViewBobAmp * 0.75;
		cg.fCurrentViewBobAmp = (1.0 - fabs(cg.refdefViewAngles[0]) * 0.01111111138015985 * 0.5)
			* 0.5
			* cg.fCurrentViewBobAmp;
	} else {
		if ( cg.fCurrentViewBobAmp > 0.0 ) {
			float f;
			f = ((float)cg.frametime) * 0.001 * cg.fCurrentViewBobAmp;
			cg.fCurrentViewBobAmp -= (f + f);



		}
	}
	if ( cg.fCurrentViewBobAmp > 0.0 ) {
	
	}
	CG_CalcViewModelMovement(cg.fCurrentViewBobPhase,cg.fCurrentViewBobAmp,cg.predictedPlayerState.velocity,vMovement);
	//CG_Printf("Movement: %f %f %f\n",vMovement[0],vMovement[1],vMovement[2]);

	// in MoHAA "eyes bone" is used here
	bone = 0;
	boneName = trap_TIKI_GetBoneNameIndex("eyes bone"); 
	for(i = 0; i < fps->numBones; i++) {
		if(fps->boneNames[i] == boneName) {
			bone = ent->bones+i;
			break;
		}
	}
	if(bone == 0) {
		CG_Printf("CG_OffsetFirstPersonView warning: Couldn't find 'eyes bone' for player\n");
	}
#if 1


	//VectorAdd(bone->p,vMovement,bone->p);
	VectorSubtract(bone->p,vMovement,bone->p);


#if 0
	CG_BoneLocal2World(bone,vec3_origin,cg.refdefViewAngles,v,a);
#else
	MatrixFromAngles(m,cg.refdefViewAngles[0],cg.refdefViewAngles[1],cg.refdefViewAngles[2]);
	MatrixTransformPoint(m,bone->p,v);
#endif
	VectorSubtract( cg.refdef.vieworg,v,ent->origin);
#else
	VectorCopy(cg.predictedPlayerState.origin,ent->origin);
	ent->origin[2] += cg.predictedPlayerState.viewheight;
	//ent->origin[2] += bone->p[2];
	// transform vMovement from players model space to worldspace
	MatrixFromAngles(m,cg.refdefViewAngles[0],cg.refdefViewAngles[1],cg.refdefViewAngles[2]);
	MatrixTransformPoint(m,vMovement,v);
	// add it
	VectorAdd(ent->origin,v,ent->origin);
#endif

	VectorMA( ent->origin, cg_gun_x.value, cg.refdef.viewaxis[0], ent->origin );
	VectorMA( ent->origin, cg_gun_y.value, cg.refdef.viewaxis[1], ent->origin );
	VectorMA( ent->origin, cg_gun_z.value, cg.refdef.viewaxis[2], ent->origin );



	VectorCopy(cg.refdef.viewaxis[0],ent->axis[0]);
	VectorCopy(cg.refdef.viewaxis[1],ent->axis[1]);
	VectorCopy(cg.refdef.viewaxis[2],ent->axis[2]);

	trap_R_AddRefEntityToScene(ent);
}
