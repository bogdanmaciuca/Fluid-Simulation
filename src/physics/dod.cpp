// destept land
#include "../ui.hpp"
#include "../main_loop.hpp"
#include "physics.hpp"

namespace dod
{

    std::tuple<std::vector<int>, std::vector<int>> grid;
    std::vector<u_int> cel_first_particle;
    std::vector<u_int> cel_particle_ids;

    glm::ivec3 grid_size;
    float cell_size;
    int particle_count;
    float density_area;
    float delta_time;
    float da2;

    glm::vec3 g = glm::vec3(0.0,-9.81,0.0);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<float> m;

    std::vector<float> radious; // why is it misspelled?
    std::vector<float> density;
    std::vector<float> pressure;
    std::vector<glm::vec3> force;

    namespace {

        float Poly6Kernel(float r2) {
            const float diff = da2 - r2;
            if (diff <= 0.0f)
            {
                return 0.0f;
            }

            return 315.0f / (64.0f * 3.0f * std::pow(density_area,9)) * diff * diff * diff;
        }

        float TaitEquation(float rest_density, float exponent, float stiffness, float particle_density) {
            return stiffness * rest_density / exponent * (std::pow(particle_density / rest_density, exponent) - 1.0f);
        }

        glm::vec3 SpikyGradient(const glm::vec3 &r_vec, float r) {
            if (r <= 1e-6f || r >= density_area)
            {
                return glm::vec3(0.0f);
            }

            const float diff = density_area - r;
            const float coef = -45.0f / (3.0f * da2 * da2 * da2) * diff * diff / r;
            return r_vec * coef;
        }

        void CalcCellDensity(int particle, int cell_start, int cell_end) { //what does calc stand for? 
            for (int i = cell_start; i < cell_end; ++i)
            {
                int neighbor = cel_particle_ids[i];

                float dx = positions[neighbor].x - positions[particle].x;
                float dy = positions[neighbor].y - positions[particle].y;
                float dz = positions[neighbor].z - positions[particle].z;

                float range = dx * dx + dy * dy + dz * dz;
                float density_impact = Poly6Kernel(range);

                density[particle] += density_impact;
                density[neighbor] += density_impact;
            }
        }

        void CalcCellForce(int particle, int cell_start, int cell_end) {
            for (int i = cell_start; i < cell_end; ++i)
            {
                int neighbor = cel_particle_ids[i];

                float dx = positions[neighbor].x - positions[particle].x;
                float dy = positions[neighbor].y - positions[particle].y;
                float dz = positions[neighbor].z - positions[particle].z;

                float range = std::sqrt(dx * dx + dy * dy + dz * dz);
                glm::vec3 force_impact =
                    m[neighbor] * (pressure[particle] + pressure[neighbor]) /
                    (2.0f * density[neighbor]) * SpikyGradient(glm::vec3(dx, dy, dz), range);

                force[particle] += force_impact;
                force[neighbor] -= force_impact;
            }
        }

