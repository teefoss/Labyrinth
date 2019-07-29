//
//  editor.c
//  Labyrinth
//
//  Created by Thomas Foster on 7/15/19.
//  Copyright © 2019 Thomas Foster. All rights reserved.
//

#include <sys/stat.h>
#include <string.h>
#include "labyrinth.h"

#define TILESIZE         8
#define tilecoord(x)    (x<<3)
#define viewcoord(x)    (x>>3)
#define drawx(x)        (x)*TILESIZE-originx // tile coord to pixel w screen offset
#define drawy(y)         (y)*TILESIZE-originy

#define FONT_W             8
#define FONT_H             8
#define FILE_FORMAT     "map%02d.lab"

#define EDITOR_WIN_W    512
#define EDITOR_WIN_H    320

// editor panels
#define MENU_H             8
#define MAPAREA_H        EDITOR_WIN_H-MENU_H

enum
{
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
};

const SDL_Rect     maparea = { 0, 0, EDITOR_WIN_W, EDITOR_WIN_H-MENU_H };
const SDL_Rect     menu = { 0, MAPAREA_H, EDITOR_WIN_W, MENU_H };
const char         symbols[TT_COUNT] = { ' ', 'P', 'W', 29, 18 };
const SDL_Color colors[] =
{
    {   0,   0, 170 },
    {   0, 170,   0 },
    {   0, 170, 170 },
    { 170,   0,   0 },
    { 170,   0, 170 },
};

char filename[80];
int mapnum;
int originx; // in tile coords
int originy;
int csrx;
int csry;

int dim;
int selected;

bool FileExists (const char *name)
{
    struct stat buffer;
    return (stat(name, &buffer) == 0);
}

bool Ctrl ()
{
    return keys[SDL_SCANCODE_RCTRL] || keys[SDL_SCANCODE_LCTRL];
}

bool Shift ()
{
    return keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
}

void GetMouseTile (int *x, int *y)
{
    SDL_GetMouseState(x, y);
    *x = (*x / SCALE + originx) / TILESIZE;
    *y = (*y / SCALE + originy) / TILESIZE;
}

void UpdateWindowTitle ()
{
    int x, y;
    char title[80];
    
    GetMouseTile(&x, &y);
    sprintf(title, "%s | %02d, %02d", filename, x, y);
    SDL_SetWindowTitle(window, title);
}




void OpenMap (int number)
{
    int     x, y;
    FILE    *stream;
    
    // TODO level number range check
    sprintf(filename, FILE_FORMAT, number);
    
    if (!FileExists(filename))
    {
        for (dim=0 ; dim<NUMDIMS ; dim++) {
            for (y=0 ; y<MAPSIZE ; y++) {
                for (x=0 ; x<MAPSIZE ; x++) {
                    map[dim][y][x].type = TT_WALL;
                    map[dim][y][x].id = dim;
                }
            }
        }
        stream = fopen(filename, "wb"); // create the file
        fclose(stream);
        printf("OpenMap: Created map file %s\n", filename);
    }
    else // open and load existing file:
    {
        stream = fopen(filename, "rb");
        if (!stream)
            Quit("OpenMap: Error, could not open file.");
        fread(map, sizeof(map), 1, stream);
        fclose(stream);
        printf("OpenMap: Loaded %s\n", filename);
    }
    
    UpdateWindowTitle();
    mapnum = number;
    dim = 0;
}




void SaveMap ()
{
    FILE *stream;
    
    //    sprintf(filename, FILE_FORMAT, mapnum);
    
    stream = fopen(filename, "wb");
    if (!stream) {
        printf("SaveMap: Warning! Could not open file %s\n", filename);
        return;
    }
    fwrite(map, sizeof(map), 1, stream);
    fclose(stream);
    printf("SaveMap: Saved map to file %s\n", filename);
}




//
// MouseTile
// Returns a pointer to the map tile currently
// under the mouse pointer
//
tile_t *MouseTile ()
{
    int x, y;
    GetMouseTile(&x, &y);
    return &map[dim][y][x];
}




void SetTile (tiletype_t type, int id)
{
    tile_t *tile = MouseTile();
    tile->type = type;
    tile->id = id;
}




void DoKeyDown (SDL_Keycode key)
{
    switch (key)
    {
        case SDLK_ESCAPE:    Quit(NULL); break;
            
        case SDLK_UP:         originy-=TILESIZE; break;
        case SDLK_DOWN:        originy+=TILESIZE; break;
        case SDLK_LEFT:     originx-=TILESIZE; break;
        case SDLK_RIGHT:    originx+=TILESIZE; break;
            
        case SDLK_s:         if (Ctrl()) SaveMap(); break;
        case SDLK_r:
        case SDLK_p:        if (Ctrl()) gamestate = GS_PLAY; break;
            
        case SDLK_TAB:
            if (Shift()) {
                if (--selected < 0)
                    selected += TT_COUNT;
            } else
                selected = (selected + 1) % TT_COUNT;
            break;
            
        default:
            break;
    }
    if (key >= '1' && key <= '5')
        dim = key - '1';
}





void DoMouseDown (uint8_t button, int x, int y)
{
    SDL_Point   clickpt = { x, y };
    SDL_Rect    maprect = maparea;
    SDL_Rect    menurect = menu;
    
    maprect.w *= SCALE;
    maprect.h *= SCALE;
    menurect.y *= SCALE;
    menurect.w *= SCALE;
    menurect.h *= SCALE;
    
    switch (button) {
        case SDL_BUTTON_LEFT:
            if (SDL_PointInRect(&clickpt, &maprect))
            {
                if (keys[SDL_SCANCODE_X])
                    SetTile(TT_EMPTY, 0);
                else
                    SetTile(selected, 0);
            } else if (SDL_PointInRect(&clickpt, &menurect)) {
                selected = clickpt.x / SCALE / TILESIZE;
                if (selected >= TT_COUNT)
                    selected = TT_COUNT-1;
            }
            break;
            
        default:
            break;
    }
}




#pragma mark - Text Printing

void gotoxy (int x, int y)
{
    csrx = x;
    csry = y;
}

// print char at cursor location
void printc (int x, int y, int ch)
{
    gotoxy(x, y);
    SDL_Rect src = { (ch%32)*FONT_W, ch/32*FONT_H, FONT_W, FONT_H };
    SDL_Rect dst = { x*FONT_W, y*FONT_H, FONT_W, FONT_H };
    SDL_RenderCopy(renderer, text, &src, &dst);
}


#if 0
// print char centered at tile x, y
void drawchar (int x, int y, int ch)
{
    SDL_Rect src = { (ch%16)*FONT_W, ch/16*FONT_H, FONT_W, FONT_H };
    SDL_Rect dst = { x+2, y, FONT_W, FONT_H };
    SDL_RenderCopy(renderer, text, &src, &dst);;
}
#endif

// print string at cursor x, y
void print (int x, int y, const char *string)
{
    gotoxy(x, y);
    char *c = (char *)string;
    while (*c != '\0')
    {
        printc(csrx, csry, *c);
        c++;
        csrx++;
    }
}

// print integer at cursor x, y
void printd (int x, int y, int d)
{
    gotoxy(x, y);
    char buffer[80];
    sprintf(buffer, "%d", d);
    print(x, y, buffer);
}

#pragma mark -




// x and y are tile coords!
void DrawTileType (tiletype_t t, int x, int y)
{
    SDL_Rect    dst;
    SDL_Color   *c;
    
    dst = (SDL_Rect){ drawx(x), drawy(y), TILESIZE, TILESIZE };
    
    switch (t) {
        case TT_EMPTY:
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &dst);
            break;
        case TT_WALL:
            c = (SDL_Color *)&colors[dim];
            SDL_SetRenderDrawColor(renderer, c->r, c->g, c->b, 255);
            SDL_RenderFillRect(renderer, &dst);
            break;
            
        default:
            printc(x-originx/TILESIZE, y-originy/TILESIZE, symbols[t]);
            break;
    }
}




void EditorLoop()
{
    SDL_Event   event;
    SDL_Rect    dst;
    uint32_t    mousestate;
    SDL_Point   clickpt;
    int         x, y;
    SDL_Rect    mapconv = maparea;
    SDL_Rect    menuconv = menu;
    
    selected = TT_PLAYERSTART;
    mapconv.w *= SCALE;
    mapconv.h *= SCALE;
    menuconv.y *= SCALE;
    menuconv.w *= SCALE;
    menuconv.h *= SCALE;
    
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    if (w != EDITOR_WIN_W*SCALE || h != EDITOR_WIN_H*SCALE)
        SDL_SetWindowSize(window, EDITOR_WIN_W*SCALE, EDITOR_WIN_H*SCALE);
    
    do
    {
        SDL_PumpEvents();
        
        //
        // KEYBOARD INPUT
        //
        
        while (SDL_PollEvent(&event))
        {
            switch (event.type) {
                case SDL_QUIT:
                    Quit(NULL);
                    break;
                case SDL_KEYDOWN:
                    DoKeyDown(event.key.keysym.sym);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    //DoMouseDown(event.button.button, event.motion.x, event.motion.y);
                    break;
                case SDL_MOUSEMOTION:
                    UpdateWindowTitle();
                default:
                    break;
            }
        }
        
        
        //
        // MOUSE INPUT
        //
        
        mousestate = SDL_GetMouseState(&clickpt.x, &clickpt.y);
        
        // handle left click
        if (mousestate & SDL_BUTTON_LMASK)
        {
            if (SDL_PointInRect(&clickpt, &mapconv))
            {
                if (keys[SDL_SCANCODE_X])
                    SetTile(TT_EMPTY, 0);
                else
                    SetTile(selected, 0);
            } else if (SDL_PointInRect(&clickpt, &menuconv)) {
                selected = clickpt.x / SCALE / TILESIZE;
                if (selected >= TT_COUNT)
                    selected = TT_COUNT-1;
            }
        }
        
        //
        // RENDER
        //
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // MAP AREA
        
        SDL_RenderSetViewport(renderer, &maparea);
        
        // draw map
        for (y=0 ; y<MAPSIZE ; y++) {
            for (x=0 ; x<MAPSIZE ; x++)
            {
                DrawTileType(map[dim][y][x].type, x, y);
            }
        }
        
        // draw grid
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        for (y=0 ; y<MAPSIZE*TILESIZE ; y+=TILESIZE) {
            for (x=0 ; x<MAPSIZE*TILESIZE ; x+=TILESIZE) {
                SDL_RenderDrawPoint(renderer, x-originx, y-originy);
            }
        }
        
        // MENU AREA
        SDL_RenderSetViewport(renderer, &menu);
        for (x=0 ; x<TT_COUNT ; x++) {
            printc(x, 0, symbols[x]);
        }
        
        // selection box
        dst = (SDL_Rect){ selected*TILESIZE, 0, TILESIZE, TILESIZE};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &dst);
        
        SDL_RenderPresent(renderer);
        
    } while (gamestate == GS_EDITOR);
    
    SDL_RenderSetViewport(renderer, NULL);
    
}
