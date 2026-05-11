#pragma once

#include <SDL3/SDL_dialog.h>

#include <string>
#include <string_view>

struct SDL_Window;

namespace dusk {

using FileCallback = void (*)(void* userdata, const char* path, const char* error);

void ShowFileSelect(FileCallback callback, void* userdata, SDL_Window* window,
    const SDL_DialogFileFilter* filters, int nfilters, const char* default_location,
    bool allow_many);
void ShowFileSave(FileCallback callback, void* userdata, SDL_Window* window,
    const SDL_DialogFileFilter* filters, int nfilters, const char* default_location);

std::string display_name_for_path(std::string_view path);
std::string read_file_select_text(std::string_view path);
void write_file_select_text(std::string_view path, std::string_view text);

}  // namespace dusk
