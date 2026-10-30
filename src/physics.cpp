#include "ui.hpp"
#include "physics.hpp"
#include "timer.hpp"

#include <cmath>

namespace {
    constexpr float OVER_RELAXATION = 1.9f;

    constexpr float INITIAL_WATER_WIDTH  = 0.6f;
    constexpr float INITIAL_WATER_HEIGHT = 0.8f;
}

Physics::Physics() {
    m_pos.reserve(MAX_PARTICLE_NUM);
    m_color.reserve(MAX_PARTICLE_NUM);
    m_particle_vel.reserve(MAX_PARTICLE_NUM);
    m_cell_particle_ids.reserve(MAX_PARTICLE_NUM);
    m_particle_cell_idx.reserve(MAX_PARTICLE_NUM);

    Reset();
}

void Physics::Reset() {
    m_particle_num = 0;
    m_inv_spacing  = 1.0f / glm::max(g_ui.grid_cell_size, 1e-6f);

    m_cell_num   = glm::ivec3(glm::floor(g_ui.box_size * m_inv_spacing)) + glm::ivec3(1);
    m_cell_num   = glm::max(m_cell_num, glm::ivec3(1));
    m_cell_total = static_cast<uint32_t>(m_cell_num.x) * m_cell_num.y * m_cell_num.z;

    m_cell_num_particles.assign(m_cell_total, 0);
    m_cell_first_particles.assign(m_cell_total + 1, 0);
    m_cell_type.assign(m_cell_total, AIR);
    m_particle_density.assign(m_cell_total, 0.0f);
    m_vel.assign(m_cell_total, glm::vec3(0.0f));
    m_vel_prev.assign(m_cell_total, glm::vec3(0.0f));
    m_pressure.assign(m_cell_total, 0.0f);
    m_vel_weights.assign(m_cell_total, glm::vec3(0.0f));

    m_cell_particle_ids.clear();
    m_particle_cell_idx.clear();
    particle_rest_density = 0.0f;

    SpawnParticles();
}

void Physics::SpawnParticles() {
    const float r       = g_ui.particle_radius;
    const float base_dx = 2.0f * r;
    float       dx      = base_dx;
    float       dy      = std::sqrt(3.0f) * 0.5f * base_dx;
    float       dz      = std::sqrt(3.0f) * 0.5f * base_dx;

    const glm::vec3 margin(g_ui.grid_cell_size + r);
    const glm::vec3 block_size = glm::max(
        glm::min(
            glm::vec3(
                INITIAL_WATER_WIDTH  * g_ui.box_size.x,
                INITIAL_WATER_HEIGHT * g_ui.box_size.y,
                INITIAL_WATER_WIDTH  * g_ui.box_size.z
            ),
            g_ui.box_size - 2.0f * margin
        ),
        glm::vec3(0.0f)
    );

    // Grow the lattice spacing until the particle count fits in our buffers.
    int    nx = 0, ny = 0, nz = 0;
    size_t total = 0;
    while (true) {
        nx = std::max(0, static_cast<int>(std::floor(block_size.x / dx)));
        ny = std::max(0, static_cast<int>(std::floor(block_size.y / dy)));
        nz = std::max(0, static_cast<int>(std::floor(block_size.z / dz)));
        total = static_cast<size_t>(nx) * ny * nz;

        if (total <= MAX_PARTICLE_NUM) {
            break;
        }

        float scale = std::cbrt(static_cast<float>(total) / MAX_PARTICLE_NUM);
        dx *= scale;
        dy *= scale;
        dz *= scale;
    }

    m_pos.clear();
    m_color.clear();
    m_particle_vel.clear();

    // Fill the block with a staggered lattice so particles do not start overlapping.
    const glm::vec3 initial_color(0.0f, 0.0f, 1.0f);
    for (int k = 0; k < nz; k++) {
        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx; i++) {
                float stagger = ((j + k) % 2 == 0) ? 0.0f : r;

                m_pos.push_back(margin + glm::vec3(
                    r + i * dx + stagger,
                    r + j * dy,
                    r + k * dz
                ));
                m_color.push_back(initial_color);
                m_particle_vel.push_back(glm::vec3(0.0f));
            }
        }
    }

    m_particle_num = static_cast<uint32_t>(m_pos.size());
}

