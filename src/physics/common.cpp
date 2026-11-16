#include "common.hpp"

#include "../ui.hpp"

void Grid::Update(std::span<glm::vec3> positions) {
    m_size = glm::ivec3(g_ui.box_size / CELL_SIZE) + 1;

    m_particle_cell_idx.assign(positions.size(), 0);
    m_particle_ids.resize(positions.size());

    m_num_particles.assign(CellNum(), 0);
    m_first_particles.assign(CellNum(), 0);
    m_cells.resize(CellNum());

    // Count particles per cell
    for (uint32_t p = 0; p < positions.size(); p++) {
        uint32_t cell_idx = CellIndexFromPos(positions[p]);
        m_particle_cell_idx[p] = cell_idx;
        m_num_particles[cell_idx]++;
    }

    // Prefix sums
    uint32_t first = 0;
    for (uint32_t c = 0; c < CellNum(); c++) {
        first += m_num_particles[c];
        m_first_particles[c] = first;
    }

    // Populate particle ID array and turn prefix sums into start indices
    for (uint32_t p = 0; p < positions.size(); p++) {
        uint32_t cell_idx = m_particle_cell_idx[p];
        m_first_particles[cell_idx]--;
        m_particle_ids[m_first_particles[cell_idx]] = p;
    }
}

