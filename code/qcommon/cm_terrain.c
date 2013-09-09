/*
===========================================================================
Copyright (C) 2008 Leszek Godlewski

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

#include "cm_local.h"
#include "cm_terrain.h"

/*
=================
CM_SignbitsForNormal
=================
Copied over from cm_patch.c
*/
static int CM_SignbitsForNormal( vec3_t normal ) {
	int	bits, j;

	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if ( normal[j] < 0 ) {
			bits |= 1<<j;
		}
	}
	return bits;
}

/*
====================
CM_GenerateTerPatchCollide
====================
*/
struct terPatchCollide_s *CM_GenerateTerPatchCollide(vec3_t origin, byte heightmap[9][9], dshader_t *shader) {
	int					x, y, tri, i;
	vec3_t				/*farend, points[4],*/ vhmap[9][9]/*, filler*/;
	float				*points[4];
	terPatchCollide_t	*tc = Hunk_Alloc(sizeof(*tc), h_high);

	VectorSet(tc->bounds[0], 512, 512, 512);
	VectorAdd(origin, tc->bounds[0], tc->bounds[1]);
	VectorCopy(origin, tc->bounds[0]);
	tc->shader = shader;

#if 1
	// build a heightmap first
	for (tri = 0, y = 0; y < 9; y++) {
		for (x = 0; x < 9; x++, tri++) {
			vhmap[y][x][0] = origin[0] + x * 64.f;
			vhmap[y][x][1] = origin[1] + y * 64.f;
			vhmap[y][x][2] = origin[2] + heightmap[y][x] * 2;
		}
	}

