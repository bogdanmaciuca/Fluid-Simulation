#pragma once

constexpr size_t MAX_PARTICLE_NUM = 1'000'000;

struct Physics {
    Physics();
    void Reset();
    void Update(float timestep, const glm::vec3& obstacle_position, float obstacle_radius);

    uint32_t ParticleNum() const { return m_particle_num; }
    std::span<const glm::vec3> ParticlePositions() const { return m_pos; }
    std::span<const glm::vec3> ParticleColors() const { return m_color; }
    std::span<const glm::vec3> ParticleVelocities() const { return m_particle_vel; }

private:
    enum CellType {
        AIR,
        FLUID,
    };

    uint32_t   m_particle_num;
    glm::ivec3 m_cell_num;
    uint32_t   m_cell_total;
    float      m_inv_spacing;
    float      m_subtimestep        = 0.0f;
    float      particle_rest_density = 0.0f;

    // Particles
    std::vector<glm::vec3> m_pos;
    std::vector<glm::vec3> m_color;
    std::vector<glm::vec3> m_particle_vel;

    // Grid (one entry per cell)
    std::vector<glm::vec3> m_vel;        // current velocity (x, y, z components)
    std::vector<glm::vec3> m_vel_prev;   // velocity before pressure projection (for FLIP)
    std::vector<float>     m_pressure;
    std::vector<glm::vec3> m_vel_weights; // interpolation weight denominator per component

    std::vector<uint32_t>  m_cell_num_particles;
    std::vector<uint32_t>  m_cell_first_particles;
    std::vector<uint32_t>  m_cell_particle_ids;
    std::vector<uint32_t>  m_particle_cell_idx;
    std::vector<CellType>  m_cell_type;
    std::vector<float>     m_particle_density;

    void SpawnParticles();
    uint32_t GetCellIdx(uint32_t i);
    glm::ivec3 GetSpatialCellIdx(uint32_t i);
    void IntegrateParticles(float timestep);
    void PushParticlesApart();
    void HandleParticleCollisions(glm::vec3 obstacle_pos, float obstacle_radius);
    void UpdateParticleDensity();
    void TransferVelocities(bool to_grid);
    void ApplyViscosity();
    void SolveIncompressibility();
    void UpdateParticleColors();
};

