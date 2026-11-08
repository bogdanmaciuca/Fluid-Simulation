#include <cmath>
#include <cstdint>
#include <tuple>

#include "../ui.hpp"
#include "../main_loop.hpp"
#include "physics.hpp"

std::vector<uint32_t> cel_first_particle;
std::vector<uint32_t> cel_particle_ids;

int particle_count;
float cell_size;
float list_margin;
float list_radius2;
float density_area;
float da2; // precalculation for density_area^2
float poly6_coef;
float spiky_coef;
float particle_spacing;
float collision_diameter;
float wall_range;
float wall_clamp;
float delta_time;
float particle_mass = 1.0f;
float rest_density = 1.0f;
glm::vec3 gravity(0.0, -9.81, 0.0);
glm::ivec3 grid_size;

std::vector<glm::vec3> colors;
std::vector<glm::vec3> positions;


constexpr float PRESSURE_STRENGTH = 1.0f;

constexpr float TAIT_EXPONENT = 7.0f;
constexpr float TAIT_STIFFNESS = 25.0f;
constexpr float WALL_SPRING = 1.0e5f;
constexpr float WALL_DAMPER = 300.0f;
constexpr float WALL_FRICTION = 5.0f;
constexpr int   MAX_SUBSTEPS = 32;
constexpr float CFL_FACTOR = 0.25f;
constexpr float WALL_DAMPING = 0.5f;

constexpr float RESTITUTION = 0.3f;
constexpr float CONTACT_FRICTION = 0.1f;
constexpr float COLLISION_RELAXATION = 0.5f;

// Pi = 3 btw

float Poly6Kernel(float r2){
    float diff = da2 - r2;

    if (diff <= 0.0f){
        return 0.0f;
    }

    return poly6_coef * diff * diff * diff;
}

float TaitEquation(float rest_density, float exponent, float stiffness, float density){
    float pressure = stiffness * rest_density / exponent * (std::pow(density / rest_density, exponent) - 1.0f);

    return std::max(pressure, 0.0f);
}

glm::vec3 SpikyGradient(const glm::vec3& r_vec, float r) {
    if (r <= 1e-6f || r >= density_area) {
        return glm::vec3(0.0f);
    }
    float diff = density_area - r;
    float coef = spiky_coef * diff * diff / r;
    return r_vec * coef;
}

class Particle{
    private:

    protected:
    glm::vec3 position;
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 shift = glm::vec3(0.0f);
    glm::vec3 velocity_change = glm::vec3(0.0f);
    float m = 1.0f;

    Particle(float x, float y, float z): position(x, y, z){}

    public:


    glm::vec3 GetPosition(){
        return position;
    }

    glm::vec3 GetVelocity(){
        return velocity;
    }

    void IncreaseVelocity(glm::vec3 inc){
        velocity += inc;
    }

    void IncreasePosition(glm::vec3 inc){
        position += inc;
    }

    void AddShift(glm::vec3 s){
        shift += s;
    }

    void AddVelocityChange(glm::vec3 dv){
        velocity_change += dv;
    }

    void ApplyCollisionResult(float relaxation){
        position += shift * relaxation;
        velocity += velocity_change * relaxation;
        shift = glm::vec3(0.0f);
        velocity_change = glm::vec3(0.0f);
    }

    void ClampToBox(const glm::vec3& max, float eps){

        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)){
            position = max * 0.5f;
        }

        if (!std::isfinite(velocity.x) || !std::isfinite(velocity.y) || !std::isfinite(velocity.z)){
            velocity = glm::vec3(0.0f);
        }

        const glm::vec3 lo(eps);
        const glm::vec3 hi = max - glm::vec3(eps);

        for (int axis = 0; axis < 3; axis++){
            if (position[axis] < lo[axis]){
                position[axis] = lo[axis];
                velocity[axis] = std::max(velocity[axis], 0.0f) * WALL_DAMPING;
            }
            else if (position[axis] > hi[axis]){
                position[axis] = hi[axis];
                velocity[axis] = std::min(velocity[axis], 0.0f) * WALL_DAMPING;
            }
        }
    }

    float GetM(){
        return m;
    }

    void SetM(float m){
        this->m = m;
    }
};