	for (tri = 0, y = 0; y < TER_QUADS_PER_ROW; y++) {
		for (x = 0; x < TER_QUADS_PER_ROW; x++, tri += 2) {
			points[0] = vhmap[y][x];
			points[1] = vhmap[y + 1][x];
			points[2] = vhmap[y][x + 1];
			points[3] = vhmap[y + 1][x + 1];

			// the XOR below is explained in RB_SurfaceTerrainPatch (../renderer/tr_surface.c)
			if ((x % 2) ^ (y % 2)) {
				// 1st triangle border planes
				tc->tris[tri + 0].planes[1].normal[0] = -1.f;
				tc->tris[tri + 0].planes[1].normal[1] = 0.f;
				tc->tris[tri + 0].planes[1].normal[2] = 0.f;
				tc->tris[tri + 0].planes[1].dist = DotProduct(tc->tris[tri + 0].planes[1].normal, points[0]);

				tc->tris[tri + 0].planes[2].normal[0] = 0.f;
				tc->tris[tri + 0].planes[2].normal[1] = -1.f;
				tc->tris[tri + 0].planes[2].normal[2] = 0.f;
				tc->tris[tri + 0].planes[2].dist = DotProduct(tc->tris[tri + 0].planes[2].normal, points[0]);

				tc->tris[tri + 0].planes[3].normal[0] = M_SQRT1_2;
				tc->tris[tri + 0].planes[3].normal[1] = M_SQRT1_2;
				tc->tris[tri + 0].planes[3].normal[2] = 0.f;
				tc->tris[tri + 0].planes[3].dist = DotProduct(tc->tris[tri + 0].planes[3].normal, points[1]);

				// 2nd triangle border planes
				tc->tris[tri + 1].planes[1].normal[0] = 1.f;
				tc->tris[tri + 1].planes[1].normal[1] = 0.f;
				tc->tris[tri + 1].planes[1].normal[2] = 0.f;
				tc->tris[tri + 1].planes[1].dist = DotProduct(tc->tris[tri + 1].planes[1].normal, points[3]);

				tc->tris[tri + 1].planes[2].normal[0] = 0.f;
				tc->tris[tri + 1].planes[2].normal[1] = 1.f;
				tc->tris[tri + 1].planes[2].normal[2] = 0.f;
				tc->tris[tri + 1].planes[2].dist = DotProduct(tc->tris[tri + 1].planes[2].normal, points[3]);

				tc->tris[tri + 1].planes[3].normal[0] = -M_SQRT1_2;
				tc->tris[tri + 1].planes[3].normal[1] = -M_SQRT1_2;
				tc->tris[tri + 1].planes[3].normal[2] = 0.f;
				tc->tris[tri + 1].planes[3].dist = DotProduct(tc->tris[tri + 1].planes[3].normal, points[2]);

				// cap planes
				// HACK HACK HACK!
				// this relies on the current layout of cplane_t
				PlaneFromPoints((float *)tc->tris[tri + 0].planes[0].normal, points[0], points[1], points[2]);

				VectorNegate(tc->tris[tri + 0].planes[0].normal, tc->tris[tri + 0].planes[4].normal);
				tc->tris[tri + 0].planes[4].dist = -tc->tris[tri + 0].planes[0].dist + 0.5;

				// HACK HACK HACK!
				// this relies on the current layout of cplane_t
				PlaneFromPoints((float *)tc->tris[tri + 1].planes[0].normal, points[1], points[3], points[2]);

				VectorNegate(tc->tris[tri + 1].planes[0].normal, tc->tris[tri + 1].planes[4].normal);
				tc->tris[tri + 1].planes[4].dist = -tc->tris[tri + 1].planes[0].dist + 0.5;
			} else {
				// 1st triangle border planes
				tc->tris[tri + 0].planes[1].normal[0] = 1.f;
				tc->tris[tri + 0].planes[1].normal[1] = 0.f;
				tc->tris[tri + 0].planes[1].normal[2] = 0.f;
				tc->tris[tri + 0].planes[1].dist = DotProduct(tc->tris[tri + 0].planes[1].normal, points[2]);

				tc->tris[tri + 0].planes[2].normal[0] = 0.f;
				tc->tris[tri + 0].planes[2].normal[1] = -1.f;
				tc->tris[tri + 0].planes[2].normal[2] = 0.f;
				tc->tris[tri + 0].planes[2].dist = DotProduct(tc->tris[tri + 0].planes[2].normal, points[0]);

				tc->tris[tri + 0].planes[3].normal[0] = -M_SQRT1_2;
				tc->tris[tri + 0].planes[3].normal[1] = M_SQRT1_2;
				tc->tris[tri + 0].planes[3].normal[2] = 0.f;
				tc->tris[tri + 0].planes[3].dist = DotProduct(tc->tris[tri + 0].planes[3].normal, points[0]);

				// 2nd triangle border planes
				tc->tris[tri + 1].planes[1].normal[0] = -1.f;
				tc->tris[tri + 1].planes[1].normal[1] = 0.f;
				tc->tris[tri + 1].planes[1].normal[2] = 0.f;
				tc->tris[tri + 1].planes[1].dist = DotProduct(tc->tris[tri + 1].planes[1].normal, points[0]);

				tc->tris[tri + 1].planes[2].normal[0] = 0.f;
				tc->tris[tri + 1].planes[2].normal[1] = 1.f;
				tc->tris[tri + 1].planes[2].normal[2] = 0.f;
				tc->tris[tri + 1].planes[2].dist = DotProduct(tc->tris[tri + 1].planes[2].normal, points[1]);

				tc->tris[tri + 1].planes[3].normal[0] = M_SQRT1_2;
				tc->tris[tri + 1].planes[3].normal[1] = -M_SQRT1_2;
				tc->tris[tri + 1].planes[3].normal[2] = 0.f;
				tc->tris[tri + 1].planes[3].dist = DotProduct(tc->tris[tri + 1].planes[3].normal, points[0]);

				// cap planes
				// HACK HACK HACK!
				// this relies on the current layout of cplane_t
				PlaneFromPoints((float *)tc->tris[tri + 0].planes[0].normal, points[0], points[3], points[2]);

				VectorNegate(tc->tris[tri + 0].planes[0].normal, tc->tris[tri + 0].planes[4].normal);
				tc->tris[tri + 0].planes[4].dist = -tc->tris[tri + 0].planes[0].dist + 0.5;

				// HACK HACK HACK!
				// this relies on the current layout of cplane_t
				PlaneFromPoints((float *)tc->tris[tri + 1].planes[0].normal, points[0], points[1], points[3]);

				VectorNegate(tc->tris[tri + 1].planes[0].normal, tc->tris[tri + 1].planes[4].normal);
				tc->tris[tri + 1].planes[4].dist = -tc->tris[tri + 1].planes[0].dist + 0.5;
			}
			for (i = 0; i < TER_TRIS_PER_PATCH; i++) {
				tc->tris[tri + 0].planes[i].signbits = CM_SignbitsForNormal(tc->tris[tri + 0].planes[i].normal);
				tc->tris[tri + 0].planes[i].type = PlaneTypeForNormal(tc->tris[tri + 0].planes[i].normal);
				tc->tris[tri + 1].planes[i].signbits = CM_SignbitsForNormal(tc->tris[tri + 1].planes[i].normal);
				tc->tris[tri + 1].planes[i].type = PlaneTypeForNormal(tc->tris[tri + 1].planes[i].normal);
			}
		}
	}
#else	// development test case code, left over just in case; works with TER_QUADS_PER_ROW = 1 ONLY!!!
#if 1
#if TER_QUADS_PER_ROW != 1
	#error("TER_QUADS_PER_ROW must be equal to 1!")
#endif
	points[0][0] = origin[0];
	points[0][1] = origin[1];
	points[0][2] = origin[2] + heightmap[0][0] * 2;