void Physics::Update(float timestep, const glm::vec3& obstacle_position, float obstacle_radius) {
    m_inv_spacing = 1.0f / glm::max(g_ui.grid_cell_size, 1e-6f);

    const glm::ivec3 old_cell_num = m_cell_num;
    m_cell_num   = glm::ivec3(glm::floor(g_ui.box_size * m_inv_spacing)) + glm::ivec3(1);
    m_cell_num   = glm::max(m_cell_num, glm::ivec3(1));
    m_cell_total = static_cast<uint32_t>(m_cell_num.x) * m_cell_num.y * m_cell_num.z;

    // The cell layout changed, so all grid data is stale.
    if (m_cell_num != old_cell_num) {
        m_cell_num_particles.assign(m_cell_total, 0);
        m_cell_first_particles.assign(m_cell_total + 1, 0);
        m_cell_type.assign(m_cell_total, AIR);
        m_particle_density.assign(m_cell_total, 0.0f);
        m_vel.assign(m_cell_total, glm::vec3(0.0f));
        m_vel_prev.assign(m_cell_total, glm::vec3(0.0f));
        m_pressure.assign(m_cell_total, 0.0f);
        m_vel_weights.assign(m_cell_total, glm::vec3(0.0f));
        particle_rest_density = 0.0f;
    }

    if (m_particle_num == 0) {
        SpawnParticles();
    }

    const int substeps = std::max(1, g_ui.substep_num);
    m_subtimestep = timestep / substeps;

    for (int step = 0; step < substeps; step++) {
        { Timer timer("Physics/Integrate");      IntegrateParticles(m_subtimestep); }
        { Timer timer("Physics/PushApart");      PushParticlesApart(); }
        { Timer timer("Physics/Collisions");     HandleParticleCollisions(obstacle_position, obstacle_radius); }
        { Timer timer("Physics/TransferToGrid"); TransferVelocities(true); }
        { Timer timer("Physics/Density");        UpdateParticleDensity(); }
        { Timer timer("Physics/PressureSolve");  SolveIncompressibility(); }
        { Timer timer("Physics/TransferFromGrid"); TransferVelocities(false); }
    }

    { Timer timer("Physics/Colors"); UpdateParticleColors(); }
}

uint32_t Physics::GetCellIdx(uint32_t i) {
    const glm::ivec3 cell_pos = GetSpatialCellIdx(i);

    return static_cast<uint32_t>(cell_pos.z) * m_cell_num.x * m_cell_num.y
         + static_cast<uint32_t>(cell_pos.y) * m_cell_num.x
         + static_cast<uint32_t>(cell_pos.x);
}

glm::ivec3 Physics::GetSpatialCellIdx(uint32_t i) {
    const glm::ivec3 cell_pos = glm::ivec3(glm::floor(m_pos[i] * m_inv_spacing));
    return glm::clamp(cell_pos, glm::ivec3(0), m_cell_num - 1);
}

void Physics::IntegrateParticles(float timestep) {
    for (uint32_t particle = 0; particle < m_particle_num; particle++) {
        // Gravity only affects the vertical component.
        m_particle_vel[particle].y += timestep * g_ui.gravity;
        m_pos[particle] += timestep * m_particle_vel[particle];
    }
}

