#pragma once

SDL_GPUShader* CreateShader(const std::string& path);

SDL_GPUTexture* CreateDepthTexture(u32 w, u32 h);

