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
// bg_public.h -- definitions shared by both the server game and client game modules

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#define	GAME_VERSION		"mohaa-base-1"

#define	DEFAULT_GRAVITY		800

#define	SCORE_NOT_PRESENT	-9999	// for the CS_SCORES[12] when only one player is present

#define	VOTE_TIME			30000	// 30 seconds before vote times out

#define	MINS_Z				0		// IneQuation: bounding box and viewheights to match MoHAA
#define	DEFAULT_VIEWHEIGHT	82
#define CROUCH_VIEWHEIGHT	48
#define	DEAD_VIEWHEIGHT		8

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h

#define	CS_MESSAGE				2		// from the map worldspawn's message field
#define	CS_MOTD					4		// g_motd string for server message of the day
#define	CS_WARMUP				5		// server time when the match will be restarted

#define	CS_MUSIC				8		// MUSIC_NewSoundtrack(cs)
#define CS_FOGINFO				9		// cg.farplane_cull cg.farplane_distance cg.farplane_color[3]
#define CS_SKYINFO				10		// cg.sky_alpha cg.sky_portal

#define	CS_GAME_VERSION			11
#define	CS_LEVEL_START_TIME		12		// so the timer only shows the current level cgs.levelStartTime

#define CS_RAIN_DENSITY			14		// cg.rain
#define CS_RAIN_SPEED			15
#define CS_RAIN_SPEEDVARY		16
#define CS_RAIN_SLANT			17
#define CS_RAIN_LENGTH			18
#define CS_RAIN_MINDIST			19
#define CS_RAIN_WIDTH			20
#define CS_RAIN_SHADER			21
#define CS_RAIN_NUMSHADERS		22

#define CS_MATCHEND				26		// cgs.matchEndTime

#define	CS_MODELS				32
#define CS_OBJECTIVES			(CS_MODELS+MAX_MODELS) // 1056
#define	CS_SOUNDS				(CS_OBJECTIVES+MAX_OBJECTIVES) // 1076

#define CS_IMAGES				(CS_SOUNDS+MAX_SOUNDS) // 1588
#define MAX_IMAGES				64

#define CS_LIGHTSTYLES			(CS_IMAGES+MAX_IMAGES) //1652
#define CS_PLAYERS				(CS_LIGHTSTYLES+MAX_LIGHTSTYLES) // 1684

#define CS_WEAPONS				(CS_PLAYERS+MAX_CLIENTS) // su44 was here
#define CS_UNKNOWN				1876
#define CS_SPECTATORS			1878
#define CS_ALLIES				1879
#define CS_AXIS					1880

#define CS_MAX					(CS_PARTICLES+MAX_LOCATIONS)
#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif


typedef enum {
	GT_SINGLE_PLAYER,	// single player ffa
	GT_FFA,				// free for all
	GT_TEAM,			// team deathmatch
	GT_TEAM_ROUNDS,
	GT_OBJECTIVE,
	GT_MAX_GAME_TYPE
} gametype_t;

//
// scale to use when evaluating entityState_t::constantLight scale
//
#define CONSTANTLIGHT_RADIUS_SCALE 8

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

// su44: vma indexes are sent as 4 bits
// see playerState_t::iViewModelAnim
typedef enum {
	VMA_IDLE,
	VMA_CHARGE,
	VMA_FIRE,
	VMA_FIRE_SECONDARY,
	VMA_RECHAMBER,
	VMA_RELOAD,
	VMA_RELOAD_SINGLE,
	VMA_RELOAD_END,
	VMA_PULLOUT,
	VMA_PUTAWAY,
	VMA_LADDERSTEP,
	VMA_NUMANIMATIONS
} viewmodelanim_t;

// su44: playerState_t::activeItems[8] slots
// they are sent as 16 bits
// TODO: find out rest of them
#define ITEM_WEAPON		1	// current mainhand weapon

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