class Liquid: public Particle{
    private:

    float radious;
    float density = 0.0f, presure = 0.0f;

    glm::vec3 force = glm::vec3(0.0f);

    public:

    Liquid(float x, float y, float z, float r): Particle(x, y, z), radious(r){}

    void ResetDensity(){
        density = m * Poly6Kernel(0.0f);
    }

    void UpdatePressure(){
        presure = TaitEquation(rest_density, TAIT_EXPONENT, TAIT_STIFFNESS, density);
    }

    void IncreaseDensity(float d){
        density += d;
    }

    float GetDensity(){
        return density;
    }

    void IncreaseForce(glm::vec3 f){
        force += f;
    }

    glm::vec3 GetForce(){
        return force;
    }

    void IncreasePreasure(float p){
        presure += p;
    }

    float GetPresure(){
        return presure;
    }

    void SetRadious(float r){
        radious = r;
    }

    float GetRadious(){
        return radious;
    }

    void ResetForce(){
        force = glm::vec3(0.0f);
    }
};

std::vector<Liquid> particles;

const glm::ivec3 FORWARD_OFFSETS[13] = {
    glm::ivec3( 1,  0,  0),
    glm::ivec3(-1,  1,  0), glm::ivec3(0,  1,  0), glm::ivec3(1,  1,  0),
    glm::ivec3(-1, -1,  1), glm::ivec3(0, -1,  1), glm::ivec3(1, -1,  1),
    glm::ivec3(-1,  0,  1), glm::ivec3(0,  0,  1), glm::ivec3(1,  0,  1),
    glm::ivec3(-1,  1,  1), glm::ivec3(0,  1,  1), glm::ivec3(1,  1,  1),
};

float AxisGap(float pos, int cell, int offset){
    if (offset > 0){
        return std::max((cell + 1) * cell_size - pos, 0.0f);
    }
    if (offset < 0){
        return std::max(pos - cell * cell_size, 0.0f);
    }
    return 0.0f;
}

template <typename PairFunc>
void ForEachNeighborPair(PairFunc pair_func){
    for (int cz = 0; cz < grid_size.z; cz++){
        for (int cy = 0; cy < grid_size.y; cy++){
            for (int cx = 0; cx < grid_size.x; cx++){
                int cell = cz * grid_size.x * grid_size.y + cy * grid_size.x + cx;
                int cell_start = cel_first_particle[cell];
                int cell_end = cel_first_particle[cell + 1];

                for (int i = cell_start; i < cell_end; i++){
                    Liquid& a = particles[cel_particle_ids[i]];
                    glm::vec3 pos = a.GetPosition();

                    for (int j = i + 1; j < cell_end; j++){
                        pair_func(a, particles[cel_particle_ids[j]]);
                    }

                    for (const glm::ivec3& offset : FORWARD_OFFSETS){
                        int nx = cx + offset.x;
                        int ny = cy + offset.y;
                        int nz = cz + offset.z;

                        if (nx < 0 || nx >= grid_size.x ||
                            ny < 0 || ny >= grid_size.y ||
                            nz < 0 || nz >= grid_size.z){
                            continue;
                        }

                        float gap_x = AxisGap(pos.x, cx, offset.x);
                        float gap_y = AxisGap(pos.y, cy, offset.y);
                        float gap_z = AxisGap(pos.z, cz, offset.z);

                        if (gap_x * gap_x + gap_y * gap_y + gap_z * gap_z >= list_radius2){
                            continue;
                        }

                        int neighbor = nz * grid_size.x * grid_size.y + ny * grid_size.x + nx;

                        for (int j = int(cel_first_particle[neighbor]); j < int(cel_first_particle[neighbor + 1]); j++){
                            pair_func(a, particles[cel_particle_ids[j]]);
                        }
                    }
                }
            }
        }
    }
}

