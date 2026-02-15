#include "SDL3/SDL_events.h"
#include "SDL3/SDL_log.h"
#include <cstdio>
#include <string>
#define SDL_MAIN_USE_CALLBACKS // This is necessary for the new callbacks API.
                               // To use the legacy API, don't define this.
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <filesystem>
#include <thread>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include "hotreload.h"

#include "AppContext.h"

constexpr auto UI_UPDATE_EVENT_TIME = 100;

constexpr uint32_t windowStartWidth = 400;
constexpr uint32_t windowStartHeight = 400;

SDL_AppResult SDL_Fail() {
	SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
	return SDL_APP_FAILURE;
}

// Simple timer callback to keep the UI "alive"
uint32_t SDLCALL WakeUpTimer(void *userdata, uint32_t timerID,
                             uint32_t interval) {
	SDL_Event event;
	SDL_zero(event);
	event.type = SDL_EVENT_USER;
	SDL_PushEvent(&event);
	return interval; // Keep running
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
	// init the library, here we make a window so we only need the Video
	// capabilities.
	if (not SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		return SDL_Fail();
	}

	// init TTf
	if (not TTF_Init()) {
		return SDL_Fail();
	}

	// init Mixer
	if (not MIX_Init()) {
		return SDL_Fail();
	}

	// create a window

	SDL_Window *window = SDL_CreateWindow(
		  "SDL Minimal Sample", windowStartWidth, windowStartHeight,
		  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
	// 0, 0,
	// SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
	if (not window) {
		return SDL_Fail();
	}

	// create a renderer
	SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
	if (not renderer) {
		return SDL_Fail();
	}

	// load the font
#if __ANDROID__
	std::filesystem::path basePath =
		  ""; // on Android we do not want to use basepath. Instead, assets are
	          // available at the root directory.
#else
	auto basePathPtr = SDL_GetBasePath();
	if (not basePathPtr) {
		return SDL_Fail();
	}
	const std::filesystem::path basePath = basePathPtr;
#endif

	const auto fontPath = basePath / "Inter-VariableFont.ttf";
	TTF_Font *font = TTF_OpenFont(fontPath.string().c_str(), 36);
	if (not font) {
		return SDL_Fail();
	}

	// render the font to a surface
	const std::string_view text = "Hello SDL I say!";
	SDL_Surface *surfaceMessage = TTF_RenderText_Solid(
		  font, text.data(), text.length(), {255, 255, 255});

	// make a texture from the surface
	SDL_Texture *messageTex =
		  SDL_CreateTextureFromSurface(renderer, surfaceMessage);

	// we no longer need the font or the surface, so we can destroy those now.
	TTF_CloseFont(font);
	SDL_DestroySurface(surfaceMessage);

	// load the SVG
	auto svg_surface = IMG_Load((basePath / "gs_tiger.svg").string().c_str());
	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, svg_surface);
	SDL_DestroySurface(svg_surface);

	// get the on-screen dimensions of the text. this is necessary for rendering
	// it
	auto messageTexProps = SDL_GetTextureProperties(messageTex);
	SDL_FRect text_rect{
		  .x = 0,
		  .y = 0,
		  .w = float(SDL_GetNumberProperty(messageTexProps,
	                                       SDL_PROP_TEXTURE_WIDTH_NUMBER, 0)),
		  .h = float(SDL_GetNumberProperty(messageTexProps,
	                                       SDL_PROP_TEXTURE_HEIGHT_NUMBER, 0))};

	// init SDL Mixer
	MIX_Mixer *mixer =
		  MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
	if (mixer == nullptr) {
		return SDL_Fail();
	}

	auto mixerTrack = MIX_CreateTrack(mixer);

	// load the music
	auto musicPath = basePath / "the_entertainer.ogg";
	auto music = MIX_LoadAudio(mixer, musicPath.string().c_str(), false);
	if (not music) {
		return SDL_Fail();
	}

	// play the music (does not loop)
	MIX_SetTrackAudio(mixerTrack, music);
	MIX_PlayTrack(mixerTrack, 0);

	// print some information about the window
	SDL_ShowWindow(window);
	{
		int width, height, bbwidth, bbheight;
		SDL_GetWindowSize(window, &width, &height);
		SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
		SDL_Log("Window size: %ix%i", width, height);
		SDL_Log("Backbuffer size: %ix%i", bbwidth, bbheight);
		if (width != bbwidth) {
			SDL_Log("This is a highdpi environment.");
		}
	}

	// set up the application data
	auto state = new AppContext{
		  .window = window,
		  .renderer = renderer,
		  .render_tiger_switch = true,
		  .counter = 0,
		  .messageTex = messageTex,
		  .imageTex = tex,
		  .messageDest = text_rect,
		  .track = mixerTrack,
	};
	*appstate = state;

	// load app_hotreload
	#if HOTRELOAD
		if (!hotreload(state)) {
			SDL_LogError(SDL_LOG_CATEGORY_CUSTOM,
			             "Failed to load hotreload module from %s",
			             HOTRELOAD_MODULE_PATH);
			return SDL_APP_FAILURE;
		}
	#endif

	SDL_SetRenderVSync(renderer, -1); // enable vysnc

	// setup ImGUI context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |=
		  ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

	ImGui::StyleColorsLight();

#if defined(__ANDROID__) || defined (__APPLE__) && TARGET_OS_IPHONE == 1
	// --- MOBILE SCALING MAGIC ---
	// Mobile screens have high DPI. We need to scale everything up.
	// In a real app, you would load a TTF font at a large size (e.g., 48px).
	// For this demo, we scale the style and the font render.
	float scale_factor =
		  3.0f; // Adjust this based on screen density (2.0 to 4.0 usually)
	ImGui::GetStyle().ScaleAllSizes(scale_factor);
	io.FontGlobalScale = scale_factor;
#endif
	// ----------------------------

	// setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForSDLRenderer(state->window, state->renderer);
	ImGui_ImplSDLRenderer3_Init(state->renderer);

	// redraw only on events
	SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "waitevent");
	// add timer event to allow animations
	SDL_AddTimer(UI_UPDATE_EVENT_TIME, WakeUpTimer,
	             nullptr); // Wake up 10 times a second

	SDL_Log("Application started successfully!");

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
	auto *app = (AppContext *)appstate;
