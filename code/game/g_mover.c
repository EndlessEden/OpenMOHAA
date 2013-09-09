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

#include "g_local.h"



/*
===============================================================================

PUSHMOVE

===============================================================================
*/

void MatchTeam( gentity_t *teamLeader, int moverState, int time );

typedef struct {
	gentity_t	*ent;
	vec3_t	origin;
	vec3_t	angles;
	float	deltayaw;
} pushed_t;
pushed_t	pushed[MAX_GENTITIES], *pushed_p;


/*
============
G_TestEntityPosition

============
*/
gentity_t	*G_TestEntityPosition( gentity_t *ent ) {
	trace_t	tr;
	int		mask;

	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_SOLID;
	}
	if ( ent->client ) {
		trap_Trace( &tr, ent->client->ps.origin, ent->r.mins, ent->r.maxs, ent->client->ps.origin, ent->s.number, mask );
	} else {
		trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, ent->s.origin, ent->s.number, mask );
	}

	if (tr.startsolid)
		return &g_entities[ tr.entityNum ];

	return NULL;
}

/*
================
G_CreateRotationMatrix
================
*/
void G_CreateRotationMatrix(vec3_t angles, vec3_t matrix[3]) {
	AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
	VectorInverse(matrix[1]);
}

/*
================
G_TransposeMatrix
================
*/
void G_TransposeMatrix(vec3_t matrix[3], vec3_t transpose[3]) {
	int i, j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			transpose[i][j] = matrix[j][i];
		}
	}
}

/*
================
G_RotatePoint
================
*/
void G_RotatePoint(vec3_t point, vec3_t matrix[3]) {
	vec3_t tvec;

	VectorCopy(point, tvec);
	point[0] = DotProduct(matrix[0], tvec);
	point[1] = DotProduct(matrix[1], tvec);
	point[2] = DotProduct(matrix[2], tvec);
}

/*
==================
G_TryPushingEntity

Returns qfalse if the move is blocked
==================
*/
qboolean	G_TryPushingEntity( gentity_t *check, gentity_t *pusher, vec3_t move, vec3_t amove ) {

	return qfalse;
}

/*
==================
G_CheckProxMinePosition
==================
*/
qboolean G_CheckProxMinePosition( gentity_t *check ) {
	vec3_t		start, end;
	trace_t	tr;

	VectorMA(check->s.origin, 0.125, check->movedir, start);
	VectorMA(check->s.origin, 2, check->movedir, end);
	trap_Trace( &tr, start, NULL, NULL, end, check->s.number, MASK_SOLID );

	if (tr.startsolid || tr.fraction < 1)
		return qfalse;

	return qtrue;
}

/*
==================
G_TryPushingProxMine
==================
*/
qboolean G_TryPushingProxMine( gentity_t *check, gentity_t *pusher, vec3_t move, vec3_t amove ) {
	vec3_t		forward, right, up;
	vec3_t		org, org2, move2;
	int ret;

	// we need this for pushing things later
	VectorSubtract (vec3_origin, amove, org);
	AngleVectors (org, forward, right, up);

	// try moving the contacted entity
	VectorAdd (check->s.origin, move, check->s.origin);

	// figure movement due to the pusher's amove
	VectorSubtract (check->s.origin, pusher->r.currentOrigin, org);
	org2[0] = DotProduct (org, forward);
	org2[1] = -DotProduct (org, right);
	org2[2] = DotProduct (org, up);
	VectorSubtract (org2, org, move2);
	VectorAdd (check->s.origin, move2, check->s.origin);

	ret = G_CheckProxMinePosition( check );
	if (ret) {
		VectorCopy( check->s.origin, check->r.currentOrigin );
		trap_LinkEntity (check);
	}
	return ret;
}

void G_ExplodeMissile( gentity_t *ent );

