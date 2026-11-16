#pragma once

constexpr float TIMESTEP      = 1.0f / 60.0f;
constexpr float CELL_SIZE     = 0.1f;

namespace kernel {
    constexpr float h = CELL_SIZE / 2.0f;
    static_assert(h <= CELL_SIZE);
    constexpr float h_2 = h * h;
    constexpr float h_3 = h * h * h;
    constexpr float h_9 = h_3 * h_3 * h_3;
    constexpr float W_coeff  = 315.0f / (64.0f * M_PI * h_9);

    inline float Poly6(float r_2) {
        using namespace kernel;
        if (r_2 >= h_2) {
            return 0.0f;
        }

        float d_2 = h_2 - r_2;
        return W_coeff * d_2 * d_2 * d_2;
    }

    inline glm::vec3 SpikyGradient() {

    }
}


struct Grid {
    void Update(std::span<glm::vec3> positions);

    std::span<glm::vec4> GetCells() { return m_cells; }
private:
    glm::ivec3             m_size;

    std::vector<uint32_t>  m_particle_cell_idx; // Cache what cell each particle belongs to
    std::vector<uint32_t>  m_particle_ids;      // Particle IDs in belonging cell order
    std::vector<uint32_t>  m_num_particles;     // Number of particles in each cell
    std::vector<uint32_t>  m_first_particles;   // First particle index in particle vector

    std::vector<glm::vec4> m_cells;             // Position + occupancy (for rendering)

    size_t CellNum() { return m_size.x * m_size.y * m_size.z; }

    uint32_t CellIndex(const glm::ivec3& p) {
        return p.z * m_size.z + p.y * m_size.y + p.x;
    };

    glm::ivec3 CellPosFromPos(const glm::vec3& p) {
        return glm::ivec3(
            std::clamp(int(std::floor(p.x / CELL_SIZE)), 0, m_size.x - 1),
            std::clamp(int(std::floor(p.y / CELL_SIZE)), 0, m_size.y - 1),
            std::clamp(int(std::floor(p.z / CELL_SIZE)), 0, m_size.z - 1)
        );
    }

    uint32_t CellIndexFromPos(const glm::vec3& p) {
        return CellIndex(CellPosFromPos(p));
    }
};

