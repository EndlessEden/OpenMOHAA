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
//
#ifndef __TR_TYPES_H
#define __TR_TYPES_H


#define	MAX_DLIGHTS		32			// can't be increased, because bit flags are used on surfaces
#define	MAX_ENTITIES	1023		// can't be increased without changing drawsurf bit packing

// su44: renderfx flags are now in q_shared.h (as in MoHAA)
// because we need them in both cgame and game modules...

// refdef flags
#define RDF_NOWORLDMODEL	1		// used for player configuration screen
#define RDF_HYPERSPACE		4		// teleportation effect
// ET-XreaL BEGIN
#define RDF_SKYBOXPORTAL	8
#define RDF_NOCUBEMAP       16		// RB: don't use cubemaps
#define RDF_NOBLOOM         32		// RB: disable bloom. useful for hud models

// ET-XreaL END

typedef struct {
	vec3_t		xyz;
	float		st[2];
	byte		modulate[4];
} polyVert_t;

typedef struct poly_s {
	qhandle_t			hShader;
	int					numVerts;
	polyVert_t			*verts;
} poly_t;

typedef enum {
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE,		// doesn't draw anything, just info for portals
	RT_SPLASH,
	RT_MAX_REF_ENTITY_TYPE
} refEntityType_t;

typedef struct {
	refEntityType_t	reType;
	int			renderfx;

	qhandle_t	hModel;				// opaque type outside refresh

	// most recent data
	vec3_t		lightingOrigin;		// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float		shadowPlane;		// projection shadows go here, stencils go slightly lower

	vec3_t		axis[3];			// rotation vectors
	qboolean	nonNormalizedAxes;	// axis are not normalized, i.e. they have scale
	float		origin[3];			// also used as MODEL_BEAM's "from"
	int			frame;				// also used as MODEL_BEAM's diameter

	// previous data for frame interpolation
	float		oldorigin[3];		// also used as MODEL_BEAM's "to"
	int			oldframe;
	float		backlerp;			// 0.0 = current, 1.0 = old

	// texturing
	int			skinNum;			// inline skin index
	qhandle_t	customSkin;			// NULL for default skin
	qhandle_t	customShader;		// use one image for the entire thing

	// misc
	byte		shaderRGBA[4];		// colors used by rgbgen entity shaders
	float		shaderTexCoord[2];	// texture coordinates used by tcMod entity modifiers
	float		shaderTime;			// subtracted from refdef time to control effect start times

	// extra sprite information
	float		radius;
	float		rotation;

	// su44: added for MoHAA .tik models
	bone_t		*bones;
	int			surfaceBits;
	vec3_t		bounds[2];

	int			staticModelIndex;
	float		scale;
} refEntity_t;


// XreaL BEGIN

typedef enum
{
	RL_OMNI,			// point light
	RL_PROJ,			// spot light
	RL_DIRECTIONAL,		// sun light

	RL_MAX_REF_LIGHT_TYPE
} refLightType_t;

typedef struct
{
	refLightType_t  rlType;
//  int             lightfx;

	qhandle_t       attenuationShader;

	vec3_t          origin;
	quat_t          rotation;
	vec3_t          center;
	vec3_t          color;		// range from 0.0 to 1.0, should be color normalized

	float			scale;		// r_lightScale if not set

	// omni-directional light specific
	vec3_t          radius;

	// projective light specific
	vec3_t			projTarget;
	vec3_t			projRight;
	vec3_t			projUp;
	vec3_t			projStart;
	vec3_t			projEnd;

	qboolean        noShadows;
	short           noShadowID;	// don't cast shadows of all entities with this id

	qboolean        inverseShadows;	// don't cast light and draw shadows by darken the scene
	// this is useful for drawing player shadows with shadow mapping
} refLight_t;

// XreaL END

#define	MAX_RENDER_STRINGS			8
#define	MAX_RENDER_STRING_LENGTH	32

typedef struct {
	int			x, y, width, height;
	float		fov_x, fov_y;
	vec3_t		vieworg;
	vec3_t		viewaxis[3];		// transformation matrix

	// time in milliseconds for shader effects and other time dependent rendering issues
	int			time;

	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];

	// su44: MoHAA fog
	float		farplane_distance;
	float		farplane_color[3];
	qboolean	farplane_cull;

	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
} refdef_t;


typedef enum {
	STEREO_CENTER,
	STEREO_LEFT,
	STEREO_RIGHT
} stereoFrame_t;

// XreaL BEGIN

// cg_shadows modes
typedef enum
{
	SHADOWING_NONE,
	SHADOWING_BLOB,
	SHADOWING_ESM16,
	SHADOWING_ESM32,
	SHADOWING_VSM16,
	SHADOWING_VSM32,
	SHADOWING_EVSM32,
} shadowingMode_t;
// XreaL END