/*
============
G_MoverPush

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If qfalse is returned, *obstacle will be the blocking entity
============
*/
qboolean G_MoverPush( gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle ) {
	int			i, e;
	gentity_t	*check;
	vec3_t		mins, maxs;
	pushed_t	*p;
	int			entityList[MAX_GENTITIES];
	int			listedEntities;
	vec3_t		totalMins, totalMaxs;

	*obstacle = NULL;


	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if ( pusher->r.currentAngles[0] || pusher->r.currentAngles[1] || pusher->r.currentAngles[2]
		|| amove[0] || amove[1] || amove[2] ) {
		float		radius;

		radius = RadiusFromBounds( pusher->r.mins, pusher->r.maxs );
		for ( i = 0 ; i < 3 ; i++ ) {
			mins[i] = pusher->r.currentOrigin[i] + move[i] - radius;
			maxs[i] = pusher->r.currentOrigin[i] + move[i] + radius;
			totalMins[i] = mins[i] - move[i];
			totalMaxs[i] = maxs[i] - move[i];
		}
	} else {
		for (i=0 ; i<3 ; i++) {
			mins[i] = pusher->r.absmin[i] + move[i];
			maxs[i] = pusher->r.absmax[i] + move[i];
		}

		VectorCopy( pusher->r.absmin, totalMins );
		VectorCopy( pusher->r.absmax, totalMaxs );
		for (i=0 ; i<3 ; i++) {
			if ( move[i] > 0 ) {
				totalMaxs[i] += move[i];
			} else {
				totalMins[i] += move[i];
			}
		}
	}

	// unlink the pusher so we don't get it in the entityList
	trap_UnlinkEntity( pusher );

	listedEntities = trap_EntitiesInBox( totalMins, totalMaxs, entityList, MAX_GENTITIES );

	// move the pusher to it's final position
	VectorAdd( pusher->r.currentOrigin, move, pusher->r.currentOrigin );
	VectorAdd( pusher->r.currentAngles, amove, pusher->r.currentAngles );
	trap_LinkEntity( pusher );

	// see if any solid entities are inside the final position
	for ( e = 0 ; e < listedEntities ; e++ ) {
		check = &g_entities[ entityList[ e ] ];

#ifdef MISSIONPACK
		if ( check->s.eType == ET_MISSILE ) {
			// if it is a prox mine
			if ( !strcmp(check->classname, "prox mine") ) {
				// if this prox mine is attached to this mover try to move it with the pusher
				if ( check->enemy == pusher ) {
					if (!G_TryPushingProxMine( check, pusher, move, amove )) {
						//explode
						check->s.loopSound = 0;
						G_AddEvent( check, EV_PROXIMITY_MINE_TRIGGER, 0 );
						G_ExplodeMissile(check);
						if (check->activator) {
							G_FreeEntity(check->activator);
							check->activator = NULL;
						}
						//G_Printf("prox mine explodes\n");
					}
				}
				else {
					//check if the prox mine is crushed by the mover
					if (!G_CheckProxMinePosition( check )) {
						//explode
						check->s.loopSound = 0;
						G_AddEvent( check, EV_PROXIMITY_MINE_TRIGGER, 0 );
						G_ExplodeMissile(check);
						if (check->activator) {
							G_FreeEntity(check->activator);
							check->activator = NULL;
						}
						//G_Printf("prox mine explodes\n");
					}
				}
				continue;
			}
		}
#endif
		// only push items and players
		if ( check->s.eType != ET_ITEM && check->s.eType != ET_PLAYER && !check->physicsObject ) {
			continue;
		}

		// if the entity is standing on the pusher, it will definitely be moved
		if ( check->s.groundEntityNum != pusher->s.number ) {
			// see if the ent needs to be tested
			if ( check->r.absmin[0] >= maxs[0]
			|| check->r.absmin[1] >= maxs[1]
			|| check->r.absmin[2] >= maxs[2]
			|| check->r.absmax[0] <= mins[0]
			|| check->r.absmax[1] <= mins[1]
			|| check->r.absmax[2] <= mins[2] ) {
				continue;
			}
			// see if the ent's bbox is inside the pusher's final position
			// this does allow a fast moving object to pass through a thin entity...
			if (!G_TestEntityPosition (check)) {
				continue;
			}
		}

		// the entity needs to be pushed
		if ( G_TryPushingEntity( check, pusher, move, amove ) ) {
			continue;
		}

		// the move was blocked an entity

		//// bobbing entities are instant-kill and never get blocked
		//if ( pusher->s_pos.trType == TR_SINE || pusher->s_apos.trType == TR_SINE ) {
		//	G_Damage( check, pusher, pusher, NULL, NULL, 99999, 0, MOD_CRUSH );
		//	continue;
		//}


		// save off the obstacle so we can call the block function (crush, etc)
		*obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for ( p=pushed_p-1 ; p>=pushed ; p-- ) {
			VectorCopy (p->origin, p->ent->s.origin);
			VectorCopy (p->angles, p->ent->s.angles);
			if ( p->ent->client ) {
				p->ent->client->ps.delta_angles[YAW] = p->deltayaw;
				VectorCopy (p->origin, p->ent->client->ps.origin);
			}
			trap_LinkEntity (p->ent);
		}
		return qfalse;
	}

	return qtrue;
}


/*
=================
G_MoverTeam
=================
*/
void G_MoverTeam( gentity_t *ent ) {
		vec3_t		move, amove;
	gentity_t	*part, *obstacle;
	vec3_t		origin, angles;
	qboolean	apos;

	obstacle = NULL;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;
	for (part = ent ; part ; part=part->teamchain) {
		// get current position
		BG_EvaluateTrajectory( &part->s_pos, level.time, origin );
		BG_EvaluateTrajectory( &part->s_apos, level.time, angles );
		VectorSubtract( origin, part->r.currentOrigin, move );
		VectorSubtract( angles, part->r.currentAngles, amove );
		if ( !G_MoverPush( part, move, amove, &obstacle ) ) {
			break;	// move was blocked
		}
	}

	if (part) {
		// go back to the previous position
		for ( part = ent ; part ; part = part->teamchain ) {
			part->s_pos.trTime += level.time - level.previousTime;
			part->s_apos.trTime += level.time - level.previousTime;
			BG_EvaluateTrajectory( &part->s_pos, level.time, part->r.currentOrigin );
			BG_EvaluateTrajectory( &part->s_apos, level.time, part->r.currentAngles );
			trap_LinkEntity( part );
		}

		// if the pusher has a "blocked" function, call it
		if (ent->blocked) {
			ent->blocked( ent, obstacle );
		}
		return;
	}

	// the move succeeded
	apos = !Q_stricmp(ent->classname, "func_rotatingdoor");	// IneQuation
	for ( part = ent ; part ; part = part->teamchain ) {
		// call the reached function if time is at or past end point
		if ( (apos ? part->s_apos.trType : part->s_pos.trType) == TR_LINEAR_STOP ) {
			if ( level.time >= (apos
				? part->s_apos.trTime + part->s_apos.trDuration
				: part->s_pos.trTime + part->s_pos.trDuration) ) {
				if ( part->reached ) {
					part->reached( part );
				}
			}
		}
	}

}

