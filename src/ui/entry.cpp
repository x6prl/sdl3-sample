#include "entry.h"

#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <cmath>

extern "C" SDL_AppResult ui_event(AppContext *ctx, SDL_Event *event) {
	// forward input to ImGUI
	ImGui_ImplSDL3_ProcessEvent(event);

	if (event->type == SDL_EVENT_QUIT) {
		ctx->app_quit = SDL_APP_SUCCESS;
	}

	// On mobile, the "Back" button usually sends a KeyDown/KeyUp for
	// SDL_SCANCODE_AC_BACK
	if (event->type == SDL_EVENT_KEY_DOWN &&
	    event->key.scancode == SDL_SCANCODE_AC_BACK) {
		ctx->app_quit = SDL_APP_SUCCESS; // Exit app on back button
	}
	return SDL_APP_CONTINUE;
}

extern "C" SDL_AppResult ui_iterate(AppContext *ctx) {
	if (ctx->render_tiger_switch) {
		auto time = SDL_GetTicks() / 1000.f;
		auto red = (std::sin(time) + 1) / 2.0 * 255;
		auto green = (std::sin(time / 2) + 1) / 2.0 * 255;
		auto blue = (std::sin(time) * 2 + 1) / 2.0 * 255;

		// SDL_Log("%f %f %f", red, green, blue);

		SDL_SetRenderDrawColor(ctx->renderer, red, green, blue,
		                       SDL_ALPHA_OPAQUE);
		SDL_RenderClear(ctx->renderer);
		SDL_RenderTexture(ctx->renderer, ctx->imageTex, NULL, NULL);
		SDL_RenderTexture(ctx->renderer, ctx->messageTex, NULL,

		                  &ctx->messageDest);
	} else {
		SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(ctx->renderer);
	}

	ImGuiIO &io = ImGui::GetIO();

	// 1. Start the Dear ImGui frame
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	// Force the ImGui window to cover the entire OS window (no title bar, no
	// resize)
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2));
	// Use this to fulfill the screen:
	// ImGui::SetNextWindowSize(io.DisplaySize);
	ImGui::SetNextWindowSize({io.DisplaySize.x / 2, io.DisplaySize.y / 2});
	ImGuiWindowFlags flags =
		  ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
		  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

	// 2. Build your GUI
	if (ImGui::Begin("MobileShell", nullptr, flags)) {

		// --- UI HEADER ---
		ImGui::TextDisabled("SDL3 + ImGui");
		ImGui::SameLine();
		ImGui::Text("FPS: %.1f", io.Framerate);
		ImGui::Separator();
		ImGui::Spacing();

		// --- BIG ACTION BUTTON ---
		// ImVec2(-1, 100) -> Width = Full Window Width, Height = 100px
		if (ImGui::Button("TAP ME!", ImVec2(-1.0f, 100.0f))) {
			ctx->counter++;
		}

		// Centered text
		float windowWidth = ImGui::GetWindowSize().x;
		float textWidth = ImGui::CalcTextSize("Count: 000").x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::Text("Count: %d", ctx->counter);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// --- TOGGLE SWITCH (Checkbox styled as large toggle) ---
		// Standard checkboxes are small. Let's use a button
		// that toggles.
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
		                      ImVec4{0.2f, 0.8f, 0.9f, 1.0f});
		if (ctx->render_tiger_switch) {
			ImGui::PushStyleColor(ImGuiCol_Button,
			                      ImVec4{0.2f, 0.9f, 0.2f, 1.0f});
		} else {
			ImGui::PushStyleColor(ImGuiCol_Button,
			                      ImVec4{0.2f, 0.3f, 0.4f, 1.0f});
		}
		const char *toggle_text =
			  ctx->render_tiger_switch ? "Tiger: ON" : "Tiger: OFF";
		if (ImGui::Button(toggle_text, ImVec2(-1.0f, 60.0f))) {
			ctx->render_tiger_switch = !ctx->render_tiger_switch;
		}
		ImGui::PopStyleColor(2);
	}
	ImGui::End();

	// 3. Render
	ImGui::Render();

	// Draw ImGui data
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), ctx->renderer);

	// Present
	SDL_RenderPresent(ctx->renderer);

	return ctx->app_quit;
}
