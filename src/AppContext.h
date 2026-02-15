#pragma once

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <atomic>

struct AppContext {
	SDL_Window *window;
	SDL_Renderer *renderer;
	bool render_tiger_switch;
	int counter;
	SDL_Texture *messageTex, *imageTex;
	SDL_FRect messageDest;
	SDL_AudioDeviceID audioDevice;
	MIX_Track *track;
	SDL_AppResult app_quit = SDL_APP_CONTINUE;
};
