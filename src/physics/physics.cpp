#include "physics.hpp"

std::tuple<std::vector<uint32_t>, std::vector<uint32_t>> GenerateGrid(
    const std::vector<glm::vec3>& positions,
    const glm::ivec3& cell_num,
    float inv_spacing
) {
    const int      nx       = cell_num.x;
    const int      ny       = cell_num.y;
    const int      nz       = cell_num.z;

    uint64_t cell_total = 1;
    for (const int dimension : { nx, ny, nz }) {
        cell_total *= static_cast<uint32_t>(dimension);
    }

    const uint32_t particle_num = static_cast<uint32_t>(positions.size());
    const uint32_t stride_y = static_cast<uint32_t>(nx);
    const uint32_t stride_z = stride_y * static_cast<uint32_t>(ny);

    std::vector<uint32_t> cell_num_particles(static_cast<size_t>(cell_total), 0);
    std::vector<uint32_t> particle_cell_idx(particle_num);
    std::vector<uint32_t> cell_first_particles(static_cast<size_t>(cell_total) + 1);
    std::vector<uint32_t> cell_particle_ids(particle_num);

    const auto CellIndex = [=](int x, int y, int z) {
        return static_cast<uint32_t>(z) * stride_z
             + static_cast<uint32_t>(y) * stride_y
             + static_cast<uint32_t>(x);
    };

    const auto CellPosFromPos = [=](const glm::vec3& p) {
        return glm::ivec3(
            std::clamp(static_cast<int>(std::floor(p.x * inv_spacing)), 0, nx - 1),
            std::clamp(static_cast<int>(std::floor(p.y * inv_spacing)), 0, ny - 1),
            std::clamp(static_cast<int>(std::floor(p.z * inv_spacing)), 0, nz - 1)
        );
    };

    const auto CellIndexFromPos = [&](const glm::vec3& p) {
        const glm::ivec3 c = CellPosFromPos(p);
        return CellIndex(c.x, c.y, c.z);
    };

    // Count particles per cell.
    for (uint32_t particle = 0; particle < particle_num; particle++) {
        const uint32_t cell_idx = CellIndexFromPos(positions[particle]);
        particle_cell_idx[particle] = cell_idx;
        cell_num_particles[cell_idx]++;
    }

    // Exclusive prefix sums (with one guard element at the end).
    uint32_t first = 0;
    for (uint32_t cell_idx = 0; cell_idx < cell_total; cell_idx++) {
        first += cell_num_particles[cell_idx];
        cell_first_particles[cell_idx] = first;
    }
    cell_first_particles[cell_total] = first;

    // Populate particle id array and convert prefix sums into start indices.
    for (uint32_t particle = 0; particle < particle_num; particle++) {
        const uint32_t cell_idx = particle_cell_idx[particle];
        cell_first_particles[cell_idx]--;
        cell_particle_ids[cell_first_particles[cell_idx]] = particle;
    }

    return { std::move(cell_first_particles), std::move(cell_particle_ids) };
}