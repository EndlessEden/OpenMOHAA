/*
===========================================================================
Copyright (C) 2011 su44

This file is part of OpenMohaa source code.

OpenMohaa source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenMohaa source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "ui_local.h"
#include "../renderer/tr_public.h"

void UI_RenderModel(uiResource_t *res) {
	refdef_t		refdef;
	refEntity_t		ent;
	float x,y,w,h;
	float xx,len;
	tiki_t *tiki;
	int animIndex;
	vec3_t origin;

	tiki = trap_TIKI_RegisterModel(res->linkcvar.string);
	animIndex = TIKI_GetAnimIndex(tiki,res->modelanim);
	if(animIndex==-1) {
		animIndex = TIKI_GetAnimIndex(tiki,va("%s0",res->modelanim));
	}
	if(animIndex==-1) {
		animIndex = 0;
	}
	x = res->rect[0];
	y = res->rect[1];
	w = res->rect[2];
	h = res->rect[3];

	UI_AdjustFrom640( &x, &y, &w, &h );


	memset( &refdef, 0, sizeof( refdef ) );


	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.fov_x = (int)((float)refdef.width / 640.0f * 90.0f);
	xx = refdef.width / tan( refdef.fov_x / 360 * M_PI );
	refdef.fov_y = atan2( refdef.height, xx );
	refdef.fov_y *= ( 360 / M_PI );

	memset( &ent, 0, sizeof(ent) );
	AxisClear(ent.axis);
	ent.hModel = trap_R_RegisterModel(tiki->name);
	ent.bones = trap_TIKI_GetBones(tiki->numBones);
	ClearBounds(ent.bounds[0],ent.bounds[1]);
	ent.radius = 0;
	trap_TIKI_AppendFrameBoundsAndRadius(tiki,animIndex,0,&ent.radius,ent.bounds);
	trap_TIKI_SetChannels(tiki,animIndex,0,1,ent.bones);
	trap_TIKI_Animate(tiki,ent.bones);

#if 0
	{
		vec3_t			mins = {-16, -16, 0};
		vec3_t			maxs = {16, 16, 96};

		len = 0.7 * ( maxs[2] - mins[2] );		
		origin[0] = len / tan( DEG2RAD(refdef.fov_x) * 0.5 );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );
		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		VectorCopy(origin,ent.origin);
	}
#else
	{
		vec3_t			mins = {-16, -16, -10};
		vec3_t			maxs = {16, 16, 54};

		len = 0.7 * ( maxs[2] - mins[2] );		
		origin[0] = len / tan( DEG2RAD(refdef.fov_x) * 0.5 );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );
		origin[2] = -( mins[2] + maxs[2] );
		VectorCopy(origin,ent.origin);
	}
	// add modeloffset
	VectorAdd(ent.origin,res->modeloffset,ent.origin);
#endif
	AnglesToAxis(res->modelangles,ent.axis);

	refdef.time = 0;

	trap_R_ClearScene();




	trap_R_AddRefEntityToScene( &ent );

	trap_R_RenderScene( &refdef );
}
