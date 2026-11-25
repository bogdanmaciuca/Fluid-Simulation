#include "internal.hpp"

#include <algorithm>

#include "../timer.hpp"
#include "../ui.hpp"

using namespace glm;

namespace {
    uint32_t              s_num;
    std::vector<dvec3>    s_pos;
    std::vector<dvec3>    s_vel;
    std::vector<dvec3>    s_acc;
    std::vector<double>   s_dens;
    std::vector<double>   s_pres;
    Grid                  s_grid;

    std::vector<dvec3>    s_colors;
    std::vector<glm::vec4> s_grid_cells;

    uint64_t              s_neighbor_checks = 0;

    void BuildGridCells() {
        ivec3 size = s_grid.GetSize();
        auto occupancy = s_grid.GetOccupancy();

        s_grid_cells.clear();
        s_grid_cells.reserve(size.x * size.y * size.z);

        for (int z = 0; z < size.z; ++z) {
            for (int y = 0; y < size.y; ++y) {
                for (int x = 0; x < size.x; ++x) {
                    uint32_t idx = s_grid.CellIdx(ivec3(x, y, z));
                    glm::vec3 center(
                        (x + 0.5) * CELL_SIZE,
                        (y + 0.5) * CELL_SIZE,
                        (z + 0.5) * CELL_SIZE
                    );
                    s_grid_cells.emplace_back(center, static_cast<float>(occupancy[idx]));
                }
            }
        }
    }

    void ResetParticles() {
        double spacing = std::cbrt(PARTICLE_MASS / REST_DENSITY);
        ivec3 count = g_ui.box_size / spacing / 1.2;
        s_num = count.x * count.y * count.z;
        s_pos.clear();
        s_pos.reserve(s_num);
        s_vel.assign(s_num, dvec3(0));
        s_acc.assign(s_num, dvec3(0));
        s_dens.resize(s_num);
        s_pres.resize(s_num);
        s_colors.assign(s_num, dvec3(0));

        for (int z = 0; z < count.z; ++z) {
            for (int y = 0; y < count.y; ++y) {
                for (int x = 0; x < count.x; ++x) {
                    dvec3 pos = dvec3(x * spacing, y * spacing, z * spacing);
                    s_pos.push_back(pos);
                }
            }
        }
    }

    // Ugly as phuck
    template<typename F>
    void ForAllNeighbors(uint32_t p, F&& f) {
        dvec3 pos = s_pos[p];
        ivec3 center_cell = s_grid.CellPosFromPos(pos);
        ivec3 grid_size = s_grid.GetSize();

        // Loop through the 3x3x3 neighborhood (-1 to +1 in each axis)
        for (int dz = -1; dz <= 1; ++dz) {
            int cz = center_cell.z + dz;
            if (cz < 0 || cz >= grid_size.z) continue; // Out of bounds, skip

            for (int dy = -1; dy <= 1; ++dy) {
                int cy = center_cell.y + dy;
                if (cy < 0 || cy >= grid_size.y) continue; // Out of bounds, skip

                for (int dx = -1; dx <= 1; ++dx) {
                    int cx = center_cell.x + dx;
                    if (cx < 0 || cx >= grid_size.x) continue; // Out of bounds, skip

                    // Compute valid neighbor cell index
                    uint32_t neighbor_cell_idx = s_grid.CellIdx(ivec3(cx, cy, cz));

                    // Iterate through all particles in this neighboring cell
                    for (uint32_t neighbor : s_grid.GetCellParticleIDs(neighbor_cell_idx)) {
                        if (neighbor != p) {
                            ++s_neighbor_checks;
                            f(neighbor);
                        }
                    }
                }
            }
        }
    }

    void ComputeDensityAndPressure() {
        for (int p = 0; p < s_num; ++p) {
            double& dens = s_dens[p];
            double& pres = s_pres[p];

            dens = PARTICLE_MASS * Poly6Kernel(0.0);
            ForAllNeighbors(p, [&](uint32_t neighbor) {
                double r_2 = distance2(s_pos[p], s_pos[neighbor]);
                if (r_2 < SMOOTHING_LENGTH_2) {
                    dens += PARTICLE_MASS * Poly6Kernel(r_2);
                }
            });

            double density_ratio = dens / REST_DENSITY;
            pres = TAIT_COEF * (std::pow(density_ratio, GAMMA) - 1.0);
            if (pres < 0.0) {
                pres = 0.0;
            }
        }
    }