/*
================
G_RunMover

================
*/
void G_RunMover( gentity_t *ent ) {
	// if not a team captain, don't do anything, because
	// the captain will handle everything
	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}

	// if stationary at one of the positions, don't move anything
	if ( ent->s_pos.trType != TR_STATIONARY || ent->s_apos.trType != TR_STATIONARY ) {
		G_MoverTeam( ent );
	}

	// check think function
	G_RunThink( ent );
	BG_EvaluateTrajectory(&ent->s_pos,level.time,ent->s.origin);
	BG_EvaluateTrajectory(&ent->s_apos,level.time,ent->s.angles);

}

/*
============================================================================

GENERAL MOVERS

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"
============================================================================
*/

/*
===============
SetMoverState
===============
*/
void SetMoverState( gentity_t *ent, moverState_t moverState, int time ) {
	vec3_t			delta;
	float			f;
	trajectory_t	*tr;
	float			*result;

	// IneQuation: rotating door support
	if (!Q_stricmp(ent->classname, "func_rotatingdoor")) {
		tr = &ent->s_apos;
		result = ent->r.currentAngles;
	} else {
		tr = &ent->s_pos;
		result = ent->r.currentOrigin;
	}

	ent->moverState = moverState;

	tr->trTime = time;
	switch( moverState ) {
	case MOVER_POS1:
		VectorCopy( ent->pos1, tr->trBase );
		tr->trType = TR_STATIONARY;
		break;
	case MOVER_POS2:
		VectorCopy( ent->pos2, tr->trBase );
		tr->trType = TR_STATIONARY;
		break;
	case MOVER_1TO2:
		VectorCopy( ent->pos1, tr->trBase );
		VectorSubtract( ent->pos2, ent->pos1, delta );
		f = 1000.0 / tr->trDuration;
		VectorScale( delta, f, tr->trDelta );
		tr->trType = TR_LINEAR_STOP;
		break;
	case MOVER_2TO1:
		VectorCopy( ent->pos2, tr->trBase );
		VectorSubtract( ent->pos1, ent->pos2, delta );
		f = 1000.0 / tr->trDuration;
		VectorScale( delta, f, tr->trDelta );
		tr->trType = TR_LINEAR_STOP;
		break;
	}
	BG_EvaluateTrajectory( tr, level.time, result );
	trap_LinkEntity( ent );
}

/*
================
MatchTeam

All entities in a mover team will move from pos1 to pos2
in the same amount of time
================
*/
void MatchTeam( gentity_t *teamLeader, int moverState, int time ) {
	gentity_t		*slave;

	for ( slave = teamLeader ; slave ; slave = slave->teamchain ) {
		SetMoverState( slave, moverState, time );
	}
}



/*
================
ReturnToPos1
================
*/
void ReturnToPos1( gentity_t *ent ) {
	MatchTeam( ent, MOVER_2TO1, level.time );

	// looping sound
	ent->s.loopSound = ent->soundLoop;

	// starting sound
	//if ( ent->sound2to1 ) {
	//	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
	//}
}


/*
================
Reached_BinaryMover
================
*/
void Reached_BinaryMover( gentity_t *ent ) {
	// stop the looping sound
	ent->s.loopSound = ent->soundLoop;

	if ( ent->moverState == MOVER_1TO2 ) {
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		// play sound
		//if ( ent->soundPos2 ) {
		//	G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
		//}

		// return to pos1 after a delay
		ent->think = ReturnToPos1;
		ent->nextthink = level.time + ent->wait;

		// fire targets
		if ( !ent->activator ) {
			ent->activator = ent;
		}
		G_UseTargets( ent, ent->activator );
	} else if ( ent->moverState == MOVER_2TO1 ) {
		// reached pos1
		SetMoverState( ent, MOVER_POS1, level.time );

		// play sound
		//if ( ent->soundPos1 ) {
		//	G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );
		//}

		// close areaportals
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qfalse );
		}
	} else {
		G_Error( "Reached_BinaryMover: bad moverState" );
	}

}


