#pragma once

SDL_GPUShader* CreateShader(const std::string& path);

SDL_GPUTexture* CreateDepthTexture(u32 w, u32 h);

struct BufferUploadData {
    u32 offset;
    std::span<byte> data;
};
void UploadToBuffer(SDL_GPUCommandBuffer* command_buffer, SDL_GPUBuffer* buffer, std::span<BufferUploadData> upload_data);