void Physics::PushParticlesApart() {
    const int      nx          = m_cell_num.x;
    const int      ny          = m_cell_num.y;
    const int      nz          = m_cell_num.z;
    const uint32_t stride_y    = static_cast<uint32_t>(nx);
    const uint32_t stride_z    = static_cast<uint32_t>(nx * ny);
    const float    inv_spacing = m_inv_spacing;

    glm::vec3* pos                  = m_pos.data();
    uint32_t*  cell_num_particles   = m_cell_num_particles.data();
    uint32_t*  cell_first_particles = m_cell_first_particles.data();
    uint32_t*  cell_particle_ids    = nullptr;

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

    {
        std::fill(m_cell_num_particles.begin(), m_cell_num_particles.end(), 0);

        // Count particles per cell. Cache each particle's cell so the fill
        // pass below does not have to recompute it.
        m_particle_cell_idx.resize(m_particle_num);
        for (uint32_t particle = 0; particle < m_particle_num; particle++) {
            const uint32_t cell_idx = CellIndexFromPos(pos[particle]);
            m_particle_cell_idx[particle] = cell_idx;
            cell_num_particles[cell_idx]++;
        }

        // Exclusive prefix sums (with one guard element at the end).
        uint32_t first = 0;
        for (uint32_t cell_idx = 0; cell_idx < m_cell_total; cell_idx++) {
            first += cell_num_particles[cell_idx];
            cell_first_particles[cell_idx] = first;
        }
        cell_first_particles[m_cell_total] = first;

        // Populate the particle id array and turn the prefix sums into start indices.
        m_cell_particle_ids.resize(m_particle_num);
        cell_particle_ids = m_cell_particle_ids.data();

        for (uint32_t particle = 0; particle < m_particle_num; particle++) {
            const uint32_t cell_idx = m_particle_cell_idx[particle];
            cell_first_particles[cell_idx]--;
            cell_particle_ids[cell_first_particles[cell_idx]] = particle;
        }
    }

    const float min_dist   = 2.0f * g_ui.particle_radius;
    const float min_dist_2 = min_dist * min_dist;

    // Iterate over the 27 neighbouring cells (including the current one).
    const auto ForCellNeighbours = [&](const glm::ivec3& cell_pos, auto&& callback) {
        const int x0 = std::max(cell_pos.x - 1, 0);
        const int y0 = std::max(cell_pos.y - 1, 0);
        const int z0 = std::max(cell_pos.z - 1, 0);
        const int x1 = std::min(cell_pos.x + 1, nx - 1);
        const int y1 = std::min(cell_pos.y + 1, ny - 1);
        const int z1 = std::min(cell_pos.z + 1, nz - 1);

        for (int xi = x0; xi <= x1; xi++) {
            for (int yi = y0; yi <= y1; yi++) {
                for (int zi = z0; zi <= z1; zi++) {
                    callback(CellIndex(xi, yi, zi));
                }
            }
        }
    };

    for (int iter = 0; iter < g_ui.collision_iter_num; iter++) {
        for (uint32_t particle = 0; particle < m_particle_num; particle++) {
            const glm::vec3 particle_pos = pos[particle];

            ForCellNeighbours(CellPosFromPos(particle_pos), [&](uint32_t cell_idx) {
                const uint32_t first = cell_first_particles[cell_idx];
                const uint32_t last  = cell_first_particles[cell_idx + 1];

                for (uint32_t j = first; j < last; j++) {
                    const uint32_t neighbour = cell_particle_ids[j];

                    // Each unordered pair is visible from both particles. Only
                    // process it from the lower id to avoid doing the work twice.
                    if (neighbour <= particle) {
                        continue;
                    }

                    glm::vec3 diff   = pos[neighbour] - particle_pos;
                    const float dist_2 = glm::length2(diff);
                    if (dist_2 <= 0.0f || dist_2 >= min_dist_2) {
                        continue;
                    }

                    // Push each particle by half of the overlap.
                    const float dist = glm::sqrt(dist_2);
                    const float s    = 0.5f * (min_dist - dist) / dist;
                    diff *= s;
                    pos[particle]  -= diff;
                    pos[neighbour] += diff;
                }
            });
        }
    }
}