	points[1][0] = origin[0] + 512.f;
	points[1][1] = origin[1];
	points[1][2] = origin[2] + heightmap[0][8] * 2;

	points[2][0] = origin[0] + 512.f;
	points[2][1] = origin[1] + 512.f;
	points[2][2] = origin[2] + heightmap[8][8] * 2;

	points[3][0] = origin[0];
	points[3][1] = origin[1] + 512.f;
	points[3][2] = origin[2] + heightmap[8][0] * 2;

	// HACK HACK HACK!
	// this relies on the current layout of cplane_t
	PlaneFromPoints((float *)tc->tris[0].planes[0].normal, points[0], points[2], points[1]);
	tc->tris[0].planes[0].signbits = CM_SignbitsForNormal(tc->tris[0].planes[0].normal);
	tc->tris[0].planes[0].type = PlaneTypeForNormal(tc->tris[0].planes[0].normal);

	/*tc->tris[0].planes[1].normal[0] = 0.f;
	tc->tris[0].planes[1].normal[1] = -1.f;
	tc->tris[0].planes[1].normal[2] = 0.f;
	tc->tris[0].planes[1].dist = DotProduct(tc->tris[0].planes[1].normal, points[0]);*/
	filler[0] = points[1][0];
	filler[1] = points[1][1];
	filler[2] = points[1][2] + 1.f;
	PlaneFromPoints((float *)tc->tris[0].planes[1].normal, points[2], points[1], filler);
	tc->tris[0].planes[1].signbits = CM_SignbitsForNormal(tc->tris[0].planes[1].normal);
	tc->tris[0].planes[1].type = PlaneTypeForNormal(tc->tris[0].planes[1].normal);

	/*tc->tris[0].planes[2].normal[0] = 1.f;
	tc->tris[0].planes[2].normal[1] = 0.f;
	tc->tris[0].planes[2].normal[2] = 0.f;
	tc->tris[0].planes[2].dist = DotProduct(tc->tris[0].planes[2].normal, points[2]);*/
	PlaneFromPoints((float *)tc->tris[0].planes[2].normal, points[1], points[0], filler);
	tc->tris[0].planes[2].signbits = CM_SignbitsForNormal(tc->tris[0].planes[2].normal);
	tc->tris[0].planes[2].type = PlaneTypeForNormal(tc->tris[0].planes[2].normal);

	/*tc->tris[0].planes[3].normal[0] = -M_SQRT1_2;
	tc->tris[0].planes[3].normal[1] = M_SQRT1_2;
	tc->tris[0].planes[3].normal[2] = 0.f;
	tc->tris[0].planes[3].dist = DotProduct(tc->tris[0].planes[3].normal, points[0]);*/
	filler[0] = points[2][0];
	filler[1] = points[2][1];
	filler[2] = points[2][2] + 1.f;
	PlaneFromPoints((float *)tc->tris[0].planes[3].normal, points[0], points[2], filler);
	tc->tris[0].planes[3].signbits = CM_SignbitsForNormal(tc->tris[0].planes[3].normal);
	tc->tris[0].planes[3].type = PlaneTypeForNormal(tc->tris[0].planes[3].normal);

	VectorNegate(tc->tris[0].planes[0].normal, tc->tris[0].planes[4].normal);
	tc->tris[0].planes[4].dist = -tc->tris[0].planes[0].dist + 16/*SURFACE_CLIP_EPSILON*/;
	tc->tris[0].planes[4].signbits = CM_SignbitsForNormal(tc->tris[0].planes[4].normal);