/*
================
Use_BinaryMover
================
*/
void Use_BinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	int				total;
	int				partial;
	trajectory_t	*tr;

	// only the master should be used
	if ( ent->flags & FL_TEAMSLAVE ) {
		Use_BinaryMover( ent->teammaster, other, activator );
		return;
	}

	if (!Q_stricmp(ent->classname, "func_rotatingdoor"))
		tr = &ent->s_apos;
	else
		tr = &ent->s_pos;

	ent->activator = activator;

	if ( ent->moverState == MOVER_POS1 ) {
		// IneQuation: if this is a rotating door, check if we need to inverse
		// its pos2 to switch the opening direction
		// ent->movedir is door plane normal, ent->physicsBounce is the plane's distance from (0 0 0)
		if (!Q_stricmp(ent->classname, "func_rotatingdoor")) {
			float dot = DotProduct(activator->r.currentOrigin, ent->movedir) + ent->physicsBounce;
			if ((dot > 0 && ent->splashMethodOfDeath != 0) || (dot < 0 && ent->splashMethodOfDeath != 1)) {
				VectorInverse(ent->pos2);
				ent->splashMethodOfDeath = !ent->splashMethodOfDeath;
			}
		}

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_1TO2, level.time + 50 );

		// starting sound
//		if ( ent->sound1to2 ) {
//			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
//		}

		// looping sound
		ent->s.loopSound = ent->soundLoop;

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qtrue );
		}
		return;
	}

	// if all the way up, just delay before coming down
	if ( ent->moverState == MOVER_POS2 ) {
		//ent->nextthink = level.time + ent->wait;
		// IneQuation: WTF? close, without waiting
		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam(ent, MOVER_2TO1, level.time/* + 50 */);	// IneQuation: this delay makes it look glitchy

		// switch off the thinker so that the door doesn't try closing again
		ent->nextthink = level.time - 100;

		// starting sound
		//if (ent->sound2to1)
		//	G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound2to1);

		// looping sound
		ent->s.loopSound = ent->soundLoop;
		return;
	}

	// only partway down before reversing
	if ( ent->moverState == MOVER_2TO1 ) {
		total = tr->trDuration;
		partial = level.time - tr->trTime;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_1TO2, level.time - ( total - partial ) );

//		if ( ent->sound1to2 ) {
//			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
//		}
		return;
	}

	// only partway up before reversing
	if ( ent->moverState == MOVER_1TO2 ) {
		total = tr->trDuration;
		partial = level.time - tr->trTime;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_2TO1, level.time - ( total - partial ) );

		//if ( ent->sound2to1 ) {
		//	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
		//}
		return;
	}

}



/*
================
InitMover

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
void InitMover( gentity_t *ent ) {
	vec3_t		move;
	float		distance;
	float		light;
	vec3_t		color;
	qboolean	lightSet, colorSet;
	char		*sound;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	//if ( ent->model2 ) {
	//	ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	//}
	///////////ent->s.modelindex = G_ModelIndex(ent->model);

	// if the "loopsound" key is set, use a constant looping sound when moving
	if ( G_SpawnString( "noise", "100", &sound ) ) {
		ent->s.loopSound = G_SoundIndex( sound );
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", color );
	if ( lightSet || colorSet ) {
		int		r, g, b, i;

		r = color[0] * 255;
		if ( r > 255 ) {
			r = 255;
		}
		g = color[1] * 255;
		if ( g > 255 ) {
			g = 255;
		}
		b = color[2] * 255;
		if ( b > 255 ) {
			b = 255;
		}
		i = light / 4;
		if ( i > 255 ) {
			i = 255;
		}
		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}


	ent->use = Use_BinaryMover;
	ent->reached = Reached_BinaryMover;

	ent->moverState = MOVER_POS1;
	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_MOVER;
	// IneQuation
	if (Q_stricmp(ent->classname, "func_rotatingdoor")) {
		VectorCopy (ent->pos1, ent->r.currentOrigin);
		trap_LinkEntity (ent);

		ent->s_pos.trType = TR_STATIONARY;
		VectorCopy( ent->pos1, ent->s_pos.trBase );

		// calculate time to reach second position from speed
		VectorSubtract( ent->pos2, ent->pos1, move );
		distance = VectorLength( move );
		if ( ! ent->speed ) {
			ent->speed = 100;
		}
		VectorScale( move, ent->speed, ent->s_pos.trDelta );
		ent->s_pos.trDuration = distance * 1000 / ent->speed;
		if ( ent->s_pos.trDuration <= 0 ) {
			ent->s_pos.trDuration = 1;
		}
	} else {
		VectorCopy(ent->s.origin, ent->r.currentOrigin);
		VectorClear(ent->r.currentAngles);
		trap_LinkEntity(ent);

		// linear component - basically static
		ent->s_pos.trType = TR_STATIONARY;
		VectorCopy(ent->s.origin, ent->s_pos.trBase);
		VectorClear(ent->s_pos.trDelta);
		ent->s_pos.trDuration = 1;

		// angular component
		ent->s_apos.trType = TR_STATIONARY;
		VectorClear(ent->s_apos.trBase);
		VectorSubtract(ent->pos2, ent->pos1, move);
		/*distance = */VectorNormalize(move);
		//distance /= ent->speed;
		VectorScale(move, ent->speed, ent->s_apos.trDelta);
		ent->s_apos.trDuration = (float)ent->splashRadius / ent->speed * 1000;
		//VectorSet(ent->s_apos.trDelta, 0, ent->speed, 0);
	}

}


