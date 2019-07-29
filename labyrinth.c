//
//  main.c
//  Labyrinth
//
//  Created by Thomas Foster on 2/8/19.
//  Copyright © 2019 Thomas Foster. All rights reserved.
//

#include <math.h>
#include <SDL2_image/SDL_image.h>

#include "labyrinth.h"

#define SHADE 1

SDL_Window 		*window;
SDL_Renderer 	*renderer;
const uint8_t 	*keys;
SDL_Surface 	*walls[WT_COUNT];
SDL_Texture		*walltextures[WT_COUNT];
SDL_Texture		*text;

gamestate_t		gamestate;
const float 	fov = ANG90 / 2;
const float 	depth = 16.0f;
const int		halfheight = WIN_H / 2;
//int				wallindex; // which wall to draw, set by CheckBlock
obj_t 			player;

// 'map' represents the entire "5-dimensional" world:
// an array of NUMDIMS 2D-64*64 areas
tile_t map[NUMDIMS][MAPSIZE][MAPSIZE];




void Quit (const char *error)
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	if (error && *error) {
		printf("Fatal Error! %s: %s\n", error, SDL_GetError());
		exit(1);
	}
	exit(0);
}




//
// ProcessInput
// Process all user input
// App quit
// player rotate/movement
//
void ProcessInput (void)
{
//	const float	adjust = 0.1f;
//	const float strafeadj = 0.5f;
	
	SDL_Event 	ev;
	float		max, min;
	
	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT)
			Quit(NULL);
		if (ev.type == SDL_KEYDOWN)
		{
			switch (ev.key.keysym.sym) {
				case SDLK_ESCAPE:
					Quit(NULL);
					break;
				case SDLK_e:
					if (Ctrl())
						gamestate = GS_EDITOR;
					break;
				default:
					break;
			}
		}
	}
	
	
	// rotate
	if (keys[SDL_SCANCODE_LEFT])
		SetAngle(&player, player.angle + PL_TURN);
	if (keys[SDL_SCANCODE_RIGHT])
		SetAngle(&player, player.angle - PL_TURN);
	
	if (keys[SDL_SCANCODE_W])
		player.dy -= PL_ACCEL; // forward
	else if (keys[SDL_SCANCODE_S])
		player.dy += PL_ACCEL; // backward
	else if (!keys[SDL_SCANCODE_W] && !keys[SDL_SCANCODE_S])
	{
		if (player.dy > 0) {
			player.dy -= PL_ACCEL;
		} else if (player.dy < 0) {
			player.dy += PL_ACCEL;
		}
		if (fabsf(player.dy) < PL_ACCEL*2.0f)
			player.dy = 0;
	}

	if (keys[SDL_SCANCODE_D])
		player.dx += PL_ACCEL; // strafe right
	else if (keys[SDL_SCANCODE_A])
		player.dx -= PL_ACCEL; // strafe left
	else if (!keys[SDL_SCANCODE_A] && !keys[SDL_SCANCODE_D])
	{
		if (player.dx > 0) {
			player.dx -= PL_ACCEL;
		} else if (player.dx < 0) {
			player.dx += PL_ACCEL;
		}
		if (fabsf(player.dx) < PL_ACCEL*2.0f)
			player.dx = 0;
	}
	
	// bound movement
	max = PL_MOVE_SPD;
	min = -max;
	bound(player.dy, min, max);
	max = PL_STRAFE_SPD;
	min = -max;
	bound(player.dx, min, max);
}




bool WallCollision (obj_t *obj)
{
	return (map[obj->w][(int)obj->top][(int)obj->left].type == TT_WALL ||
			map[obj->w][(int)obj->top][(int)obj->right].type == TT_WALL ||
			map[obj->w][(int)obj->bottom][(int)obj->left].type == TT_WALL ||
			map[obj->w][(int)obj->bottom][(int)obj->right].type == TT_WALL);
}




void RenderFloor(uint8_t r, uint8_t g, uint8_t b)
{
	SDL_Rect floor = { 0, halfheight, WIN_W, halfheight };
	SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	SDL_RenderFillRect(renderer, &floor);
	
}




void RenderCeiling(uint8_t r, uint8_t g, uint8_t b)
{
	SDL_Rect ceil = { 0, 0, WIN_W, halfheight };
	SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	SDL_RenderFillRect(renderer, &ceil);
}




//
// CalcHeight
// Calulate wall height
//
int CalcHeight (float xintercept, float yintercept)
{
	float dx, dy;
	float distadj;
	int ceiling, floor;
	
	dx = xintercept - player.x;
	dy = yintercept - player.y;
	distadj = dx * player.sin + dy * player.cos;
	ceiling = (float)halfheight - (WIN_H / distadj);
	floor = WIN_H - ceiling;
	
	return floor - ceiling;
}