void DensityPair(Liquid& a, Liquid& b){
    glm::vec3 offset = b.GetPosition() - a.GetPosition();

    float density_impact = Poly6Kernel(glm::dot(offset, offset));

    a.IncreaseDensity(b.GetM() * density_impact);
    b.IncreaseDensity(a.GetM() * density_impact);
}

void ForcePair(Liquid& a, Liquid& b){
    if (a.GetPresure() <= 0.0f && b.GetPresure() <= 0.0f){
        return;
    }

    glm::vec3 offset = b.GetPosition() - a.GetPosition();
    float range = glm::length(offset);

    if (range >= density_area){
        return;
    }

    if (range <= 1e-6f){
        return;
    }

    float pressure_term = a.GetPresure() / (a.GetDensity() * a.GetDensity())
        + b.GetPresure() / (b.GetDensity() * b.GetDensity());

    glm::vec3 gradient = SpikyGradient(offset, range);

    a.IncreaseForce(PRESSURE_STRENGTH * b.GetM() * pressure_term * gradient);
    b.IncreaseForce(-PRESSURE_STRENGTH * a.GetM() * pressure_term * gradient);
}

void CollidePair(Liquid& a, Liquid& b){
    glm::vec3 offset = b.GetPosition() - a.GetPosition();
    float dist2 = glm::dot(offset, offset);

    if (dist2 >= collision_diameter * collision_diameter){
        return;
    }

    float dist = std::sqrt(dist2);
    glm::vec3 normal;

    if (dist > 1e-6f){
        normal = offset / dist;
    }
    else {
        normal = glm::normalize(glm::vec3(0.577f, 0.6f, 0.55f));
        if (&a > &b){
            normal = -normal;
        }
        dist = 0.0f;
    }

    float total_mass = a.GetM() + b.GetM();
    float weight_a = b.GetM() / total_mass;
    float weight_b = a.GetM() / total_mass;

    float penetration = collision_diameter - dist;
    a.AddShift(-normal * penetration * weight_a);
    b.AddShift( normal * penetration * weight_b);

    glm::vec3 relative_velocity = b.GetVelocity() - a.GetVelocity();
    float approach = glm::dot(relative_velocity, normal);

    if (approach < 0.0f){
        glm::vec3 impulse = normal * ((1.0f + RESTITUTION) * approach);
        a.AddVelocityChange( impulse * weight_a);
        b.AddVelocityChange(-impulse * weight_b);

        glm::vec3 sliding = relative_velocity - normal * approach;
        a.AddVelocityChange( sliding * (CONTACT_FRICTION * weight_a));
        b.AddVelocityChange(-sliding * (CONTACT_FRICTION * weight_b));
    }
}

class LiquidFactory{
    private:
    int lattice_index = 0;
    float default_x, default_y, default_z;

    public:
    LiquidFactory(){}
    LiquidFactory(float x, float y, float z):default_x(x), default_y(y), default_z(z){}

    Liquid CreateLiquidParticleBegine(){
        const float margin = wall_range + 0.5f * particle_spacing;
        const int max_x = std::max(1, int((grid_size.x * cell_size - 2.0f * margin) / particle_spacing));
        const int max_z = std::max(1, int((grid_size.z * cell_size - 2.0f * margin) / particle_spacing));
        const int side = std::max(1, int(std::ceil(std::cbrt(float(particle_count)))));
        const int per_x = std::min(side, max_x);
        const int per_z = std::min(side, max_z);

        const int ix = lattice_index % per_x;
        const int iz = (lattice_index / per_x) % per_z;
        const int iy = lattice_index / (per_x * per_z);
        lattice_index++;

        const float x = grid_size.x * cell_size * 0.5f + (ix - (per_x - 1) * 0.5f) * particle_spacing;
        const float z = grid_size.z * cell_size * 0.5f + (iz - (per_z - 1) * 0.5f) * particle_spacing;
        const float y = margin + iy * particle_spacing;

        Liquid liquid(x, y, z, 1.0);
        liquid.SetM(particle_mass);
        return liquid;
    }

    Liquid CreateLiquidParticle(){
        Liquid liquid(default_x, default_y, default_z, 1.0);
        liquid.SetM(particle_mass);
        return liquid;
    }