/*
===============================================================================

DOOR

A use can be triggered either by a touch function, by being shot, or by being
targeted by another entity.

===============================================================================
*/

/*
================
Blocked_Door
================
*/
void Blocked_Door( gentity_t *ent, gentity_t *other ) {
	// remove anything other than a client
	//if ( !other->client ) {
	//	// except CTF flags!!!!
	//	if( other->s.eType == ET_ITEM /*&& other->item->giType == IT_TEAM*/ ) {
	//		Team_DroppedFlagThink( other );
	//		return;
	//	}
	////	G_TempEntity( other->s.origin, EV_ITEM_POP );
	//	G_FreeEntity( other );
	//	return;
	//}

	if ( ent->damage ) {
		G_Damage( other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH );
	}
	if (ent->spawnflags & 4 && Q_stricmp(ent->classname, "func_rotatingdoor")) {	// IneQuation: rotating doors don't crush
		return;		// crushers don't reverse
	}
G_Printf("Blocked_Door: reversing\n");
	// reverse direction
	Use_BinaryMover( ent, ent, other );

}

/*
================
Touch_DoorTriggerSpectator
================
*/
static void Touch_DoorTriggerSpectator( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	int i, axis;
	vec3_t origin, dir, angles;

	axis = ent->count;
	VectorClear(dir);
	if (fabs(other->s.origin[axis] - ent->r.absmax[axis]) <
		fabs(other->s.origin[axis] - ent->r.absmin[axis])) {
		origin[axis] = ent->r.absmin[axis] - 10;
		dir[axis] = -1;
	}
	else {
		origin[axis] = ent->r.absmax[axis] + 10;
		dir[axis] = 1;
	}
	for (i = 0; i < 3; i++) {
		if (i == axis) continue;
		origin[i] = (ent->r.absmin[i] + ent->r.absmax[i]) * 0.5;
	}
	vectoangles(dir, angles);
	TeleportPlayer(other, origin, angles );
}

/*
================
Touch_DoorTrigger
================
*/
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( other->client && other->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		// if the door is not open and not opening
		if ( ent->parent->moverState != MOVER_1TO2 &&
			ent->parent->moverState != MOVER_POS2) {
			Touch_DoorTriggerSpectator( ent, other, trace );
		}
	}
	else if ( ent->parent->moverState != MOVER_1TO2 ) {
		Use_BinaryMover( ent->parent, ent, other );
	}
}


/*
======================
Think_SpawnNewDoorTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them
======================
*/
void Think_SpawnNewDoorTrigger( gentity_t *ent ) {
	gentity_t		*other;
	vec3_t		mins, maxs;
	int			i, best;

	// set all of the slaves as shootable
	for ( other = ent ; other ; other = other->teamchain ) {
		other->takedamage = ent->splashDamage ? qfalse : qtrue; // IneQuation: unless door requires "manual" activation
	}

	// find the bounds of everything on the team
	VectorCopy (ent->r.absmin, mins);
	VectorCopy (ent->r.absmax, maxs);

	for (other = ent->teamchain ; other ; other=other->teamchain) {
		AddPointToBounds (other->r.absmin, mins, maxs);
		AddPointToBounds (other->r.absmax, mins, maxs);
	}

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for ( i = 1 ; i < 3 ; i++ ) {
		if ( maxs[i] - mins[i] < maxs[best] - mins[best] ) {
			best = i;
		}
	}
	maxs[best] += 120;
	mins[best] -= 120;

	// create a trigger with this size
	other = G_Spawn ();
	other->classname = "door_trigger";
	VectorCopy (mins, other->r.mins);
	VectorCopy (maxs, other->r.maxs);
	other->parent = ent;
	other->r.contents = CONTENTS_TRIGGER;
	other->touch = Touch_DoorTrigger;
	other->splashDamage = ent->splashDamage;
	// remember the thinnest axis
	other->count = best;
	trap_LinkEntity (other);

	MatchTeam( ent, ent->moverState, level.time );
}

void Think_MatchTeam( gentity_t *ent ) {
	MatchTeam( ent, ent->moverState, level.time );
}


