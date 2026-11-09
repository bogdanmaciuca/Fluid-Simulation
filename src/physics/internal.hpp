#pragma once

// DOD
void PhysicsDODInit();
void PhysicsDODUpdate(double dt);
uint32_t PhysicsDODGetParticleNum();
std::span<glm::dvec3> PhysicsDODGetPositions();
std::span<glm::dvec3> PhysicsDODGetColors();
std::span<glm::vec4> PhysicsDODGetGridCells();

// OOP
void PhysicsOOPInit();
void PhysicsOOPUpdate(double dt);
uint32_t PhysicsOOPGetParticleNum();
std::span<glm::dvec3> PhysicsOOPGetPositions();
std::span<glm::dvec3> PhysicsOOPGetColors();
std::span<glm::vec4> PhysicsOOPGetGridCells();

// Utilities
constexpr double CELL_SIZE = 0.1;
constexpr double INV_CELL_SIZE = 1.0 / CELL_SIZE;
constexpr double PARTICLE_MASS = 0.6;
constexpr double SMOOTHING_LENGTH = CELL_SIZE;
constexpr double SMOOTHING_LENGTH_2 = SMOOTHING_LENGTH * SMOOTHING_LENGTH;
constexpr double SMOOTHING_LENGTH_3 = SMOOTHING_LENGTH * SMOOTHING_LENGTH * SMOOTHING_LENGTH;
constexpr double SMOOTHING_LENGTH_6 = SMOOTHING_LENGTH_3 * SMOOTHING_LENGTH_3;
constexpr double SMOOTHING_LENGTH_9 = SMOOTHING_LENGTH_3 * SMOOTHING_LENGTH_3 * SMOOTHING_LENGTH_3;
constexpr double REST_DENSITY = 1000.0;    // kg/m^3
constexpr double SPEED_OF_SOUND = 20.0;     // m/s
constexpr double GAMMA = 3.0;              // Tait equation exponent
constexpr double TAIT_COEF = (SPEED_OF_SOUND * SPEED_OF_SOUND * REST_DENSITY) / GAMMA;

struct Grid {
    void Update(std::span<glm::dvec3> positions);
    std::span<uint32_t> GetCellParticleIDs(uint32_t cell_idx);
    std::span<const double> GetOccupancy() const { return m_occupancy; }

    glm::ivec3 GetSize() { return m_size; }
    uint32_t GetStrideY() { return m_stride_y; }
    uint32_t GetStrideZ() { return m_stride_z; }

    uint32_t CellIdx(const glm::ivec3& v) const {
        return v.z * m_stride_z + v.y * m_stride_y + v.x;
    }
    glm::ivec3 CellPosFromPos(const glm::dvec3& p) const {
        return glm::ivec3(
            std::clamp(static_cast<int>(std::floor(p.x * INV_CELL_SIZE)), 0, m_size.x - 1),
            std::clamp(static_cast<int>(std::floor(p.y * INV_CELL_SIZE)), 0, m_size.y - 1),
            std::clamp(static_cast<int>(std::floor(p.z * INV_CELL_SIZE)), 0, m_size.z - 1)
        );
    }
    uint32_t CellIdxFromPos(const glm::dvec3& p) const {
        return CellIdx(CellPosFromPos(p));
    }
private:
    glm::ivec3 m_size;
    // Indices into the particle vector(s)
    std::vector<uint32_t> m_ids;
    // First index of cell particles in ids
    std::vector<uint32_t> m_first;
    // Number of particles in cell
    std::vector<uint32_t> m_num;
    // Normalized occupancy (0..1) per cell
    std::vector<double> m_occupancy;
    // Cached belonging cell for each particle
    std::vector<uint32_t> m_cell_idx;

    uint32_t m_stride_y;
    uint32_t m_stride_z;
};

inline double Poly6Kernel(double r_2) {
    if (r_2 >= SMOOTHING_LENGTH_2) return 0.0;
    constexpr double coeff = 315.0 / (64.0 * M_PI * SMOOTHING_LENGTH_9);
    double diff = SMOOTHING_LENGTH_2 - r_2;
    return coeff * (diff * diff * diff);
}

inline glm::dvec3 SpikyGradient(glm::dvec3 r_vec, double r) {
    if (r >= SMOOTHING_LENGTH || r == 0.0) return glm::dvec3(0.0);
    constexpr double coeff = -45.0 / (M_PI * SMOOTHING_LENGTH_6);
    double diff = SMOOTHING_LENGTH - r;
    return coeff * (diff * diff) * (r_vec / r); // Normalized direction * magnitude gradient
}

inline double ViscosityLaplacian(double r) {
    if (r >= SMOOTHING_LENGTH) return 0.0;
    constexpr double coeff = 45.0 / (M_PI * SMOOTHING_LENGTH_6);
    return coeff * (SMOOTHING_LENGTH - r);
}