void Physics::HandleParticleCollisions(glm::vec3 obstacle_pos, float obstacle_radius) {
    const float h          = 1.0f / m_inv_spacing;
    const float r          = g_ui.particle_radius;
    const float min_dist   = obstacle_radius + r;
    const float min_dist_2 = min_dist * min_dist;

    const glm::vec3 min_pos(h + r);
    const glm::vec3 max_pos = glm::vec3(m_cell_num - 1) * h - r;

    for (uint32_t particle = 0; particle < m_particle_num; particle++) {
        glm::vec3 diff   = m_pos[particle] - obstacle_pos;
        const float dist_2 = glm::length2(diff);

        // Push particles out of the obstacle sphere.
        if (dist_2 < min_dist_2) {
            if (dist_2 > 0.0f) {
                const float dist = glm::sqrt(dist_2);
                const float s    = (min_dist - dist) / dist;
                m_pos[particle] += diff * s;
            }
            else {
                // Degenerate case: particle is exactly at the obstacle center.
                m_pos[particle] = obstacle_pos + glm::vec3(min_dist, 0.0f, 0.0f);
            }
        }

        // Box wall collisions.
        if (m_pos[particle].x < min_pos.x) { m_pos[particle].x = min_pos.x; m_particle_vel[particle].x = 0.0f; }
        else if (m_pos[particle].x > max_pos.x) { m_pos[particle].x = max_pos.x; m_particle_vel[particle].x = 0.0f; }
        if (m_pos[particle].y < min_pos.y) { m_pos[particle].y = min_pos.y; m_particle_vel[particle].y = 0.0f; }
        else if (m_pos[particle].y > max_pos.y) { m_pos[particle].y = max_pos.y; m_particle_vel[particle].y = 0.0f; }
        if (m_pos[particle].z < min_pos.z) { m_pos[particle].z = min_pos.z; m_particle_vel[particle].z = 0.0f; }
        else if (m_pos[particle].z > max_pos.z) { m_pos[particle].z = max_pos.z; m_particle_vel[particle].z = 0.0f; }
    }
}

void Physics::UpdateParticleDensity() {
    std::fill(m_particle_density.begin(), m_particle_density.end(), 0.0f);

    const float h  = 1.0f / m_inv_spacing;
    const float h2 = 0.5f * h;

    const auto AddDensity = [&](const glm::ivec3& c, float weight) {
        if (c.x < 0 || c.y < 0 || c.z < 0 ||
            c.x >= m_cell_num.x || c.y >= m_cell_num.y || c.z >= m_cell_num.z) {
            return;
        }

        const uint32_t cell_idx = static_cast<uint32_t>(c.z) * m_cell_num.x * m_cell_num.y
                                + static_cast<uint32_t>(c.y) * m_cell_num.x
                                + static_cast<uint32_t>(c.x);
        m_particle_density[cell_idx] += weight;
    };

    for (uint32_t i = 0; i < m_particle_num; i++) {
        const glm::vec3 pos = glm::clamp(
            m_pos[i],
            glm::vec3(h),
            glm::vec3(m_cell_num - 1) * h
        );

        const glm::vec3 shifted = pos - glm::vec3(h2);
        const glm::ivec3 c0 = glm::ivec3(glm::floor(shifted * m_inv_spacing));
        const glm::ivec3 c1 = glm::min(c0 + 1, m_cell_num - 2);

        const glm::vec3 t  = (shifted - glm::vec3(c0) * h) * m_inv_spacing;
        const glm::vec3 s  = glm::vec3(1.0f) - t;

        // Trilinear distribution over the 8 corners of the containing cell.
        AddDensity(glm::ivec3(c0.x, c0.y, c0.z), s.x * s.y * s.z);
        AddDensity(glm::ivec3(c1.x, c0.y, c0.z), t.x * s.y * s.z);
        AddDensity(glm::ivec3(c0.x, c1.y, c0.z), s.x * t.y * s.z);
        AddDensity(glm::ivec3(c1.x, c1.y, c0.z), t.x * t.y * s.z);
        AddDensity(glm::ivec3(c0.x, c0.y, c1.z), s.x * s.y * t.z);
        AddDensity(glm::ivec3(c1.x, c0.y, c1.z), t.x * s.y * t.z);
        AddDensity(glm::ivec3(c0.x, c1.y, c1.z), s.x * t.y * t.z);
        AddDensity(glm::ivec3(c1.x, c1.y, c1.z), t.x * t.y * t.z);
    }

    // Estimate the rest density from the first frame's fluid cells.
    if (particle_rest_density == 0.0f) {
        float    sum            = 0.0f;
        uint32_t num_fluid_cells = 0;

        for (uint32_t i = 0; i < m_cell_total; i++) {
            if (m_cell_type[i] == FLUID) {
                sum += m_particle_density[i];
                num_fluid_cells++;
            }
        }

        if (num_fluid_cells > 0) {
            particle_rest_density = sum / num_fluid_cells;
        }
    }
}