/*QUAKED func_door (0 .5 .8) ? START_OPEN x CRUSHER
TOGGLE		wait in both the start and end states for a trigger event.
START_OPEN	the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER	monsters will not trigger this door

"model2"	.md3 model to also draw
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"lip"		lip remaining at end of move (8 default)
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
"health"	if set, the door must be shot open
*/
void SP_func_door (gentity_t *ent) {
	vec3_t	abs_movedir;
	float	distance;
	vec3_t	size;
	float	lip;

	ent->sound1to2 = ent->sound2to1 = G_SoundIndex("sound/movers/doors/dr1_strt.wav");
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex("sound/movers/doors/dr1_end.wav");

	ent->blocked = Blocked_Door;

	// default speed of 400
	if (!ent->speed)
		ent->speed = 400;

	// default wait of 2 seconds
	if (!ent->wait)
		ent->wait = 2;
	ent->wait *= 1000;

	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// default damage of 2 points
	G_SpawnInt( "dmg", "2", &ent->damage );

	// first position at start
	VectorCopy( ent->s.origin, ent->pos1 );

	// calculate second position
	trap_SetBrushModel( ent, ent->model );
	G_SetMovedir (ent->s.angles, ent->movedir);
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	VectorSubtract( ent->r.maxs, ent->r.mins, size );
	distance = DotProduct( abs_movedir, size ) - lip;
	VectorMA( ent->pos1, distance, ent->movedir, ent->pos2 );

	// if "start_open", reverse position 1 and 2
	if ( ent->spawnflags & 1 ) {
		vec3_t	temp;

		VectorCopy( ent->pos2, temp );
		VectorCopy( ent->s.origin, ent->pos2 );
		VectorCopy( temp, ent->pos1 );
	}

	InitMover( ent );

	ent->nextthink = level.time + FRAMETIME;

	if ( ! (ent->flags & FL_TEAMSLAVE ) ) {
		int health;

		G_SpawnInt( "health", "0", &health );
		if ( health ) {
			ent->takedamage = qtrue;
		}
		if ( ent->targetname || health ) {
			// non touch/shoot doors
			ent->think = Think_MatchTeam;
		} else {
			ent->think = Think_SpawnNewDoorTrigger;
		}
	}


}

/*QUAKED func_rotatingdoor (0 0.25 0.5) ? START_OPEN OPEN_DIRECTION DOOR_DONT_LINK NOT_PLAYERS NOT_MONSTERS TOGGLE AUTO_OPEN TARGETED
if two doors touch, they are assumed to be connected and operate as a unit.

TOGGLE causes the door to wait in both the start and end states for a trigger event.
DOOR_DONT_LINK is for when you have two doors that are touching but you want to operate independently.

START_OPEN causes the door to move to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not usefull for touch or takedamage doors).
OPEN_DIRECTION indicates which direction to open when START_OPEN is set.
AUTO_OPEN causes the door to open when a player is near instead of waiting for the player to use the door.
TARGETED door is only operational from triggers or script

"message"		is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"openangle"		how wide to open the door
"angle"			determines the opening direction.  point toward the middle of the door (away from the hinge)
"targetname"	if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"		if set, door must be shot open
"time"			move time (0.3 default)
"wait"			wait before returning (3 default, -1 = never return)
"dmg"			damage to inflict when blocked (0 default)
"alwaysaway"	makes the door always open away from the opener

"sound_open"		Specify the sound that plays when the door starts opening (default global door_wood_open)
"sound_close"		Specify the sound that plays when the door starts closing (default global door_wood_close)
"sound_message"		Specify the sound that plays when the door displays a message
"sound_locked"		Specify the sound that plays when the door is locked (default global door_wood_locked)

******************************************************************************/
void SP_func_rotatingdoor(gentity_t *ent) {
	float	time;

	ent->sound1to2 = ent->sound2to1 = G_SoundIndex("sound/movers/doors/dr1_strt.wav");
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex("sound/movers/doors/dr1_end.wav");

	ent->blocked = Blocked_Door;

	// default wait of 3 seconds
	if (!ent->wait)
		ent->wait = 3;
	ent->wait *= 1000;

	// movement speed
	G_SpawnFloat("time", "0.3", &time);

	// how wide the door will open
	G_SpawnInt("openangle", "90", &ent->splashRadius);

	//if (!ent->speed)
		ent->speed = ent->splashRadius / time;

	// default damage of 0 points
	G_SpawnInt("dmg", "0", &ent->damage);

	trap_SetBrushModel(ent, ent->model);

	// this is actually used for the door to always open away from player
	// FIXME: the behaviour for vertically-pointing angles is undefined!
	// find the door plane
	if (ent->angle == -1.f) {
		VectorSet(ent->movedir, 0, 0, 1);
		ent->physicsBounce = ent->s.origin[2];
	} else if (ent->angle == -2.f) {
		VectorSet(ent->movedir, 0, 0, -1);
		ent->physicsBounce = -ent->s.origin[2];
	} else {
		AngleVectors(ent->s.angles, ent->movedir, NULL, NULL);
		ent->physicsBounce = DotProduct(ent->s.origin, ent->movedir);
	}

	// these are calculated only so that InitMover calculates the trDelta properly
	// first position at start
	VectorClear(ent->pos1);
	// destination
	if (ent->movedir[2] != 0.f)
		VectorSet(ent->pos2, ent->splashRadius, 0, 0);
	else
		VectorSet(ent->pos2, 0, ent->splashRadius, 0);

	// keep track of the door opening direction so that we can just negate apos.trDelta
	ent->splashMethodOfDeath = 0;

	// if "start_open", reverse position 1 and 2
	if (ent->spawnflags & 1) {
		// TODO: it's not that easy, need to switch off alwaysaway and stuff
		/*vec3_t	temp;

		VectorCopy(ent->pos2, temp);
		VectorCopy(ent->pos1, ent->pos2 );
		VectorCopy(temp, ent->pos1);*/
	}

	// if not AUTO_OPEN, player needs to press use to activate
	if (!(ent->spawnflags & 64))
		ent->splashDamage = 1;
	else
		ent->splashDamage = 0;

	InitMover(ent);

	ent->nextthink = level.time + FRAMETIME;

	if (!(ent->flags & FL_TEAMSLAVE)) {
		int health;

		G_SpawnInt("health", "0", &health);
		if (health)
			ent->takedamage = qtrue;
		if (ent->targetname || health)
			// non touch/shoot doors
			ent->think = Think_MatchTeam;
		else
			ent->think = Think_SpawnNewDoorTrigger;
	}
}