/*
** glconfig_t
**
** Contains variables specific to the OpenGL configuration
** being run right now.  These are constant once the OpenGL
** subsystem is initialized.
*/
typedef enum {
	TC_NONE,
	TC_S3TC
} textureCompression_t;

typedef enum {
	GLDRV_ICD,					// driver is integrated with window system
								// WARNING: there are tests that check for
								// > GLDRV_ICD for minidriverness, so this
								// should always be the lowest value in this
								// enum set
	GLDRV_STANDALONE,			// driver is a non-3Dfx standalone driver
	GLDRV_VOODOO,				// driver is a 3Dfx standalone driver
// XreaL BEGIN
	GLDRV_DEFAULT,				// old OpenGL system
	GLDRV_OPENGL3,				// new driver system
	GLDRV_MESA,					// crap
// XreaL END
} glDriverType_t;

typedef enum {
	GLHW_GENERIC,			// where everthing works the way it should
	GLHW_3DFX_2D3D,			// Voodoo Banshee or Voodoo3, relevant since if this is
							// the hardware type then there can NOT exist a secondary
							// display adapter
	GLHW_RIVA128,			// where you can't interpolate alpha
	GLHW_RAGEPRO,			// where you can't modulate alpha on alpha textures
	GLHW_PERMEDIA2,			// where you don't have src*dst
// XreaL BEGIN
	GLHW_ATI,					// where you don't have proper GLSL support
	GLHW_ATI_DX10,				// ATI Radeon HD series DX10 hardware
	GLHW_NV_DX10				// Geforce 8/9 class DX10 hardware
// XreaL END
} glHardwareType_t;

typedef struct {
	char					renderer_string[MAX_STRING_CHARS];
	char					vendor_string[MAX_STRING_CHARS];
	char					version_string[MAX_STRING_CHARS];
	char					extensions_string[BIG_INFO_STRING];

	int						maxTextureSize;			// queried from GL
	int						numTextureUnits;		// multitexture ability

	int						colorBits, depthBits, stencilBits;

	glDriverType_t			driverType;
	glHardwareType_t		hardwareType;

	qboolean				deviceSupportsGamma;
	textureCompression_t	textureCompression;
	qboolean				textureEnvAddAvailable;

	int						vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float					windowAspect;

	int						displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Voodoo or Voodoo2 will have this set to TRUE, as will a Win32 ICD that
	// used CDS.
	qboolean				isFullscreen;
	qboolean				stereoEnabled;
	qboolean				smpActive;		// dual processor
} glconfig_t;


// XreaL BEGIN
typedef struct
{
	qboolean		ARBTextureCompressionAvailable;

	int             maxCubeMapTextureSize;

	qboolean        occlusionQueryAvailable;
	int             occlusionQueryBits;

	char            shadingLanguageVersion[MAX_STRING_CHARS];

	int             maxVertexUniforms;
	int             maxVaryingFloats;
	int             maxVertexAttribs;
	qboolean        vboVertexSkinningAvailable;
	int				maxVertexSkinningBones;

	qboolean		texture3DAvailable;
	qboolean        textureNPOTAvailable;

	qboolean        drawBuffersAvailable;
	qboolean		textureHalfFloatAvailable;
	qboolean        textureFloatAvailable;
	int             maxDrawBuffers;

	qboolean        vertexArrayObjectAvailable;

	qboolean        stencilWrapAvailable;

	float           maxTextureAnisotropy;
	qboolean        textureAnisotropyAvailable;

	qboolean        framebufferObjectAvailable;
	int             maxRenderbufferSize;
	int             maxColorAttachments;
	qboolean        framebufferPackedDepthStencilAvailable;
	qboolean        framebufferBlitAvailable;

	qboolean        generateMipmapAvailable;
} glconfig2_t;
// XreaL END


// =========================================
// Gordon, these MUST NOT exceed the values for SHADER_MAX_VERTEXES/SHADER_MAX_INDEXES
#define MAX_PB_VERTS    1025
#define MAX_PB_INDICIES ( MAX_PB_VERTS * 6 )

typedef struct polyBuffer_s
{
	vec4_t          xyz[MAX_PB_VERTS];
	vec2_t          st[MAX_PB_VERTS];
	byte            color[MAX_PB_VERTS][4];
	int             numVerts;

	int             indicies[MAX_PB_INDICIES];
	int             numIndicies;

	qhandle_t       shader;
} polyBuffer_t;

// =========================================

#endif	// __TR_TYPES_H