    dvec3 Jet(double t) {
        t = std::clamp(t, 0.0, 1.0);
        return dvec3(
            std::clamp(std::min(4.0 * t - 1.5, -4.0 * t + 4.5), 0.0, 1.0),
            std::clamp(std::min(4.0 * t - 0.5, -4.0 * t + 3.5), 0.0, 1.0),
            std::clamp(std::min(4.0 * t + 0.5, -4.0 * t + 2.5), 0.0, 1.0)
        );
    }

    void ComputeColors() {
        double max_pres = 0.0;
        for (int p = 0; p < s_num; ++p) {
            max_pres = std::max(max_pres, s_pres[p]);
        }
        s_colors.resize(s_num);
        for (int p = 0; p < s_num; ++p) {
            double t = (max_pres > 0.0) ? s_pres[p] / max_pres : 0.0;
            s_colors[p] = Jet(t);
        }
    }

    void ComputeForces() {
        for (int p = 0; p < s_num; ++p) {
            dvec3 pressure_force(0.0);
            dvec3 viscosity_force(0.0);

            dvec3 pos_i = s_pos[p];
            dvec3 vel_i = s_vel[p];
            double rho_i  = s_dens[p];
            double pres_i = s_pres[p];

            ForAllNeighbors(p, [&](uint32_t neighbor) {
                dvec3 pos_j = s_pos[neighbor];
                dvec3 r_vec = pos_i - pos_j;
                double r_2 = distance2(pos_i, pos_j);

                if (r_2 < SMOOTHING_LENGTH_2) {
                    double r = std::sqrt(r_2);
                    double pres_j = s_pres[neighbor];
                    double rho_j = s_dens[neighbor];

                    // Pressure Force (Symmetric Monaghan form)
                    dvec3 grad_w = SpikyGradient(r_vec, r);
                    pressure_force -= PARTICLE_MASS * (pres_i / (rho_i * rho_i) + pres_j / (rho_j * rho_j)) * grad_w;

                    // Viscosity Force
                    dvec3 vel_j = s_vel[neighbor];
                    double laplacian = ViscosityLaplacian(r);
                    viscosity_force += g_ui.viscosity * PARTICLE_MASS * ((vel_j - vel_i) / rho_j) * laplacian;
                }
            });

            s_acc[p] = pressure_force + viscosity_force + dvec3(0, g_ui.gravity, 0);
        }
    }

    void Integrate(double dt) {
        const dvec3 box_min(0.0);
        const dvec3 box_max(g_ui.box_size);

        for (int p = 0; p < s_num; ++p) {
            s_vel[p] += s_acc[p] * dt;
            s_pos[p] += s_vel[p] * dt;

            if (s_pos[p].x < box_min.x) { s_pos[p].x = box_min.x; s_vel[p].x *= -g_ui.restitution; }
            if (s_pos[p].x > box_max.x) { s_pos[p].x = box_max.x; s_vel[p].x *= -g_ui.restitution; }

            if (s_pos[p].y < box_min.y) { s_pos[p].y = box_min.y; s_vel[p].y *= -g_ui.restitution; }
            if (s_pos[p].y > box_max.y) { s_pos[p].y = box_max.y; s_vel[p].y *= -g_ui.restitution; }

            if (s_pos[p].z < box_min.z) { s_pos[p].z = box_min.z; s_vel[p].z *= -g_ui.restitution; }
            if (s_pos[p].z > box_max.z) { s_pos[p].z = box_max.z; s_vel[p].z *= -g_ui.restitution; }
        }
    }
}

void PhysicsDODInit() {
    ResetParticles();
}

void PhysicsDODUpdate(double dt) {
    s_neighbor_checks = 0;
    { Timer t("Grid Update");     s_grid.Update(s_pos); }
    { Timer t("Density & Pressure"); ComputeDensityAndPressure(); }
    { Timer t("Forces");          ComputeForces(); }
    { Timer t("Integrate");       Integrate(dt); }
    { Timer t("Colors");          ComputeColors(); }
    BuildGridCells();

    std::cout << (double)s_neighbor_checks / s_num << "\n";
}

uint32_t PhysicsDODGetParticleNum() {
    return s_num;
}

std::span<dvec3> PhysicsDODGetPositions() {
    return s_pos;
}

std::span<dvec3> PhysicsDODGetColors() {
    return s_colors;
}

std::span<glm::vec4> PhysicsDODGetGridCells() {
    return s_grid_cells;
}