/*
===============================================================================

PLAT

===============================================================================
*/

/*
==============
Touch_Plat

Don't allow decent if a living player is on it
===============
*/
void Touch_Plat( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client || other->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	// delay return-to-pos1 by one second
	if ( ent->moverState == MOVER_POS2 ) {
		ent->nextthink = level.time + 1000;
	}
}

/*
==============
Touch_PlatCenterTrigger

If the plat is at the bottom position, start it going up
===============
*/
void Touch_PlatCenterTrigger(gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->parent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent->parent, ent, other );
	}
}


/*
================
SpawnPlatTrigger

Spawn a trigger in the middle of the plat's low position
Elevator cars require that the trigger extend through the entire low position,
not just sit on top of it.
================
*/
void SpawnPlatTrigger( gentity_t *ent ) {
	gentity_t	*trigger;
	vec3_t	tmin, tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position
	trigger = G_Spawn();
	trigger->classname = "plat_trigger";
	trigger->touch = Touch_PlatCenterTrigger;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->parent = ent;

	tmin[0] = ent->pos1[0] + ent->r.mins[0] + 33;
	tmin[1] = ent->pos1[1] + ent->r.mins[1] + 33;
	tmin[2] = ent->pos1[2] + ent->r.mins[2];

	tmax[0] = ent->pos1[0] + ent->r.maxs[0] - 33;
	tmax[1] = ent->pos1[1] + ent->r.maxs[1] - 33;
	tmax[2] = ent->pos1[2] + ent->r.maxs[2] + 8;

	if ( tmax[0] <= tmin[0] ) {
		tmin[0] = ent->pos1[0] + (ent->r.mins[0] + ent->r.maxs[0]) *0.5;
		tmax[0] = tmin[0] + 1;
	}
	if ( tmax[1] <= tmin[1] ) {
		tmin[1] = ent->pos1[1] + (ent->r.mins[1] + ent->r.maxs[1]) *0.5;
		tmax[1] = tmin[1] + 1;
	}

	VectorCopy (tmin, trigger->r.mins);
	VectorCopy (tmax, trigger->r.maxs);

	trap_LinkEntity (trigger);
}


/*QUAKED func_plat (0 .5 .8) ?
Plats are always drawn in the extended position so they will light correctly.

"lip"		default 8, protrusion above rest position
"height"	total height of movement, defaults to model height
"speed"		overrides default 200.
"dmg"		overrides default 2
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_plat (gentity_t *ent) {
	float		lip, height;

	ent->sound1to2 = ent->sound2to1 = G_SoundIndex("sound/movers/plats/pt1_strt.wav");
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex("sound/movers/plats/pt1_end.wav");

	VectorClear (ent->s.angles);

	G_SpawnFloat( "speed", "200", &ent->speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "wait", "1", &ent->wait );
	G_SpawnFloat( "lip", "8", &lip );

	ent->wait = 1000;

	// create second position
	trap_SetBrushModel( ent, ent->model );

	if ( !G_SpawnFloat( "height", "0", &height ) ) {
		height = (ent->r.maxs[2] - ent->r.mins[2]) - lip;
	}

	// pos1 is the rest (bottom) position, pos2 is the top
	VectorCopy( ent->s.origin, ent->pos2 );
	VectorCopy( ent->pos2, ent->pos1 );
	ent->pos1[2] -= height;

	InitMover( ent );

	// touch function keeps the plat from returning while
	// a live player is standing on it
	ent->touch = Touch_Plat;

	ent->blocked = Blocked_Door;

	ent->parent = ent;	// so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if ( !ent->targetname ) {
		SpawnPlatTrigger(ent);
	}
}


/*
===============================================================================

BUTTON

===============================================================================
*/

/*
==============
Touch_Button

===============
*/
void Touch_Button(gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent, other, other );
	}
}