    void SetX(float x){
        default_x = x;
    }

    void SetY(float y){
        default_y = y;
    }

    void SetZ(float z){
        default_z = z;
    }
};



LiquidFactory liquid_factory;

void UpdateGridSize(){
    grid_size = glm::max(glm::ivec3(g_ui.box_size / cell_size), glm::ivec3(1));

    liquid_factory.SetX(cell_size * grid_size.x / 2);
    liquid_factory.SetY(cell_size * grid_size.y * 3 / 4);
    liquid_factory.SetZ(cell_size * grid_size.z / 2);
}

void BuildGrid(){
    positions.resize(particles.size());

    for (size_t i = 0; i < particles.size(); i++){
        positions[i] = particles[i].GetPosition();
    }

    std::tie(cel_first_particle, cel_particle_ids) =
        GenerateGrid(positions, grid_size, 1.0f / cell_size);
}

std::vector<uint32_t> list_first;
std::vector<uint32_t> list_second;
std::vector<glm::vec3> list_positions;
bool neighbor_list_valid = false;
bool any_pressure = false;

void BuildNeighborList(){
    UpdateGridSize();
    BuildGrid();

    list_first.clear();
    list_second.clear();

    const Liquid* base = particles.data();

    ForEachNeighborPair([&](Liquid& a, Liquid& b){
        glm::vec3 offset = b.GetPosition() - a.GetPosition();

        if (glm::dot(offset, offset) < list_radius2){
            list_first.push_back(uint32_t(&a - base));
            list_second.push_back(uint32_t(&b - base));
        }
    });

    list_positions = positions;
    neighbor_list_valid = true;
}

void EnsureNeighborList(){
    if (!neighbor_list_valid || list_positions.size() != particles.size()){
        BuildNeighborList();
        return;
    }

    const float max_move = 0.4f * list_margin;
    const float max_move2 = max_move * max_move;

    for (size_t i = 0; i < particles.size(); i++){
        glm::vec3 moved = particles[i].GetPosition() - list_positions[i];

        if (glm::dot(moved, moved) > max_move2){
            BuildNeighborList();
            return;
        }
    }
}

template <typename PairFunc>
void ForEachListedPair(PairFunc pair_func){
    const size_t pair_count = list_first.size();

    for (size_t k = 0; k < pair_count; k++){
        pair_func(particles[list_first[k]], particles[list_second[k]]);
    }
}

void ComputeDensities(){
    for (Liquid& particle : particles){
        particle.ResetDensity();
    }

    ForEachListedPair([](Liquid& a, Liquid& b){ DensityPair(a, b); });
}

void ComputePressures(){
    any_pressure = false;

    for (Liquid& particle : particles){
        particle.UpdatePressure();

        if (particle.GetPresure() > 0.0f){
            any_pressure = true;
        }
    }
}

void ComputeForces(){
    if (!any_pressure){
        return;
    }

    ForEachListedPair([](Liquid& a, Liquid& b){ ForcePair(a, b); });
}

void Integrate(float dt){
    const glm::vec3 box = g_ui.box_size;

    for (Liquid& particle : particles){
        particle.IncreaseForce(gravity);

        const glm::vec3 pos = particle.GetPosition();
        const glm::vec3 vel = particle.GetVelocity();

        for (int axis = 0; axis < 3; axis++){
            glm::vec3 push(0.0f);

            float lo_gap = pos[axis];
            if (lo_gap < wall_range){
                push[axis] += WALL_SPRING * (wall_range - lo_gap) - WALL_DAMPER * std::min(vel[axis], 0.0f);
            }

            float hi_gap = box[axis] - pos[axis];
            if (hi_gap < wall_range){
                push[axis] -= WALL_SPRING * (wall_range - hi_gap) + WALL_DAMPER * std::max(vel[axis], 0.0f);
            }

            if (lo_gap < wall_range || hi_gap < wall_range){
                for (int other = 0; other < 3; other++){
                    if (other != axis){
                        push[other] -= WALL_FRICTION * vel[other];
                    }
                }
            }

            particle.IncreaseForce(push);
        }

        particle.IncreaseVelocity(particle.GetForce() * dt);
        particle.IncreasePosition(particle.GetVelocity() * dt);
        particle.ClampToBox(g_ui.box_size, wall_clamp);
        particle.ResetForce();
    }
}

