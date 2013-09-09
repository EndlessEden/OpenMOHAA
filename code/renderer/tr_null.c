/*
===========================================================================
Copyright (C) 2012 su44

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
#include "../qcommon/q_shared.h"
#include "tr_public.h"

refimport_t ri;

typedef struct image_s image_t;

void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {

}

void RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {

}

void RE_BeginFrame( stereoFrame_t stereoFrame ) {

}

void RE_BeginRegistration( glconfig_t *glconfig ) {

}

void RE_LoadWorldMap( const char *mapname ) {

}

void RE_SetWorldVisData( const byte *vis ) {

}

qhandle_t RE_RegisterModel( const char *name ) {
	return 0;
}

qhandle_t RE_RegisterSkin( const char *name ) {
	return 0;
}

void RE_Shutdown( qboolean destroyWindow ) {

}

qhandle_t	RE_RegisterShaderLightMap( const char *name, int lightmapIndex ) {
	return 0;
}

qhandle_t RE_RegisterShader( const char *name ) {
	return 0;
}

qhandle_t RE_RegisterShaderNoMip( const char *name ) {
	return 0;
}

qhandle_t RE_RegisterShaderFromImage(const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage) {
	return 0;
}

int	RE_GetShaderWidth(qhandle_t shader) {
	return 512;
}

int	RE_GetShaderHeight(qhandle_t shader) {
	return 512;
}

void RE_EndRegistration( void ) {

}

int R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
					int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer ) {
	return 0;
}
int	R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame,
					 float frac, const char *tagName ) {
	return 0;
}
void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs ) {

}
void RE_ClearScene( void ) {

}
void RE_AddRefEntityToScene( const refEntity_t *ent ) {

}
void RE_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num ) {

}
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {

}
void RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {

}
void RE_RenderScene( const refdef_t *fd ) {

}
void RE_EndFrame( int *frontEndMsec, int *backEndMsec ) {

}
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir ) {
	return 0;
}
void RE_SetColor( const float *rgba ) {

}
void RE_StretchPic ( float x, float y, float w, float h,
					  float s1, float t1, float s2, float t2, qhandle_t hShader ) {

}
void RE_RotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle ) {

}
void RE_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font) {

}
// IneQuation: string drawing, wombat
int RE_Text_Width(fontInfo_t *font, const char *text, int limit, qboolean useColourCodes) {
	return 16;
}
int RE_Text_Height(fontInfo_t *font, const char *text, int limit, qboolean useColourCodes) {
	return 16;
}
void RE_Text_Paint(fontInfo_t *font, float x, float y, float scale, float alpha, const char *text, float adjust, int limit, qboolean useColourCodes, qboolean is640) {

}
void RE_Text_PaintChar(fontInfo_t *font, float x, float y, float scale, int c, qboolean is640) {

}
void R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset) {

}
qboolean R_GetEntityToken( char *buffer, int size ) {
	return 0;
}
void RE_TakeVideoFrame( int width, int height,
		byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg ) {

}
qboolean R_inPVS( const vec3_t p1, const vec3_t p2 ) {
	return 0;
}

refexport_t *GetRefAPI ( int apiVersion, refimport_t *rimp ) {
	static refexport_t	re;

	ri = *rimp;

	Com_Memset( &re, 0, sizeof( re ) );

	//if ( apiVersion != REF_API_VERSION ) {
	//	ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n",
	//		REF_API_VERSION, apiVersion );
	//	return NULL;
	//}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel = RE_RegisterModel;
	re.RegisterSkin = RE_RegisterSkin;
	re.RegisterShader = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.LoadWorld = RE_LoadWorldMap;
	re.SetWorldVisData = RE_SetWorldVisData;
	re.EndRegistration = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.ClearScene = RE_ClearScene;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;
	re.AddPolyToScene = RE_AddPolyToScene;
	re.LightForPoint = R_LightForPoint;
	re.AddLightToScene = RE_AddLightToScene;
	re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
	re.RenderScene = RE_RenderScene;

	re.SetColor = RE_SetColor;
	re.DrawStretchPic = RE_StretchPic;
	re.RotatedPic = RE_RotatedPic;
	re.DrawStretchRaw = RE_StretchRaw;
	re.UploadCinematic = RE_UploadCinematic;

	re.RegisterFont = RE_RegisterFont;
	re.RemapShader = R_RemapShader;
	re.GetEntityToken = R_GetEntityToken;
	re.inPVS = R_inPVS;

	re.TakeVideoFrame = RE_TakeVideoFrame;

	// IneQuation
	re.Text_Width = RE_Text_Width;
	re.Text_Height = RE_Text_Height;
	re.Text_Paint = RE_Text_Paint;
	re.Text_PaintChar = RE_Text_PaintChar;

	// su44
	re.GetShaderHeight = RE_GetShaderHeight;
	re.GetShaderWidth = RE_GetShaderWidth;

	return &re;
}