void PlayLoop (void)
{
	int 	x, y;
	float	dist;
	int		wallheight;
	tile_t	tile;
	obj_t	ray;
	point	raydir;
	SDL_Surface *s;
	
	// INIT PLAYER
	
	for (int w=0 ; w<NUMDIMS ; w++) {
		for (int y=0 ; y<MAPSIZE ; y++) {
			for (int x=0 ; x<MAPSIZE ; x++) {
				if (map[w][y][x].type == TT_PLAYERSTART) {
					player.x = x + 0.5f;
					player.y = y + 0.5f;
					player.w = w;
				}
			}
		}
	}
	player.r = PL_RADIUS;
	player.oldx = player.x;
	player.oldy = player.y;
	player.ingate = false;
	player.entryside = -1;
	SetAngle(&player, M_PI/2);
	
	ray.type = OT_RAY;
	ray.r = 0;
	
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	if (w != WIN_W*SCALE || h != WIN_H*SCALE)
		SDL_SetWindowSize(window, WIN_W*SCALE, WIN_H*SCALE);
	
	// game loop
	do
	{
		ProcessInput();
		ControlMovement(&player);
		CheckBlock(&player); 	// do collisions and gate stuff
		
		// RENDER
		
		RenderFloor(80, 80, 80);
		RenderCeiling(64, 64, 64);
		
		for (x=0; x < WIN_W; x++)
		{
			// set view angle
			SetAngle(&ray, (player.angle+fov/2.0f) - ((float)x/WIN_W*fov));
			dist = 0;
			ray.w = player.w; // start ray cast in current dimension
			ray.ingate = false;
			raydir = (point){ ray.sin, ray.cos }; // set ray direction (unit vector)
			float samplex = 0;
			
			while (dist < MAPSIZE)
			{
				// extend vector out
				SetPosition(&ray, player.x+raydir.x*dist, player.y+raydir.y*dist);
				tile = map[ray.w][ray.tiley][ray.tilex];
				
				CheckBlock(&ray);
				
				if (tile.type == TT_WALL)
				{
					float angle = atan2f(ray.y-(ray.tiley+0.5f), ray.x-(ray.tilex+0.5f));
					float rc = M_PI*0.25; // upper (or lower) right corner
					float lc = M_PI*0.75;
					
					if ((angle >= -rc && angle < rc) ||
						(angle >= lc || angle < -lc))
					{
						samplex = ray.y - ray.tiley; // hit right or left side
					} else {
						samplex = ray.x - ray.tilex; // hit top or bottom side
					}
					break; // done casting ray
				}
				
				// extend ray distance and check again:
				dist += 0.01f;
			} // while (dist < MAP_W)
			
			// assume tile.type == TT_WALL:
			if (map[ray.w][(int)ray.oldy][(int)ray.oldx].type == TT_GATE_H ||
				map[ray.w][(int)ray.oldy][(int)ray.oldx].type == TT_GATE_V)
			{
				s = walls[WT_FIRE];
			} else {
				s = walls[ray.w];
			}
//			s = ray.ingate ? walls[WT_FIRE] : walls[ray.w];
			
			wallheight = CalcHeight(ray.x, ray.y);
			float ceiling = halfheight-wallheight/2;
			float floor = halfheight+wallheight/2;
			
			// draw walls
			for (y=ceiling ; y<floor ; y++)
			{
				if (y < 0 || y > WIN_H) continue;
				float sampley = ((float)y - ceiling) / (float)wallheight;
				
				SDL_LockSurface(s);
				int pixely = (int)(sampley * s->h);
				int pixelx = (int)(samplex * s->w);
				uint8_t pixel = *((uint8_t *)s->pixels + pixely * s->w + pixelx);
				uint8_t r, g, b;
				SDL_GetRGB(pixel, s->format, &r, &g, &b);
				SDL_SetRenderDrawColor(renderer, r, g, b, 255);
				SDL_RenderDrawPoint(renderer, x, y);
				SDL_UnlockSurface(s);
			}
#if SHADE
			SDL_Rect dark = { x, ceiling, 1, wallheight };
			int alpha = 255-wallheight*2;
			if (alpha < 0)
				alpha = 0;
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
			SDL_RenderFillRect(renderer, &dark);
#endif
		}
		
		SDL_RenderPresent(renderer);
	} while (gamestate == GS_PLAY);
}




int main (void)
{
	int	i;
	
	// INIT SDL, WINDOW, RENDERER
	
	if (SDL_Init(SDL_INIT_VIDEO) != 0) Quit("SDL_Init failed");
	
	window = SDL_CreateWindow("Labyrinth", 0, 0, WIN_W*SCALE, WIN_H*SCALE, 0);
	if (!window) Quit("SDL_CreateWindow failed");
	
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	if (!renderer) Quit("SDL_CreateRenderer failed");
	SDL_RenderSetScale(renderer, SCALE, SCALE);
	
	// INIT SURFACES & TEXTURES
	
	walls[ WT_WOOD ]  	= IMG_Load("assets/wood.png");
	walls[ WT_MARBLE ] 	= IMG_Load("assets/marble.png");
	walls[ WT_FIRE ] 	= IMG_Load("assets/fire.png");
	walls[ WT_TECH ] 	= IMG_Load("assets/tech.png");
	walls[ WT_CEMENT ] 	= IMG_Load("assets/cement.png");
	walls[ WT_STONE ] 	= IMG_Load("assets/stone.png");
	for (i=0 ; i<WT_COUNT ; i++)
	{
		if (!walls[i])
			Quit("Could not load wall surface");
		walltextures[i] = SDL_CreateTextureFromSurface(renderer, walls[i]);
		if (!walltextures[i])
			Quit("Could not load wall texture");
	}
	
	// text
	SDL_Surface *temp = IMG_Load("assets/cgafont.png");
	if (!temp) Quit("Could not load cgafont.png!");
	text = SDL_CreateTextureFromSurface(renderer, temp);
	if (!text) Quit("Could not load font texture!");
	
	// INIT GAME
	
	keys = SDL_GetKeyboardState(NULL);
	gamestate = GS_PLAY;
	OpenMap(1);
	
	while (1)
	{
		switch (gamestate) {
			case GS_EDITOR:
				EditorLoop();
				break;
			case GS_PLAY:
				PlayLoop();
				break;
			default:
				break;
		}
	}
}
