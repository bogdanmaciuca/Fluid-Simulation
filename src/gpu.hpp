#pragma once

namespace gpu {
    /*
    * Initializes a GPU device, claims a window for it and
    * initializes glslang
    * `Context` must be constructed before any GPU objects are created and
    * destroyed after all GPU objects are released.
    */
    struct Context {
        Context(SDL_Window* window);
        ~Context();

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context&&) = delete;

        static SDL_GPUDevice* GetDevice() { return s_device; }
    private:
        inline static bool           s_is_init    = false;
        inline static SDL_GPUDevice* s_device     = nullptr;
        inline static SDL_Window*    s_window_ptr = nullptr;
    };

    template <
        typename Handle,
        typename Info,
        Handle* (*Create)(SDL_GPUDevice*, const Info*),
        void    (*Release)(SDL_GPUDevice*, Handle*)
    >
    struct Resource {
        Resource(const Info& create_info) {
            m_ptr = Create(GetDevice(), &create_info);
            assert(m_ptr != nullptr);
        }

        ~Resource() {
            if (m_ptr != nullptr) {
                Release(GetDevice(), m_ptr);
            }
        }

        Resource(Resource&& other) noexcept : m_ptr(other.m_ptr) {
            other.m_ptr = nullptr;
        }

        Resource& operator=(Resource&& other) noexcept {
            if (this != &other) {
                if (m_ptr != nullptr) {
                    Release(GetDevice(), m_ptr);
                }
                m_ptr       = other.m_ptr;
                other.m_ptr = nullptr;
            }
            return *this;
        }

        Resource(const Resource&) = delete;
        Resource& operator=(const Resource&) = delete;

        Handle* Ptr() { return m_ptr; }

    protected:
        Handle* m_ptr = nullptr; // SDL handle
    };

    struct Buffer : Resource<
        SDL_GPUBuffer,
        SDL_GPUBufferCreateInfo,
        SDL_CreateGPUBuffer,
        SDL_ReleaseGPUBuffer
    > {
        using Resource::Resource;
    };

    struct Texture : Resource<
        SDL_GPUTexture,
        SDL_GPUTextureCreateInfo,
        SDL_CreateGPUTexture,
        SDL_ReleaseGPUTexture
    > {
        using Resource::Resource;
    };

    struct TransferBuffer : Resource<
        SDL_GPUTransferBuffer,
        SDL_GPUTransferBufferCreateInfo,
        SDL_CreateGPUTransferBuffer,
        SDL_ReleaseGPUTransferBuffer
    > {
        using Resource::Resource;

        void Map(std::function<void(void* mem)> f);
    };
}