	// HACK HACK HACK!
	// this relies on the current layout of cplane_t
	PlaneFromPoints((float *)tc->tris[1].planes[0].normal, points[0], points[3], points[2]);
	tc->tris[1].planes[0].signbits = CM_SignbitsForNormal(tc->tris[1].planes[0].normal);
	tc->tris[1].planes[0].type = PlaneTypeForNormal(tc->tris[1].planes[0].normal);

	/*tc->tris[1].planes[1].normal[0] = -1.f;
	tc->tris[1].planes[1].normal[1] = 0.f;
	tc->tris[1].planes[1].normal[2] = 0.f;
	tc->tris[1].planes[1].dist = DotProduct(tc->tris[1].planes[1].normal, points[0]);*/
	PlaneFromPoints((float *)tc->tris[1].planes[1].normal, points[2], points[0], filler);
	tc->tris[1].planes[1].signbits = CM_SignbitsForNormal(tc->tris[1].planes[1].normal);
	tc->tris[1].planes[1].type = PlaneTypeForNormal(tc->tris[1].planes[1].normal);

	/*tc->tris[1].planes[2].normal[0] = 0.f;
	tc->tris[1].planes[2].normal[1] = 1.f;
	tc->tris[1].planes[2].normal[2] = 0.f;
	tc->tris[1].planes[2].dist = DotProduct(tc->tris[1].planes[2].normal, points[2]);*/
	PlaneFromPoints((float *)tc->tris[1].planes[2].normal, points[3], points[2], filler);
	tc->tris[1].planes[2].signbits = CM_SignbitsForNormal(tc->tris[1].planes[2].normal);
	tc->tris[1].planes[2].type = PlaneTypeForNormal(tc->tris[1].planes[2].normal);

	/*tc->tris[1].planes[3].normal[0] = M_SQRT1_2;
	tc->tris[1].planes[3].normal[1] = -M_SQRT1_2;
	tc->tris[1].planes[3].normal[2] = 0.f;
	tc->tris[1].planes[3].dist = DotProduct(tc->tris[1].planes[3].normal, points[0]);*/
	filler[0] = points[3][0];
	filler[1] = points[3][1];
	filler[2] = points[3][2] + 1.f;
	PlaneFromPoints((float *)tc->tris[1].planes[3].normal, points[0], points[3], filler);
	tc->tris[1].planes[3].signbits = CM_SignbitsForNormal(tc->tris[1].planes[3].normal);
	tc->tris[1].planes[3].type = PlaneTypeForNormal(tc->tris[1].planes[3].normal);

	VectorNegate(tc->tris[1].planes[0].normal, tc->tris[1].planes[4].normal);
	tc->tris[1].planes[4].dist = -tc->tris[1].planes[0].dist + 16/*SURFACE_CLIP_EPSILON*/;
	tc->tris[1].planes[4].signbits = CM_SignbitsForNormal(tc->tris[1].planes[4].normal);
	tc->tris[1].planes[4].type = PlaneTypeForNormal(tc->tris[1].planes[4].normal);
#else	// total kernel panic, last resort kind of code :S
	points[0][0] = origin[0] + 512.f;
	points[0][1] = origin[1] + 512.f;
	points[0][2] = origin[2] + heightmap[8][8];

	tc->tris[0].planes[0].normal[0] = 0.f;
	tc->tris[0].planes[0].normal[1] = 0.f;
	tc->tris[0].planes[0].normal[2] = 1.f;
	tc->tris[0].planes[0].dist = (origin[2] + heightmap[0][0] * 2 + origin[2] + heightmap[0][8] * 2 + origin[2] + heightmap[8][8] * 2) / 3.f;
	tc->tris[0].planes[0].signbits = CM_SignbitsForNormal(tc->tris[0].planes[0].normal);
	tc->tris[0].planes[0].type = PlaneTypeForNormal(tc->tris[0].planes[0].normal);

	tc->tris[0].planes[1].normal[0] = 1.f;
	tc->tris[0].planes[1].normal[0] = 0.f;
	tc->tris[0].planes[1].normal[2] = 0.f;
	tc->tris[0].planes[1].dist = points[0][0];
	tc->tris[0].planes[1].signbits = CM_SignbitsForNormal(tc->tris[0].planes[1].normal);
	tc->tris[0].planes[1].type = PlaneTypeForNormal(tc->tris[0].planes[1].normal);