// su44: our pmtype_t enum must be the same
// as in MoHAA, because playerState_t::pm_type
// is send over the net and used by cgame
// for movement prediction
typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_CLIMBWALL, // su44: I think it's used for ladders
	PM_NOCLIP,		// noclip movement
	PM_DEAD,		// no acceleration or turning, but free falling

	// TODO: remove this one below, there is no PM_SPECTATOR in MoH
	PM_SPECTATOR,	// still run into walls
} pmtype_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_TIME_LAND		32		// pm_time is time before rejump
#define	PMF_TIME_KNOCKBACK	64		// pm_time is an air-accelerate only time
#define	PMF_TIME_WATERJUMP	256		// pm_time is waterjump
#define	PMF_RESPAWNED		512		// clear after attack and jump buttons come up
#define	PMF_USE_ITEM_HELD	1024
#define PMF_GRAPPLE_PULL	2048	// pull towards grapple location
#define PMF_FOLLOW			4096	// spectate following another player
#define PMF_SCOREBOARD		8192	// spectate as a scoreboard
#define PMF_INVULEXPAND		16384	// invulnerability sphere set to full size

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

#define	MAXTOUCH	32
typedef struct {
	// state (in / out)
	playerState_t	*ps;

	// command (in)
	usercmd_t	cmd;
	int			tracemask;			// collide against these types of surfaces
	int			debugLevel;			// if set, diagnostic output will be printed
	qboolean	noFootsteps;		// if the game is setup for no footsteps by the server

	int			framecount;

	// results (out)
	int			numtouch;
	int			touchents[MAXTOUCH];

	vec3_t		mins, maxs;			// bounding box size

	int			watertype;
	int			waterlevel;

	float		xyspeed;

	// for fixed msec Pmove
	int			pmove_fixed;
	int			pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void		(*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int			(*pointcontents)( const vec3_t point, int passEntityNum );
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void Pmove (pmove_t *pmove);

//===================================================================================


// player_state->stats[] indexes
// NOTE: may not have more than 32
typedef enum {
	STAT_HEALTH,
	STAT_MAX_HEALTH,
	STAT_WEAPONS,
	STAT_EQUIPPED_WEAPON,
	STAT_AMMO,	
	STAT_MAXAMMO,
	STAT_CLIPAMMO,		// ammo in current weapon clip
	STAT_MAXCLIPAMMO,	// maxammo in current weapon clip
	STAT_INZOOM,
	STAT_CROSSHAIR,
	STAT_LAST_PAIN,		// Last amount of damage the player took
	STAT_UNUSED_2,
	STAT_BOSSHEALTH,	// if we are fighting a boss, how much health he currently has
	STAT_CINEMATIC,		// This is set when we go into cinematics
	STAT_ADDFADE,		// This is set when we need to do an addblend for the fade
	STAT_LETTERBOX,		// This is set to the fraction of the letterbox
	STAT_COMPASSNORTH,
	STAT_OBJECTIVELEFT,
	STAT_OBJECTIVERIGHT,
	STAT_OBJECTIVECENTER,
	STAT_TEAM,
	STAT_KILLS,
	STAT_DEATHS,
	STAT_UNUSED_3,
	STAT_UNUSED_4,
	STAT_HIGHEST_SCORE,
	STAT_ATTACKERCLIENT, 
	STAT_INFOCLIENT,
	STAT_INFOCLIENT_HEALTH,
	STAT_DAMAGEDIR,
	STAT_LAST_STAT
} statIndex_t;
#define STAT_DEAD_YAW 5 // su44: Is there a DEAD_YAW stat in MoHAA?

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum { 
	PERS_SCORE,		// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_TEAM		// player team
} persEnum_t;

// entityState_t->eFlags
// su44: eFlags used in MoHAA
#define EF_UNARMED			0x00000020		// su44: this player has his weapon holstered
#define EF_ALLIES			0x00000080		// su44: this player is in allies team
#define EF_AXIS				0x00000100		// su44: this player is in axis team
#define EF_DEAD				0x00000200		// don't draw a foe marker over players with EF_DEAD


// su44: q3 remnants
#define EF_TICKING			0x00000002		// used to make players play the prox mine ticking sound
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes
#define	EF_AWARD_EXCELLENT	0x00000008		// draw an excellent sprite
#define EF_PLAYER_EVENT		0x00000010
#define	EF_BOUNCE			0x00000010		// for missiles
#define	EF_AWARD_GAUNTLET	0x00000040		// draw a gauntlet sprite
#define	EF_MOVER_STOP		0x00000400		// will push otherwise
#define EF_AWARD_CAP		0x00000800		// draw the capture sprite
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote
#define	EF_AWARD_IMPRESSIVE	0x00008000		// draw an impressive sprite
#define	EF_AWARD_DEFEND		0x00010000		// draw a defend sprite
#define	EF_AWARD_ASSIST		0x00020000		// draw a assist sprite
#define EF_AWARD_DENIED		0x00040000		// denied
#define EF_TEAMVOTED		0x00080000		// already cast a team vote

// these defines could be deleted sometime when code/game/ is cleared of Q3A stuff
#define TEAM_FREE 0
#define TEAM_RED 4
#define TEAM_BLUE 3
#define TEAM_NUM_TEAMS 5

typedef enum {
	TEAM_NONE,
	TEAM_SPECTATOR,
	TEAM_FREEFORALL,
	TEAM_ALLIES,
	TEAM_AXIS
} team_t;

// How many players on the overlay
#define TEAM_MAXOVERLAY		32

// means of death
// su44: changed to MoHAA's
typedef enum {
	MOD_NONE,
	MOD_SUICIDE,
	MOD_CRUSH,
	MOD_CRUSH_EVERY_FRAME,
	MOD_TELEFRAG,
	MOD_LAVA,
	MOD_SLIME,
	MOD_FALLING,
	MOD_LAST_SELF_INFLICTED,
	MOD_EXPLOSION,
	MOD_EXPLODEWALL,
	MOD_ELECTRIC,
	MOD_ELECTRICWATER,
	MOD_THROWNOBJECT,
	MOD_GRENADE,
	MOD_BEAM,
	MOD_ROCKET,
	MOD_IMPACT,
	MOD_BULLET,
	MOD_FAST_BULLET,
	MOD_VEHICLE,
	MOD_FIRE,
	MOD_FLASHBANG,
	MOD_ON_FIRE,
	MOD_GIB,
	MOD_IMPALE,
	MOD_BASH,
	MOD_SHOTGUN,
	MOD_TOTAL_NUMBER,

	// su44: these are not present in MoH,
	// but deleting them would give compile
	// errors in game module
	MOD_WATER,
	MOD_TRIGGER_HURT,
	MOD_UNKNOWN = MOD_NONE,

} meansOfDeath_t;

//---------------------------------------------------------

// g_dmflags->integer flags
#define	DF_NO_FALLING			8
#define DF_FIXED_FOV			16
#define	DF_NO_FOOTSTEPS			32

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY|CONTENTS_FENCE)	// IneQuation: don't fall through fences
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_FENCE)					// IneQuation: don't fall through fences
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE)

//
// entityState_t->eType
//
typedef enum {
	ET_MODELANIM_SKEL,
	ET_MODELANIM,
	ET_VEHICLE,
	ET_PLAYER,
	ET_ITEM,
	ET_GENERAL,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_MULTIBEAM,
	ET_PORTAL,
	ET_EVENT_ONLY,
	ET_RAIN,
	ET_LEAF,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_DECAL,
	ET_EMITTER,
	ET_ROPE,
	ET_EVENTS,
	ET_EXEC_COMMANDS
} entityType_t;

// su44; yes, I know there is no q3 trajectory_t in MoHAA,
// but I need it for fgame rotating doors code.
void	BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void	BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void	BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap );
void	BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap );
