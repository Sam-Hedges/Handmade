#ifndef HANDMADE_PLATFORM_H
#define HANDMADE_PLATFORM_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>

#define internal static
#define local_persist static
#define global_var static

typedef Uint8 uint8;
typedef Uint16 uint16;
typedef Uint32 uint32;
typedef Uint64 uint64;

typedef Sint8 int8;
typedef Sint16 int16;
typedef Sint32 int32;
typedef Sint64 int64;

global_var float PI	 = 3.14159265359;
global_var float TAU = 2 * PI;

struct platform_program_state
{
	bool IsRunning;
	SDL_Event LastEvent;
};

#endif // HANDMADE_PLATFORM_H