void Physics::TransferVelocities(bool to_grid) {
    const int   nx       = m_cell_num.x;
    const int   ny       = m_cell_num.y;
    const int   nz       = m_cell_num.z;
    const float h        = 1.0f / m_inv_spacing;
    const float h2       = 0.5f * h;
    const int   stride_y = nx;
    const int   stride_z = nx * ny;

    const auto CellIndex = [&](int x, int y, int z) {
        return static_cast<uint32_t>(z) * stride_z
             + static_cast<uint32_t>(y) * stride_y
             + static_cast<uint32_t>(x);
    };

    // MAC-grid offsets: each velocity component lives on a different face.
    const glm::vec3 offsets[3] = {
        glm::vec3(0.0f, h2,   h2),
        glm::vec3(h2,   0.0f, h2),
        glm::vec3(h2,   h2,   0.0f),
    };

    // Boundary cells behave as solid; interior AIR cells are empty.
    const auto IsAir = [&](const glm::ivec3& c) {
        if (c.x <= 0 || c.y <= 0 || c.z <= 0 ||
            c.x >= nx - 1 || c.y >= ny - 1 || c.z >= nz - 1) {
            return false;
        }
        return m_cell_type[CellIndex(c.x, c.y, c.z)] == AIR;
    };

    if (to_grid) {
        std::fill(m_vel.begin(), m_vel.end(), glm::vec3(0.0f));
        std::fill(m_vel_weights.begin(), m_vel_weights.end(), glm::vec3(0.0f));
        std::fill(m_cell_type.begin(), m_cell_type.end(), AIR);

        // Mark every cell that contains a particle as fluid.
        for (uint32_t i = 0; i < m_particle_num; i++) {
            const glm::ivec3 c = GetSpatialCellIdx(i);
            m_cell_type[CellIndex(c.x, c.y, c.z)] = FLUID;
        }
    }

    for (int component = 0; component < 3; component++) {
        const glm::vec3 offset = offsets[component];

        for (uint32_t i = 0; i < m_particle_num; i++) {
            const glm::vec3 pos = glm::clamp(
                m_pos[i],
                glm::vec3(h),
                glm::vec3(m_cell_num - 1) * h
            );

            const glm::vec3 shifted = pos - offset;
            const glm::ivec3 c0 = glm::min(glm::ivec3(glm::floor(shifted * m_inv_spacing)), m_cell_num - 2);
            const glm::ivec3 c1 = c0 + 1;

            const glm::vec3 t = (shifted - glm::vec3(c0) * h) * m_inv_spacing;
            const glm::vec3 s = glm::vec3(1.0f) - t;

            const glm::ivec3 corners[8] = {
                glm::ivec3(c0.x, c0.y, c0.z),
                glm::ivec3(c1.x, c0.y, c0.z),
                glm::ivec3(c0.x, c1.y, c0.z),
                glm::ivec3(c1.x, c1.y, c0.z),
                glm::ivec3(c0.x, c0.y, c1.z),
                glm::ivec3(c1.x, c0.y, c1.z),
                glm::ivec3(c0.x, c1.y, c1.z),
                glm::ivec3(c1.x, c1.y, c1.z),
            };

            const float weights[8] = {
                s.x * s.y * s.z,
                t.x * s.y * s.z,
                s.x * t.y * s.z,
                t.x * t.y * s.z,
                s.x * s.y * t.z,
                t.x * s.y * t.z,
                s.x * t.y * t.z,
                t.x * t.y * t.z,
            };

            if (to_grid) {
                const float pv = m_particle_vel[i][component];

                for (int k = 0; k < 8; k++) {
                    const uint32_t cell = CellIndex(corners[k].x, corners[k].y, corners[k].z);
                    m_vel[cell][component]         += pv * weights[k];
                    m_vel_weights[cell][component] += weights[k];
                }
            }
            else {
                float d_sum    = 0.0f;
                float pic_sum  = 0.0f;
                float corr_sum = 0.0f;

                for (int k = 0; k < 8; k++) {
                    const uint32_t cell = CellIndex(corners[k].x, corners[k].y, corners[k].z);

                    // A face is valid if at least one of its two adjacent cells is not air.
                    glm::ivec3 neighbour_pos = corners[k];
                    if      (component == 0) neighbour_pos.x--;
                    else if (component == 1) neighbour_pos.y--;
                    else                     neighbour_pos.z--;

                    const float valid = (!IsAir(corners[k]) || !IsAir(neighbour_pos)) ? 1.0f : 0.0f;
                    const float w     = valid * weights[k];

                    d_sum    += w;
                    pic_sum  += w * m_vel[cell][component];
                    corr_sum += w * (m_vel[cell][component] - m_vel_prev[cell][component]);
                }

                if (d_sum > 0.0f) {
                    const float pic_v  = pic_sum / d_sum;
                    const float corr   = corr_sum / d_sum;
                    const float flip_v = m_particle_vel[i][component] + corr;

                    // g_ui.pic_flip is the FLIP fraction (0 = pure PIC, 1 = pure FLIP).
                    m_particle_vel[i][component] =
                        (1.0f - g_ui.pic_flip) * pic_v + g_ui.pic_flip * flip_v;
                }
            }
        }
    }

    if (to_grid) {
        // Normalize accumulated velocities by their interpolation weights.
        for (uint32_t cell = 0; cell < m_cell_total; cell++) {
            for (int component = 0; component < 3; component++) {
                const float weight = m_vel_weights[cell][component];
                if (weight > 0.0f) {
                    m_vel[cell][component] /= weight;
                }
            }
        }

        // Save the pre-projection velocity for FLIP before viscosity. The
        // particle-velocity correction then includes both viscosity and pressure.
        m_vel_prev = m_vel;

        ApplyViscosity();

        // Static box walls: no flow through the boundary shell. In the MAC
        // layout a face velocity is stored on the cell on its positive side,
        // so also zero the component whose negative neighbour is solid.
        const auto IsSolid = [&](int x, int y, int z) {
            return x <= 0 || y <= 0 || z <= 0 ||
                   x >= nx - 1 || y >= ny - 1 || z >= nz - 1;
        };

        for (int z = 0; z < nz; z++) {
            for (int y = 0; y < ny; y++) {
                for (int x = 0; x < nx; x++) {
                    const uint32_t cell = CellIndex(x, y, z);

                    if (IsSolid(x, y, z)) {
                        m_vel[cell] = glm::vec3(0.0f);
                        continue;
                    }

                    if (IsSolid(x - 1, y, z)) m_vel[cell].x = 0.0f;
                    if (IsSolid(x, y - 1, z)) m_vel[cell].y = 0.0f;
                    if (IsSolid(x, y, z - 1)) m_vel[cell].z = 0.0f;
                }
            }
        }
    }
}