/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"model2"	.md3 model to also draw
"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_button( gentity_t *ent ) {
	vec3_t		abs_movedir;
	float		distance;
	vec3_t		size;
	float		lip;

	ent->sound1to2 = G_SoundIndex("sound/movers/switches/butn2.wav");

	if ( !ent->speed ) {
		ent->speed = 40;
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->wait *= 1000;

	// first position
	VectorCopy( ent->s.origin, ent->pos1 );

	// calculate second position
	trap_SetBrushModel( ent, ent->model );

	G_SpawnFloat( "lip", "4", &lip );

	G_SetMovedir( ent->s.angles, ent->movedir );
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	VectorSubtract( ent->r.maxs, ent->r.mins, size );
	distance = abs_movedir[0] * size[0] + abs_movedir[1] * size[1] + abs_movedir[2] * size[2] - lip;
	VectorMA (ent->pos1, distance, ent->movedir, ent->pos2);

	if (ent->health) {
		// shootable button
		ent->takedamage = qtrue;
	} else {
		// touchable button
		ent->touch = Touch_Button;
	}

	InitMover( ent );
}



/*
===============================================================================

TRAIN

===============================================================================
*/


#define TRAIN_START_ON		1
#define TRAIN_TOGGLE		2
#define TRAIN_BLOCK_STOPS	4

/*
===============
Think_BeginMoving

The wait time at a corner has completed, so start moving again
===============
*/
void Think_BeginMoving( gentity_t *ent ) {

}

/*
===============
Reached_Train
===============
*/
void Reached_Train( gentity_t *ent ) {
	
}


/*
===============
Think_SetupTrainTargets

Link all the corners together
===============
*/
void Think_SetupTrainTargets( gentity_t *ent ) {
	gentity_t		*path, *next, *start;

	ent->nextTrain = G_Find( NULL, FOFS(targetname), ent->target );
	if ( !ent->nextTrain ) {
		G_Printf( "func_train at %s with an unfound target\n",
			vtos(ent->r.absmin) );
		return;
	}

	start = NULL;
	for ( path = ent->nextTrain ; path != start ; path = next ) {
		if ( !start ) {
			start = path;
		}

		if ( !path->target ) {
			G_Printf( "Train corner at %s without a target\n",
				vtos(path->s.origin) );
			return;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = NULL;
		do {
			next = G_Find( next, FOFS(targetname), path->target );
			if ( !next ) {
				G_Printf( "Train corner at %s without a target path_corner\n",
					vtos(path->s.origin) );
				return;
			}
		} while ( strcmp( next->classname, "path_corner" ) );

		path->nextTrain = next;
	}

	// start the train moving from the first corner
	Reached_Train( ent );
}



/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8)
Train path corners.
Target: next path corner and other targets to fire
"speed" speed to move to the next corner
"wait" seconds to wait before behining move to next corner
*/
void SP_path_corner( gentity_t *self ) {
	if ( !self->targetname ) {
		G_Printf ("path_corner with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEntity( self );
		return;
	}
	// path corners don't need to be linked in
}



/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
A train is a mover that moves between path_corner target points.
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.
"model2"	.md3 model to also draw
"speed"		default 100
"dmg"		default	2
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_train (gentity_t *self) {
	VectorClear (self->s.angles);

	if (self->spawnflags & TRAIN_BLOCK_STOPS) {
		self->damage = 0;
	} else {
		if (!self->damage) {
			self->damage = 2;
		}
	}

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		G_Printf ("func_train without a target at %s\n", vtos(self->r.absmin));
		G_FreeEntity( self );
		return;
	}

	trap_SetBrushModel( self, self->model );
	InitMover( self );

	self->reached = Reached_Train;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets;
}

/*
===============================================================================

STATIC

===============================================================================
*/


/*QUAKED func_static (0 .5 .8) ?
A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_static( gentity_t *ent ) {
	trap_SetBrushModel( ent, ent->model );
	InitMover( ent );
	VectorCopy( ent->s.origin, ent->s.origin );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
}


/*
===============================================================================

ROTATING

===============================================================================
*/


/*QUAKED func_rotating (0 .5 .8) ? START_ON - X_AXIS Y_AXIS
You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"model2"	.md3 model to also draw
"speed"		determines how fast it moves; default value is 100.
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_rotating (gentity_t *ent) {
	
}


/*
===============================================================================

BOBBING

===============================================================================
*/


/*QUAKED func_bobbing (0 .5 .8) ? X_AXIS Y_AXIS
Normally bobs on the Z axis
"model2"	.md3 model to also draw
"height"	amplitude of bob (32 default)
"speed"		seconds to complete a bob cycle (4 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_bobbing (gentity_t *ent) {
	
}

/*
===============================================================================

PENDULUM

===============================================================================
*/


/*QUAKED func_pendulum (0 .5 .8) ?
You need to have an origin brush as part of this entity.
Pendulums always swing north / south on unrotated models.  Add an angles field to the model to allow rotation in other directions.
Pendulum frequency is a physical constant based on the length of the beam and gravity.
"model2"	.md3 model to also draw
"speed"		the number of degrees each way the pendulum swings, (30 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_pendulum(gentity_t *ent) {

}
