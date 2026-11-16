#pragma once

namespace gpu {
    struct Context {
        Context(SDL_Window* window);
        ~Context();
    };

    struct Buffer {
        Buffer(const SDL_GPUBufferCreateInfo& create_info);
        ~Buffer();

        Buffer(Buffer&& other);
        Buffer& operator=(Buffer&& other);

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        void Resize(u32 new_size);
        SDL_GPUBuffer* Ptr() { return m_ptr; };
    private:
        SDL_GPUBuffer* m_ptr   = nullptr;
        u32            m_size  = 0;
        u32            m_usage = 0;
    };

    template<typename T>
    struct Texture {
        Texture(const SDL_GPUTextureCreateInfo& create_info);
        ~Texture();

        Texture(Texture&&);
        Texture& operator=(Texture&&);

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        void Resize(u32 new_size);
        SDL_GPUTexture* Ptr();
    private:
        consteval SDL_GPUTextureFormat FormatFromType(auto v) {
            if constexpr (std::is_same_v<T, float>) return SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
            if constexpr (std::is_same_v<T, Vec4>)  return SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
        }
        glm::ivec3 m_size;
        glm::ivec3 m_capacity;
    };

    struct TransferBuffer {
        TransferBuffer(const SDL_GPUTransferBufferCreateInfo& create_info);
        ~TransferBuffer();

        TransferBuffer(TransferBuffer&&);
        TransferBuffer& operator=(TransferBuffer&&);

        TransferBuffer(const TransferBuffer&) = delete;
        TransferBuffer& operator=(const TransferBuffer&) = delete;

        void Resize(u32 new_size);
        SDL_GPUBuffer* Ptr();
        void Stage(std::function<void(void* mem)> f);
    };
}

