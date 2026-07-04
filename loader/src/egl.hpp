#pragma once

#include <string>
#include <vector>

bool init_headless_egl(int width, int height);
void cleanup_egl();
void save_screenshot_to_ppm(const std::string &filename, int width, int height);
std::vector<unsigned char> get_screenshot_pixels_rgb(int width, int height);