        void CalcForce(int particle_position) {
            density[particle_position] = 0.0f;
            pressure[particle_position] = 0.0f;
            

            float particle_x = positions[particle_position].x;
            float particle_y = positions[particle_position].y;
            float particle_z = positions[particle_position].z;

            int x_cell_coordinate = floor(particle_x / cell_size);
            int y_cell_coordinate = floor(particle_y / cell_size);
            int z_cell_coordinate = floor(particle_z / cell_size);

            int cell = z_cell_coordinate * grid_size.x * grid_size.y + y_cell_coordinate * grid_size.x + x_cell_coordinate;
    
        

            int cell_start = particle_position + 1;
            int cell_end;
            if (z_cell_coordinate == grid_size.z && y_cell_coordinate == grid_size.y && x_cell_coordinate == grid_size.x){
                cell_end = particle_count;
            }else {
                cell_end = cel_first_particle[cell + 1];
                
                CalcCellDensity(particle_position, cell_start, cell_end);
                
                if ((x_cell_coordinate + 1) * cell_size - positions[particle_position].x < density_area){
                    cell_start = cel_first_particle[cell + 1];
                    cell_end = cel_first_particle[cell + 2];
                    
                    CalcCellDensity(particle_position, cell_start, cell_end);
                }
                
                if ((y_cell_coordinate + 1) * cell_size - positions[particle_position].y < density_area){
                    cell_start = cel_first_particle[cell + grid_size.x];
                    cell_end = cel_first_particle[cell + grid_size.x + 1];
                    
                    CalcCellDensity(particle_position, cell_start, cell_end);
                }
                
                if (((x_cell_coordinate + 1) * cell_size - positions[particle_position].x) * ((x_cell_coordinate + 1) * cell_size - positions[particle_position].x) + ((y_cell_coordinate + 1) * cell_size - positions[particle_position].y) * ((y_cell_coordinate + 1) * cell_size - positions[particle_position].y) < da2){
                    cell_start = cel_first_particle[cell + grid_size.x + 1];
                    cell_end = cel_first_particle[cell + grid_size.x + 2];
                    
                    CalcCellDensity(particle_position, cell_start, cell_end);
                }
                
                if ((z_cell_coordinate + 1) * cell_size - positions[particle_position].z < density_area){
                    cell_start = cel_first_particle[cell + grid_size.x * grid_size.y];
                    cell_end = cel_first_particle[cell + grid_size.x * grid_size.y + 1];
                    
                    CalcCellDensity(particle_position, cell_start, cell_end);
                }
                
                if (((x_cell_coordinate + 1) * cell_size - positions[particle_position].x) * ((x_cell_coordinate + 1) * cell_size - positions[particle_position].x) + ((z_cell_coordinate + 1) * cell_size - positions[particle_position].z) * ((z_cell_coordinate + 1) * cell_size - positions[particle_position].z) < da2){
                    cell_start = cel_first_particle[cell + 1 + grid_size.x * grid_size.y];
                    cell_end = cel_first_particle[cell + 2 + grid_size.x * grid_size.y];
                    
                    CalcCellDensity(particle_position, cell_start, cell_end);
                }
                
                if (((z_cell_coordinate + 1) * cell_size - positions[particle_position].z) * ((z_cell_coordinate + 1) * cell_size - positions[particle_position].z) + ((y_cell_coordinate + 1) * cell_size - positions[particle_position].y) * ((y_cell_coordinate + 1) * cell_size - positions[particle_position].y) < da2){
                    cell_start = cel_first_particle[cell + grid_size.x + grid_size.x * grid_size.z];
                    cell_end = cel_first_particle[cell + grid_size.x + grid_size.x * grid_size.z + 1];
                    
                    CalcCellDensity(particle_position, cell_start, cell_end);
                }
                
                if (((z_cell_coordinate + 1) * cell_size - positions[particle_position].z) * ((z_cell_coordinate + 1) * cell_size - positions[particle_position].z) + ((y_cell_coordinate + 1) * cell_size - positions[particle_position].y) * ((y_cell_coordinate + 1) * cell_size - positions[particle_position].y) + ((x_cell_coordinate + 1) * cell_size - positions[particle_position].x) * ((x_cell_coordinate + 1) * cell_size - positions[particle_position].y) < da2 * density_area){
                    cell_start = cel_first_particle[cell + grid_size.x * grid_size.y + grid_size.x + 1];
                    cell_end = cel_first_particle[cell + grid_size.x * grid_size.y + grid_size.x + 2];
                    
                    CalcCellDensity(particle_position, cell_start, cell_end);
                }
                
                pressure[particle_position] = TaitEquation(0.003, 7, 2, density[particle_position]);
                
                cell_start = cel_first_particle[cell];
                cell_end = particle_position;
                
                CalcCellForce(particle_position, cell_start, cell_end);
                
                if (x_cell_coordinate * cell_size - positions[particle_position].x < density_area && cell > 0){
                    cell_start = cel_first_particle[cell - 1];
                    cell_end = cel_first_particle[cell];
                    
                    CalcCellForce(particle_position, cell_start, cell_end);
                }
                
                if (y_cell_coordinate * cell_size - positions[particle_position].y < density_area){
                    cell_start = cel_first_particle[cell - grid_size.x];
                    cell_end = cel_first_particle[cell - grid_size.x + 1];
                    
                    CalcCellForce(particle_position, cell_start, cell_end);
                }
                
                if ((x_cell_coordinate * cell_size - positions[particle_position].x) * (x_cell_coordinate * cell_size - positions[particle_position].x) + (y_cell_coordinate * cell_size - positions[particle_position].y) * (y_cell_coordinate * cell_size - positions[particle_position].y) < da2){
                    cell_start = cel_first_particle[cell - grid_size.x - 1];
                    cell_end = cel_first_particle[cell - grid_size.x];
                    
                    CalcCellForce(particle_position, cell_start, cell_end);
                }
                
                if (z_cell_coordinate * cell_size - positions[particle_position].z < density_area){
                    cell_start = cel_first_particle[cell - grid_size.x * grid_size.y];
                    cell_end = cel_first_particle[cell - grid_size.x * grid_size.y + 1];
                    
                    CalcCellForce(particle_position, cell_start, cell_end);
                }
                
                if ((x_cell_coordinate * cell_size - positions[particle_position].x) * (x_cell_coordinate * cell_size - positions[particle_position].x) + (z_cell_coordinate * cell_size - positions[particle_position].z) * (z_cell_coordinate * cell_size - positions[particle_position].z) < da2){
                    cell_start = cel_first_particle[cell - 1 - grid_size.x * grid_size.y];
                    cell_end = cel_first_particle[cell - grid_size.x * grid_size.y];
                    
                    CalcCellForce(particle_position, cell_start, cell_end);
                }
                
                if ((z_cell_coordinate * cell_size - positions[particle_position].z) * (z_cell_coordinate * cell_size - positions[particle_position].z) + (y_cell_coordinate * cell_size - positions[particle_position].y) * (y_cell_coordinate * cell_size - positions[particle_position].y) < da2){
                    cell_start = cel_first_particle[cell - grid_size.x - grid_size.y * grid_size.x];
                    cell_end = cel_first_particle[cell - grid_size.x - grid_size.y * grid_size.x + 1];
                    
                    CalcCellForce(particle_position, cell_start, cell_end);
                }
                
                if ((z_cell_coordinate * cell_size - positions[particle_position].z) * (z_cell_coordinate * cell_size - positions[particle_position].z) + (y_cell_coordinate * cell_size - positions[particle_position].y) * (y_cell_coordinate * cell_size - positions[particle_position].y) * (x_cell_coordinate * cell_size - positions[particle_position].y) < da2 * density_area){
                    cell_start = cel_first_particle[cell - grid_size.x * grid_size.y - grid_size.x - 1];
                    cell_end = cel_first_particle[cell - grid_size.x * grid_size.y - grid_size.x];
                    
                    CalcCellForce(particle_position, cell_start, cell_end);
                }
            }
                
            force[particle_position] += g;
        }

