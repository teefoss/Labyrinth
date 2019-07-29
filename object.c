//
//  object.c
//  Labyrinth
//
//  Created by Thomas Foster on 7/19/19.
//  Copyright © 2019 Thomas Foster. All rights reserved.
//

#include "labyrinth.h"


//
// SetPlayerAngle
// Set angle and store sin and cos
//
void SetAngle (obj_t *obj, float a)
{
	// keep angle within -180 to 180
	if (a < 0)
		a += ANGLES;
	else if (a > ANGLES)
		a -= ANGLES;
	obj->angle = a;
	obj->sin = sinf(a);
	obj->cos = cosf(a);
}




void SetPosition (obj_t *obj, float x, float y)
{
	// save old position and update
	obj->oldx 	= obj->x;
	obj->oldy 	= obj->y;
	obj->x 		= x;
	obj->y 		= y;
	
	// update position info
	obj->tilex 		= (int)x;
	obj->tiley 		= (int)y;
	obj->top 		= y - obj->r;
	obj->bottom 	= y + obj->r;
	obj->left		= x - obj->r;
	obj->right		= x + obj->r;
}




tiletype_t CurrentBlockType (obj_t *obj)
{
	return map[obj->w][(int)obj->y][(int)obj->x].type;
}




//
// GateSide
// Returns which side of the gate the object is on
// LEFT or RIGHT if it's a GATE_H
// TOP or BOTTOM if it's a GATE_V
// (should never return SIDE_UNDEFINED, indicates error)
//
side_t GateSide (obj_t *obj)
{
	tiletype_t type;
	int 		x, y; // block coords
	float 		position;
	
	type = CurrentBlockType(obj);
	x = (int)obj->x;
	y = (int)obj->y;
	position = type == TT_GATE_H ? obj->x-(float)x : obj->y-(float)y;
	
	if (type == TT_GATE_H)
		return position < 0.5f ? SIDE_LEFT : SIDE_RIGHT;
	else if (type == TT_GATE_V)
		return position < 0.5f ? SIDE_TOP : SIDE_BOTTOM;
	return SIDE_UNDEFINED;
}




//
// DoGate
// Obj is in a gate, handle it
//
void DoGate (obj_t *obj)
{
	int w;
	
	if (!obj->ingate) {
		// just entered a gate
		obj->ingate = true;
		obj->entryside = GateSide(obj); // store current side
	} else {
		// already in gate: crossed to other side?
		if (obj->entryside == SIDE_UNDEFINED)
			Quit("DoGate: GetSide returned SIDE_UNDEFINED");
		
		if (GateSide(obj) != obj->entryside) {
			// crossed to other side
			for (w=0 ; w<NUMDIMS ; w++) {
				if (w == obj->w) continue;
				if (map[w][obj->tiley][obj->tilex].type == CurrentBlockType(obj)) {
					obj->w = w;
					break;
				}
			}
			// (in case obj goes back while still in portal)
			obj->entryside = GateSide(obj);
		}
	}
}




//
// CheckBlock
// Check block obj is currently in and
// process accordingly
//
void CheckBlock (obj_t *obj)
{
	switch (map[obj->w][(int)obj->y][(int)obj->x].type)
	{
		// handle passage through a gate
		case TT_GATE_H:
		case TT_GATE_V:
			DoGate(obj);
			break;
			
		default:
			obj->ingate = false; // in case exited a gate
			break;
	}
}






bool TryMove (obj_t *obj)
{
	int x1,y1,xh,yh,x,y;
	
	x1 = (int)(obj->x - obj->r);
	y1 = (int)(obj->y - obj->r);
	xh = (int)(obj->x + obj->r);
	yh = (int)(obj->y + obj->r);
	
	// check for solid walls
	for (y=y1 ; y<=yh ; y++)
		for (x=x1 ; x<=xh ; x++)
		{
			if (map[obj->w][y][x].type == TT_WALL)
				return false;
		}
	
	// TODO check for actors
	
	return true;
}



void ClipMove (obj_t *obj, float xmove, float ymove)
{
	float basex, basey;
	
	basex = obj->x;
	basey = obj->y;
	
	obj->x = basex + xmove;
	obj->y = basey + ymove;
	if (TryMove(obj))
		return;
	
	obj->x = basex + xmove;
	obj->y = basey;
	if (TryMove(obj))
		return;
	
	obj->x = basex;
	obj->y = basey + ymove;
	if (TryMove(obj))
		return;
	
	obj->x = basex;
	obj->y = basey;
}





void Thrust (float angle, float speed)
{
	float xmove, ymove;
		
	xmove = speed * cosf(angle);
	ymove = -speed * sinf(angle);

	player.oldx = player.x;
	player.oldy = player.y;
	player.tilex = (int)player.x;
	player.tiley = (int)player.y;
	
	ClipMove(&player, xmove, ymove);
}




void ControlMovement (obj_t *obj)
{
	float angle;
	
	//
	// forwards/backwards
	//
	if (obj->dy > 0)
	{
		angle = obj->angle + ANG90;
		if (angle < 0)
			angle += ANGLES;
		Thrust (angle,obj->dy);
	}
	else if (obj->dy < 0)
	{
		angle = obj->angle - ANG90;
		if (angle >= ANGLES)
			angle -= ANGLES;
		Thrust (angle,-obj->dy);
	}
	
	//
	// strafe
	//
	if (obj->dx < 0)
	{
		Thrust (obj->angle,-obj->dx); // strafe left
	}
	else if (obj->dx > 0)
	{
		angle = obj->angle + ANGLES/2;
		if (angle >= ANGLES)
			angle -= ANGLES;
		Thrust (angle,obj->dx);		// strafe right
	}
}