void Physics::ApplyViscosity() {
    if (g_ui.viscosity <= 0.0f) {
        return;
    }

    const float alpha = glm::clamp(g_ui.viscosity, 0.0f, 1.0f);
    const int   nx    = m_cell_num.x;
    const int   ny    = m_cell_num.y;
    const int   nz    = m_cell_num.z;

    const auto CellIndex = [&](int x, int y, int z) {
        return static_cast<uint32_t>(z) * nx * ny
             + static_cast<uint32_t>(y) * nx
             + static_cast<uint32_t>(x);
    };

    for (int z = 1; z < nz - 1; z++) {
        for (int y = 1; y < ny - 1; y++) {
            for (int x = 1; x < nx - 1; x++) {
                const uint32_t center = CellIndex(x, y, z);
                if (m_cell_type[center] != FLUID) {
                    continue;
                }

                const uint32_t neighbours[6] = {
                    CellIndex(x - 1, y, z),
                    CellIndex(x + 1, y, z),
                    CellIndex(x, y - 1, z),
                    CellIndex(x, y + 1, z),
                    CellIndex(x, y, z - 1),
                    CellIndex(x, y, z + 1),
                };

                glm::vec3 sum(0.0f);
                int       count = 0;
                for (uint32_t neighbour : neighbours) {
                    if (m_cell_type[neighbour] == FLUID) {
                        sum += m_vel_prev[neighbour];
                        count++;
                    }
                }

                if (count > 0) {
                    const glm::vec3 average = sum / static_cast<float>(count);
                    m_vel[center] = glm::mix(m_vel_prev[center], average, alpha);
                }
            }
        }
    }
}

