#include "internal.hpp"
#include "../ui.hpp"

void Grid::Update(std::span<glm::dvec3> positions) {
    m_size = g_ui.box_size * INV_CELL_SIZE;

    m_stride_y = m_size.x;
    m_stride_z = m_size.x * m_size.y;
    const uint32_t cell_num = m_size.x * m_size.y * m_size.z;

    m_num.assign(cell_num, 0);

    // Count particles per cell. Cache each particle's cell so the fill
    // pass below does not have to recompute it.
    m_cell_idx.resize(positions.size());
    for (uint32_t particle = 0; particle < positions.size(); particle++) {
        const uint32_t cell_idx = CellIdxFromPos(positions[particle]);
        m_cell_idx[particle] = cell_idx;
        m_num[cell_idx]++;
    }

    // Exclusive prefix sums (with one guard element at the end).
    m_first.resize(cell_num + 1);
    uint32_t first = 0;
    for (uint32_t cell = 0; cell < cell_num; cell++) {
        first += m_num[cell];
        m_first[cell] = first;
    }
    m_first[cell_num] = first;

    // Populate the particle id array and turn the prefix sums into start indices.
    m_ids.resize(positions.size());
    for (uint32_t p = 0; p < positions.size(); p++) {
        const uint32_t cell_idx = m_cell_idx[p];
        m_first[cell_idx]--;
        m_ids[m_first[cell_idx]] = p;
    }
}

std::span<uint32_t> Grid::GetCellParticleIDs(uint32_t cell_idx) {
    return std::span(m_ids.data() + m_first[cell_idx], m_num[cell_idx]);
}