        void ResetForce(int particle_position){
            force[particle_position] = glm::vec3(0.0f);
        }   

    } 
    
    
    float last_x = 0.01; 
    float last_y = 0.01; 
    float last_z = 0.01;
    
    float default_x, default_y, default_z;
    
    void CreateLiquidParticlesBegine(){
        if (last_x < grid_size.x * cell_size){
            last_x+=0.01;
        }
        else if (last_y < grid_size.y * cell_size) {
            last_x = 0.01;
            last_y+=0.01;
        }
        else if (last_z < grid_size.z * cell_size) {
            last_y = 0.01;
            last_z+=0.01;
        }
        
        positions.push_back(glm::vec3(last_x,last_y,last_z));
        radious.push_back(1.0);
    }

    void UpdateGridSize(){
        grid_size = g_ui.box_size / cell_size;
        
        default_x =  cell_size * grid_size.x / 2;
        default_y = cell_size * grid_size.y * 3 / 4;
        default_z = cell_size * grid_size.z / 2;
    }
    
    
    void PhysicsUpdate() {
        // grid = create_grid();

        
        UpdateGridSize();

        std::tie(cel_first_particle, cel_particle_ids) = GenerateGrid(positions, grid_size, 1.0f/ cell_size);
        
        // cel_first_particle = std::get<0>(grid);
        // cel_particle_ids = std::get<1>(grid);
        
        for (int i : cel_particle_ids){
            CalcForce(i);
        }

        for(int i = 0; i < particle_count; i++){
            positions[i] += force[i] * delta_time;
            ResetForce(i);
        }
    }

    std::span<glm::vec3> PhyisicsGetPositions(){
        return positions;
    }

    std::span<glm::vec3> PhysicsGetColors(){
        colors.resize(particle_count);
        for (int i = 0; i < particle_count; ++i){
            colors[i] = glm::vec3(0.0,0.0,1.0);
        }

        return  std::span(colors);
    }

    void PhysicsInit() {
        delta_time = FIXED_TIMESTEP;
        grid_size = g_ui.box_size / g_ui.grid_cell_size;
        cell_size = g_ui.grid_cell_size;
        density_area = cell_size; // wip?
        da2 = density_area * density_area;
        particle_count = 0;

        default_x = cell_size * grid_size.x / 2.0;
        default_y = cell_size * grid_size.y / 4.0 * 3;
        default_z = cell_size * grid_size.z / 2.0;

        for (int i = 0; i < particle_count; i++){
            CreateLiquidParticlesBegine();
        }
    }

    void IncreaseParticles(){
        particle_count++;
        positions.push_back(glm::vec3(default_x, default_y, default_z));
        radious.push_back(1.0);
    }

    void DecreaseParticles(){
        particle_count--;
        positions.pop_back();
        radious.pop_back();
    }
}