void ResolveCollisions(){
    const int iterations = std::max(g_ui.collision_iter_num, 1);

    for (int i = 0; i < iterations; i++){
        ForEachListedPair([](Liquid& a, Liquid& b){ CollidePair(a, b); });

        for (Liquid& particle : particles){
            particle.ApplyCollisionResult(COLLISION_RELAXATION);
            particle.ClampToBox(g_ui.box_size, wall_clamp);
        }
    }
}

void PhysicsUpdate(){
    const float sound_speed = std::sqrt(TAIT_STIFFNESS);
    const float max_dt = CFL_FACTOR * density_area / sound_speed;
    const int cfl_substeps = int(std::ceil(delta_time / max_dt));
    const int substeps = std::min(std::max(std::max(g_ui.substep_num, cfl_substeps), 1), MAX_SUBSTEPS);
    const float dt = delta_time / float(substeps);

    gravity.y = g_ui.gravity;

    for (int step = 0; step < substeps; step++){
        EnsureNeighborList();

        ComputeDensities();
        ComputePressures();
        ComputeForces();

        Integrate(dt);
        ResolveCollisions();
    }

    g_ui.particle_num = uint32_t(particles.size());
}

std::span<glm::vec3> PhysicsGetPositions(){
    positions.resize(particles.size());

    for(size_t i = 0; i < particles.size(); i++){
        positions[i] = particles[i].GetPosition();
    }

    return std::span(positions);
}

void PhysicsInit(){
    delta_time = FIXED_TIMESTEP;
    density_area = g_ui.grid_cell_size; //wip
    da2 = density_area * density_area;
    poly6_coef = 315.0f / (64.0f * 3.0f * std::pow(density_area, 9.0f));
    spiky_coef = -45.0f / (3.0f * da2 * da2 * da2);
    particle_count = 6000;

    collision_diameter = std::min(2.0f * g_ui.particle_radius, density_area);
    particle_spacing = collision_diameter;
    wall_range = 0.5f * collision_diameter;
    wall_clamp = 0.1f * wall_range;

    list_margin = 0.5f * collision_diameter;
    cell_size = density_area + list_margin;
    list_radius2 = cell_size * cell_size;
    neighbor_list_valid = false;

    particle_mass = g_ui.density * particle_spacing * particle_spacing * particle_spacing;

    rest_density = 0.0f;
    const int reach = int(std::ceil(density_area / particle_spacing));
    for (int i = -reach; i <= reach; i++){
        for (int j = -reach; j <= reach; j++){
            for (int k = -reach; k <= reach; k++){
                glm::vec3 offset = glm::vec3(i, j, k) * particle_spacing;
                rest_density += particle_mass * Poly6Kernel(glm::dot(offset, offset));
            }
        }
    }

    UpdateGridSize();

    liquid_factory = LiquidFactory(cell_size * grid_size.x / 2, cell_size * grid_size.y * 3 / 4, cell_size * grid_size.z / 2);

    particles.clear();
    particles.reserve(particle_count);

    for(int i = 0; i < particle_count; i++){
        particles.push_back(liquid_factory.CreateLiquidParticleBegine());
    }

    positions.resize(particles.size());
}

void IncreaseParticles(){
    neighbor_list_valid = false;
    particles.push_back(liquid_factory.CreateLiquidParticle());
    particle_count = int(particles.size());
}

void DecreaseParticles(){
    neighbor_list_valid = false;
    if (particles.empty()){
        return;
    }

    particles.pop_back();
    particle_count = int(particles.size());
}

std::span<glm::vec3> Colors(){
    colors.resize(particles.size());

    for (size_t i = 0; i < particles.size(); i++){
        colors[i] = glm::vec3(1.0f);
    }

    return std::span(colors);
}