	tc->tris[0].planes[2].normal[0] = 0.f;
	tc->tris[0].planes[2].normal[0] = -1.f;
	tc->tris[0].planes[2].normal[2] = 0.f;
	tc->tris[0].planes[2].dist = origin[0];
	tc->tris[0].planes[2].signbits = CM_SignbitsForNormal(tc->tris[0].planes[2].normal);
	tc->tris[0].planes[2].type = PlaneTypeForNormal(tc->tris[0].planes[2].normal);

	tc->tris[0].planes[3].normal[0] = -M_PI_2;
	tc->tris[0].planes[3].normal[0] = M_PI_2;
	tc->tris[0].planes[3].normal[2] = 0.f;
	tc->tris[0].planes[3].dist = origin[0];
	tc->tris[0].planes[3].signbits = CM_SignbitsForNormal(tc->tris[0].planes[3].normal);
	tc->tris[0].planes[3].type = PlaneTypeForNormal(tc->tris[0].planes[3].normal);

	VectorNegate(tc->tris[0].planes[0].normal, tc->tris[0].planes[4].normal);
	tc->tris[0].planes[4].dist = -tc->tris[0].planes[0].dist;
	tc->tris[0].planes[4].signbits = CM_SignbitsForNormal(tc->tris[0].planes[4].normal);
	tc->tris[0].planes[4].type = PlaneTypeForNormal(tc->tris[0].planes[4].normal);

	tc->tris[1].planes[0].normal[0] = 0.f;
	tc->tris[1].planes[0].normal[1] = 0.f;
	tc->tris[1].planes[0].normal[2] = 1.f;
	tc->tris[1].planes[0].dist = (origin[2] + heightmap[0][0] * 2 + origin[2] + heightmap[8][0] * 2 + origin[2] + heightmap[8][8] * 2) / 3.f;
	tc->tris[1].planes[0].signbits = CM_SignbitsForNormal(tc->tris[1].planes[0].normal);
	tc->tris[1].planes[0].type = PlaneTypeForNormal(tc->tris[1].planes[0].normal);

	tc->tris[1].planes[1].normal[0] = -1.f;
	tc->tris[1].planes[1].normal[0] = 0.f;
	tc->tris[1].planes[1].normal[2] = 0.f;
	tc->tris[1].planes[1].dist = origin[0];
	tc->tris[1].planes[1].signbits = CM_SignbitsForNormal(tc->tris[1].planes[1].normal);
	tc->tris[1].planes[1].type = PlaneTypeForNormal(tc->tris[1].planes[1].normal);

	tc->tris[1].planes[2].normal[0] = 0.f;
	tc->tris[1].planes[2].normal[0] = 1.f;
	tc->tris[1].planes[2].normal[2] = 0.f;
	tc->tris[1].planes[2].dist = points[0][1];
	tc->tris[1].planes[2].signbits = CM_SignbitsForNormal(tc->tris[1].planes[2].normal);
	tc->tris[1].planes[2].type = PlaneTypeForNormal(tc->tris[1].planes[2].normal);

	tc->tris[1].planes[3].normal[0] = M_PI_2;
	tc->tris[1].planes[3].normal[0] = -M_PI_2;
	tc->tris[1].planes[3].normal[2] = 0.f;
	tc->tris[1].planes[3].dist = origin[0];
	tc->tris[1].planes[3].signbits = CM_SignbitsForNormal(tc->tris[1].planes[3].normal);
	tc->tris[1].planes[3].type = PlaneTypeForNormal(tc->tris[1].planes[3].normal);

	VectorNegate(tc->tris[1].planes[0].normal, tc->tris[1].planes[4].normal);
	tc->tris[1].planes[4].dist = -tc->tris[1].planes[0].dist;
	tc->tris[1].planes[4].signbits = CM_SignbitsForNormal(tc->tris[1].planes[4].normal);
	tc->tris[1].planes[4].type = PlaneTypeForNormal(tc->tris[1].planes[4].normal);
#endif
#endif

	return tc;
}

