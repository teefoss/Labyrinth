//
//  labyrinth.h
//  Labyrinth
//
//  Created by Thomas Foster on 7/11/19.
//  Copyright © 2019 Thomas Foster. All rights reserved.
//

#ifndef labyrinth_h
#define labyrinth_h

#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define bound(a,b,c)		a = a < b  ? b : a > c ? c : a
#define sign(x)				x < 0 ? -1 : x > 0 ? 1 : 0
#define signf(float)		float < 0.0f ? -1.0f : float > 0.0f ? 1.0f : 0.0f

#define WIN_W				320
#define WIN_H				200
#define SCALE				3

#define MAPSIZE				64

#define ANGLES	 M_PI * 2
#define ANG90	 ANGLES / 4
#define ANG180	 ANG90 * 2
#define ANG270	 ANG90 * 3


#define NUMDIMS				5

#define PL_RADIUS			0.25f
#define PL_TURN 			0.025f
#define PL_ACCEL			0.005f
#define PL_MOVE_SPD			0.1f
#define PL_STRAFE_SPD		(PL_MOVE_SPD * 0.5f)
#define PLAYER_MAX_SPEED	0.1f

typedef struct
{
	float x;
	float y;
} point;

// gate sides, etc
typedef enum
{
	SIDE_UNDEFINED,
	SIDE_TOP,
	SIDE_BOTTOM,
	SIDE_LEFT,
	SIDE_RIGHT
} side_t;

typedef enum
{
	GS_EDITOR,
	GS_TITLE,
	GS_PLAY
} gamestate_t;

typedef enum
{					// Dimension
	WT_WOOD,		// 0
	WT_MARBLE,		// 1
	WT_TECH,		// 2
	WT_CEMENT,		// 3
	WT_STONE,		// 4
	WT_FIRE,		// gate
	WT_COUNT
} wall_t;

typedef enum
{
	TT_EMPTY,
	TT_PLAYERSTART,
	TT_WALL,
	TT_GATE_H,
	TT_GATE_V,
	TT_COUNT
} tiletype_t;

typedef struct
{
	tiletype_t type;
	// type is TT_WALL: id is which wall_t
	// type is TT_GATE: id indicates which dimension gate goes to (0..<NUMDIMS)
	int id;
} tile_t;

typedef struct
{
	float samplex;
} drawinfo_t;

typedef enum
{
	OT_PLAYER,
	OT_RAY
} objtype_t;

typedef struct
{
	objtype_t type;
	
	// object's position (center)
	float 	x;
	float 	y;
	
	// object's speed
	float 	dx;
	float 	dy;
	
	// object's radius
	float	r;
	
	// which tile object is in
	int		tilex;
	int		tiley;
	
	// object's old position
	float 	oldx;
	float	oldy;
	
	// hit box
	float	top,bottom,left,right; // updated by SetPosition
	
	// gate
	bool	ingate; // obj in gate?
	side_t	entryside;	// which side of gate
	
	// current dimension
	int 	w;
	
	// player angle, set with SetPlayerAngle to update sin/cos
	float 	angle;
	
	// store sinf(angle), cosf(angle) for fewer calcs
	float 	sin;
	float 	cos;
} obj_t;



// LABYRINTH.C

extern SDL_Window 		*window;
extern SDL_Renderer 	*renderer;
extern SDL_Surface 		*walls[WT_COUNT];
extern SDL_Texture		*walltextures[WT_COUNT];
extern SDL_Texture		*text;

extern obj_t 			player;
extern gamestate_t 		gamestate;
extern const uint8_t 	*keys;
extern tile_t 			map[NUMDIMS][MAPSIZE][MAPSIZE];

void Quit (const char *error);

// OBJECT.C

void CheckBlock (obj_t *obj);
void SetAngle (obj_t *obj, float a);
void SetPosition (obj_t *obj, float x, float y);
void Thrust (float angle, float speed);
void ControlMovement (obj_t *obj);

// EDITOR.C

bool Ctrl (void);
void EditorLoop (void);
void OpenMap (int number);

#endif /* labyrinth_h */