void Physics::SolveIncompressibility() {
    std::fill(m_pressure.begin(), m_pressure.end(), 0.0f);

    const int   nx = m_cell_num.x;
    const int   ny = m_cell_num.y;
    const int   nz = m_cell_num.z;
    const float h  = 1.0f / m_inv_spacing;
    const float cp = g_ui.density * h / glm::max(m_subtimestep, 1e-6f);

    const auto CellIndex = [&](int x, int y, int z) {
        return static_cast<uint32_t>(z) * nx * ny
             + static_cast<uint32_t>(y) * nx
             + static_cast<uint32_t>(x);
    };

    for (int iter = 0; iter < g_ui.pressure_iter_num; iter++) {
        for (int z = 1; z < nz - 1; z++) {
            for (int y = 1; y < ny - 1; y++) {
                for (int x = 1; x < nx - 1; x++) {
                    const uint32_t center = CellIndex(x, y, z);
                    if (m_cell_type[center] != FLUID) {
                        continue;
                    }

                    const uint32_t left   = CellIndex(x - 1, y, z);
                    const uint32_t right  = CellIndex(x + 1, y, z);
                    const uint32_t bottom = CellIndex(x, y - 1, z);
                    const uint32_t top    = CellIndex(x, y + 1, z);
                    const uint32_t back   = CellIndex(x, y, z - 1);
                    const uint32_t front  = CellIndex(x, y, z + 1);

                    // The outermost cell shell is solid (s = 0), interior cells are open (s = 1).
                    const float sx0 = (x > 1)      ? 1.0f : 0.0f;
                    const float sx1 = (x < nx - 2) ? 1.0f : 0.0f;
                    const float sy0 = (y > 1)      ? 1.0f : 0.0f;
                    const float sy1 = (y < ny - 2) ? 1.0f : 0.0f;
                    const float sz0 = (z > 1)      ? 1.0f : 0.0f;
                    const float sz1 = (z < nz - 2) ? 1.0f : 0.0f;

                    const float s = sx0 + sx1 + sy0 + sy1 + sz0 + sz1;
                    if (s == 0.0f) {
                        continue;
                    }

                    float div = (m_vel[right].x  - m_vel[center].x)
                              + (m_vel[top].y    - m_vel[center].y)
                              + (m_vel[front].z  - m_vel[center].z);

                    // Optional density drift compensation. It is off by default
                    // because the rest density is estimated from the initial
                    // free-surface configuration and can make a settled fluid
                    // expand and keep moving.
                    if (g_ui.compensate_drift && particle_rest_density > 0.0f) {
                        const float compression = m_particle_density[center] - particle_rest_density;
                        if (compression > 0.0f) {
                            div -= compression;
                        }
                    }

                    float p = -div / s;
                    p *= OVER_RELAXATION;
                    m_pressure[center] += cp * p;

                    m_vel[center].x -= sx0 * p;
                    m_vel[right].x  += sx1 * p;
                    m_vel[center].y -= sy0 * p;
                    m_vel[top].y    += sy1 * p;
                    m_vel[center].z -= sz0 * p;
                    m_vel[front].z  += sz1 * p;
                }
            }
        }
    }
}

void Physics::UpdateParticleColors() {
    const float color_change = 0.01f;

    for (uint32_t i = 0; i < m_particle_num; i++) {
        // Slowly shift blue -> white, like the reference implementation.
        m_color[i].x = glm::clamp(m_color[i].x - color_change, 0.0f, 1.0f);
        m_color[i].y = glm::clamp(m_color[i].y - color_change, 0.0f, 1.0f);
        m_color[i].z = glm::clamp(m_color[i].z + color_change, 0.0f, 1.0f);

        if (particle_rest_density == 0.0f) {
            continue;
        }

        const glm::ivec3 cell_pos = glm::clamp(
            GetSpatialCellIdx(i),
            glm::ivec3(1),
            m_cell_num - 1
        );
        const uint32_t cell = static_cast<uint32_t>(cell_pos.z) * m_cell_num.x * m_cell_num.y
                            + static_cast<uint32_t>(cell_pos.y) * m_cell_num.x
                            + static_cast<uint32_t>(cell_pos.x);

        // Color low-density (foam/spray) particles light blue.
        const float rel_density = m_particle_density[cell] / particle_rest_density;
        if (rel_density < 0.7f) {
            m_color[i] = glm::vec3(0.8f, 0.8f, 1.0f);
        }
    }
}