/*
====================
CM_TracePointThroughTerPatchCollide
====================
*/
void CM_TracePointThroughTerPatchCollide(traceWork_t *tw, const struct terPatchCollide_s *tc) {
	int			i, j;
	float		d1, d2, f;
	vec3_t		testpoint;
	qboolean	getout, startout;

	if (!tw->isPoint)
		return;

	for (i = 0; i < TER_TRIS_PER_PATCH; i++) {
		getout = startout = qfalse;

		d1 = DotProduct(tw->start, tc->tris[i].planes[0].normal) - tc->tris[i].planes[0].dist;
		d2 = DotProduct(tw->end, tc->tris[i].planes[0].normal) - tc->tris[i].planes[0].dist;

		if (d2 > 0)
			getout = qtrue;	// endpoint is not in solid
		if (d1 > 0)
			startout = qtrue;

		// if completely in front of plane, no intersection with the entire plane
		if (d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1))
			return;

		// if it doesn't cross the plane, the plane isn't relevant
		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses plane
		if (d1 > d2) {	// enter
			f = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);
			if (f < 0)
				f = 0;
			if (f > tw->trace.fraction)	// don't even bother checking the borders
				continue;
			VectorSubtract(tw->end, tw->start, testpoint);
			VectorMA(tw->start, f, testpoint, testpoint);
			for (j = 1; j < 4; j++) {
				if (DotProduct(testpoint, tc->tris[i].planes[j].normal) - tc->tris[i].planes[j].dist > 0)
					break;	// intersection is beyond the triangle
			}
			if (j < 4)	// intersection is beyond the triangle
				continue;
			// we have a hit
			tw->trace.fraction = f;
			tw->trace.plane = tc->tris[i].planes[0];
			return;
		}
	}
}

/*
====================
CM_TraceThroughTerTriangle
====================
*/
void CM_TraceThroughTerTriangle(traceWork_t *tw, const cplane_t planes[5]) {
	int			i;
	cplane_t	*clipplane;
	float		dist;
	float		enterFrac, leaveFrac;
	float		d1, d2;
	qboolean	getout, startout;
	float		f;
	float		t;
	vec3_t		startp;
	vec3_t		endp;

	enterFrac = -1.0;
	leaveFrac = 1.0;
	clipplane = NULL;

	getout = qfalse;
	startout = qfalse;

	// NOTE
	// This function takes 4 planes that make a brush that extends infinitely into -Z.

	if (tw->sphere.use) {
		//
		// compare the trace against all planes of the brush
		// find the latest time the trace crosses a plane towards the interior
		// and the earliest time the trace crosses a plane towards the exterior
		//
		for (i = 0; i < TER_PLANES_PER_TRI; i++) {
			// adjust the plane distance apropriately for radius
			dist = planes[i].dist + tw->sphere.radius;

			// find the closest point on the capsule to the plane
			t = DotProduct(planes[i].normal, tw->sphere.offset );
			if (t > 0) {
				VectorSubtract(tw->start, tw->sphere.offset, startp);
				VectorSubtract(tw->end, tw->sphere.offset, endp);
			} else {
				VectorAdd(tw->start, tw->sphere.offset, startp);
				VectorAdd(tw->end, tw->sphere.offset, endp);
			}

			d1 = DotProduct(startp, planes[i].normal) - dist;
			d2 = DotProduct(endp, planes[i].normal) - dist;

			if (d2 > 0)
				getout = qtrue;	// endpoint is not in solid
			if (d1 > 0)
				startout = qtrue;

			// if completely in front of face, no intersection with the entire brush
			if (d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1))
				return;

			// if it doesn't cross the plane, the plane isn't relevent
			if (d1 <= 0 && d2 <= 0)
				continue;

			// crosses face
			if (d1 > d2) {	// enter
				f = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);
				if (f < 0)
					f = 0;
				if (f > enterFrac) {
					enterFrac = f;
					clipplane = (cplane_t *)&planes[i];
				}
			} else {	// leave
				f = (d1 + SURFACE_CLIP_EPSILON) / (d1 - d2);
				if (f > 1)
					f = 1;
				if (f < leaveFrac)
					leaveFrac = f;
			}
		}
	} else {
		//
		// compare the trace against all planes of the brush
		// find the latest time the trace crosses a plane towards the interior
		// and the earliest time the trace crosses a plane towards the exterior
		//
		for (i = 0; i < TER_PLANES_PER_TRI; i++) {
			// adjust the plane distance apropriately for mins/maxs
			dist = planes[i].dist - DotProduct(tw->offsets[ planes[i].signbits ], planes[i].normal);

			d1 = DotProduct(tw->start, planes[i].normal) - dist;
			d2 = DotProduct(tw->end, planes[i].normal) - dist;

			if (d2 > 0)
				getout = qtrue;	// endpoint is not in solid
			if (d1 > 0)
				startout = qtrue;

			// if completely in front of face, no intersection with the entire brush
			if (d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1))
				return;

			// if it doesn't cross the plane, the plane isn't relevent
			if (d1 <= 0 && d2 <= 0)
				continue;

			// crosses face
			if (d1 > d2) {	// enter
				f = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);
				if (f < 0)
					f = 0;
				if (f > enterFrac) {
					enterFrac = f;
					clipplane = (cplane_t *)&planes[i];
				}
			} else {	// leave
				f = (d1 + SURFACE_CLIP_EPSILON) / (d1 - d2);
				if (f > 1)
					f = 1;
				if (f < leaveFrac)
					leaveFrac = f;
			}
		}
	}

	//
	// all planes have been checked, and the trace was not
	// completely outside the brush
	//
	if (!startout) {	// original point was inside brush
		//tw->trace.startsolid = qtrue;
		if (!getout) {
			tw->trace.allsolid = qtrue;
			tw->trace.fraction = 0;
		}
		return;
	}

	if (enterFrac < leaveFrac /*&& clipplane == &planes[0]*/) {	// only the top plane can clip!
		if (enterFrac > -1 && enterFrac < tw->trace.fraction) {
			if (enterFrac < 0)
				enterFrac = 0;
			tw->trace.fraction = enterFrac;
			tw->trace.plane = /*planes[0]*/*clipplane;	// terrain ALWAYS returns its surface
		}
	}
}