#if HOTRELOAD
	if (SDL_EVENT_USER == event->type) {
		if (!hotreload(app)) {
			SDL_Log("Failed to reload app_hotreload from %s",
			        HOTRELOAD_MODULE_PATH);
			// return SDL_APP_CONTINUE;
		}
	}
#endif
	return ui_event(app, event);
}

// NOTE: When "waitevent" is set, this callback is only called _after_
// SDL_AppEvent https://wiki.libsdl.org/SDL3/SDL_HINT_MAIN_CALLBACK_RATE
SDL_AppResult SDL_AppIterate(void *appstate) {
	// SDL_Log(__PRETTY_FUNCTION__);
	auto *app = (AppContext *)appstate;

	return ui_iterate(app);
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	auto *app = (AppContext *)appstate;
	// OS will destroy everything itself on exit
	
	// if (app) {
	//   SDL_DestroyRenderer(app->renderer);
	//   SDL_DestroyWindow(app->window);
	//
	//   // prevent the music from abruptly ending.
	MIX_StopTrack(app->track, MIX_TrackMSToFrames(app->track, 1000));
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//   // Mix_FreeMusic(app->music); // this call blocks until the music has
	//   // finished fading
	//   SDL_CloseAudioDevice(app->audioDevice);
	//
	//   delete app;
	// }
	// TTF_Quit();
	// MIX_Quit();
	//
	SDL_Log("Application quit successfully!");
	SDL_Quit();
}