/*
====================
CM_TraceThroughTerPatchCollide
====================
*/
void CM_TraceThroughTerPatchCollide(traceWork_t *tw, const struct terPatchCollide_s *tc) {
	int		i;
	float	origFrac;

	if (!(tc->shader->contentFlags & tw->contents) || !CM_BoundsIntersect(tw->bounds[0], tw->bounds[1], tc->bounds[0], tc->bounds[1]))
		return;

	c_terrain_patch_traces++;

	if (tw->isPoint) {
		origFrac = tw->trace.fraction;
		CM_TracePointThroughTerPatchCollide(tw, tc);
		if (tw->trace.fraction != origFrac) {
			tw->trace.surfaceFlags = tc->shader->surfaceFlags;
			tw->trace.contents = tc->shader->contentFlags;
		}
		return;
	}

	for (i = 0; i < TER_TRIS_PER_PATCH; i++) {
		origFrac = tw->trace.fraction;
		CM_TraceThroughTerTriangle(tw, tc->tris[i].planes);
		if (tw->trace.fraction != origFrac) {
			tw->trace.surfaceFlags = tc->shader->surfaceFlags;
			tw->trace.contents = tc->shader->contentFlags;
		}
		if (!tw->trace.fraction)
			return;
	}
}

/*
====================
CM_PositionTestInTerPatchCollide
====================
*/
void CM_PositionTestInTerPatchCollide(traceWork_t *tw, const struct terPatchCollide_s *tc) {
	int			i, j;
	float		dist;
	float		d1;
	float		t;
	vec3_t		startp;

	if (!(tc->shader->contentFlags & tw->contents) || tw->isPoint)	// terrain has no volume
		return;

	for (i = 0; i < TER_TRIS_PER_PATCH; i++) {
		if (tw->sphere.use) {
			for (j = 0; j < TER_PLANES_PER_TRI; j++) {
				// adjust the plane distance apropriately for radius
				dist = tc->tris[i].planes[j].dist + tw->sphere.radius;
				// find the closest point on the capsule to the plane
				t = DotProduct(tc->tris[i].planes[j].normal, tw->sphere.offset);
				if (t > 0)
					VectorSubtract(tw->start, tw->sphere.offset, startp);
				else
					VectorAdd(tw->start, tw->sphere.offset, startp);
				d1 = DotProduct(startp, tc->tris[i].planes[j].normal) - dist;
				// if completely in front of face, no intersection
				if (d1 > 0)
					return;
			}
		} else {
			for (j = 0; j < TER_PLANES_PER_TRI; j++) {
				// adjust the plane distance apropriately for mins/maxs
				dist = tc->tris[i].planes[j].dist - DotProduct(tw->offsets[ tc->tris[i].planes[j].signbits ], tc->tris[i].planes[j].normal);

				d1 = DotProduct(tw->start, tc->tris[i].planes[j].normal) - dist;

				// if completely in front of face, no intersection
				if (d1 > 0)
					return;
			}
		}

		// inside this terrain patch
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0;
		tw->trace.contents = tc->shader->contentFlags;
		return;
	}
}
