// destept land
#include <cmath>
#include <tuple>
#include <vector>

#include <glm/glm.hpp>

namespace dod
{

    std::tuple<std::vector<int>, std::vector<int>> grid;
    std::vector<int> cell_first_particle;
    std::vector<int> cell_particle_ids;

    int grid_size;
    int cell_size;
    float density_area;
    float da2;

    float g = 9.82;

    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> z;
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

        void CalcCellDensity(int particle, int cell_start, int cell_end) {
            for (int i = cell_start; i < cell_end; ++i)
            {
                int neighbor = cell_particle_ids[i];

                float dx = x[neighbor] - x[particle];
                float dy = y[neighbor] - y[particle];
                float dz = z[neighbor] - z[particle];

                float range = dx * dx + dy * dy + dz * dz;
                float density_impact = Poly6Kernel(range);

                density[particle] += density_impact;
                density[neighbor] += density_impact;
            }
        }

        void CalcCellForce(int particle, int cell_start, int cell_end) {
            for (int i = cell_start; i < cell_end; ++i)
            {
                int neighbor = cell_particle_ids[i];

                float dx = x[neighbor] - x[particle];
                float dy = y[neighbor] - y[particle];
                float dz = z[neighbor] - z[particle];

                float range = std::sqrt(dx * dx + dy * dy + dz * dz);
                glm::vec3 force_impact =
                    m[neighbor] * (pressure[particle] + pressure[neighbor]) /
                    (2.0f * density[neighbor]) * SpikyGradient(glm::vec3(dx, dy, dz), range);

                force[particle] += force_impact;
                force[neighbor] -= force_impact;
            }
        }

        void CalcForce(int particle_position) {
            float particle_x = x[particle_position];
            float particle_y = y[particle_position];
            float particle_z = z[particle_position];

            int x_cell_coordinate = floor(particle_x / cell_size);
            int y_cell_coordinate = floor(particle_y / cell_size);
            int z_cell_coordinate = floor(particle_z / cell_size);

            int cell = x_cell_coordinate * grid_size * grid_size +
                             y_cell_coordinate * grid_size + z_cell_coordinate;

            int cell_start = particle_position + 1;
            int cell_end = cell_first_particle[cell + 1];
            CalcCellDensity(particle_position, cell_start, cell_end);

            if ((z_cell_coordinate + 1) * cell_size - particle_z < density_area) {
                cell_start = cell_first_particle[cell + 1];
                cell_end = cell_first_particle[cell + 2];
                CalcCellDensity(particle_position, cell_start, cell_end);
            }

            if ((y_cell_coordinate + 1) * cell_size - particle_y < density_area) {
                cell_start = cell_first_particle[cell + grid_size];
                cell_end = cell_first_particle[cell + grid_size + 1];
                CalcCellDensity(particle_position, cell_start, cell_end);
            }

            if (((z_cell_coordinate + 1) * cell_size - particle_z) * ((z_cell_coordinate + 1) * cell_size - particle_z) +
                    ((y_cell_coordinate + 1) * cell_size - particle_y) * ((y_cell_coordinate + 1) * cell_size - particle_y) < da2) {
                cell_start = cell_first_particle[cell + grid_size + 1];
                cell_end = cell_first_particle[cell + grid_size + 2];
                CalcCellDensity(particle_position, cell_start, cell_end);
            }

            if ((x_cell_coordinate + 1) * cell_size - particle_x < density_area) {
                cell_start = cell_first_particle[cell + grid_size * grid_size];
                cell_end = cell_first_particle[cell + grid_size * grid_size + 1];
                CalcCellDensity(particle_position, cell_start, cell_end);
            }

            if (((z_cell_coordinate + 1) * cell_size - particle_z) * ((z_cell_coordinate + 1) * cell_size - particle_z) + 
                    ((x_cell_coordinate + 1) * cell_size - particle_x) * ((x_cell_coordinate + 1) * cell_size - particle_x) < da2) {
                cell_start = cell_first_particle[cell + 1 + grid_size * grid_size];
                cell_end = cell_first_particle[cell + 2 + grid_size * grid_size];
                CalcCellDensity(particle_position, cell_start, cell_end);
            }

            if (((x_cell_coordinate + 1) * cell_size - particle_x) * ((x_cell_coordinate + 1) * cell_size - particle_x) + 
                    ((y_cell_coordinate + 1) * cell_size - particle_y) * ((y_cell_coordinate + 1) * cell_size - particle_y) < da2) {
                cell_start = cell_first_particle[cell + grid_size + grid_size * grid_size];
                cell_end = cell_first_particle[cell + grid_size + grid_size * grid_size + 1];
                CalcCellDensity(particle_position, cell_start, cell_end);
            }

            if (((z_cell_coordinate + 1) * cell_size - particle_z) * ((z_cell_coordinate + 1) * cell_size - particle_z) + 
                    ((y_cell_coordinate + 1) * cell_size - particle_y) * ((y_cell_coordinate + 1) * cell_size - particle_y) + 
                    ((x_cell_coordinate + 1) * cell_size - particle_x) * ((x_cell_coordinate + 1) * cell_size - particle_y) < da2 * density_area) {
                cell_start = cell_first_particle[cell + grid_size * grid_size + grid_size + 1];
                cell_end = cell_first_particle[cell + grid_size * grid_size + grid_size + 2];
                CalcCellDensity(particle_position, cell_start, cell_end);
            }

            pressure[particle_position] = TaitEquation(0.003f, 7.0f, 2.0f, density[particle_position]);

            cell_start = cell_first_particle[cell];
            cell_end = particle_position;
            CalcCellForce(particle_position, cell_start, cell_end);

            if (z_cell_coordinate * cell_size - particle_z < density_area) {
                cell_start = cell_first_particle[cell - 1];
                cell_end = cell_first_particle[cell];
                CalcCellForce(particle_position, cell_start, cell_end);
            }

            if (y_cell_coordinate * cell_size - particle_y < density_area) {
                cell_start = cell_first_particle[cell - grid_size];
                cell_end = cell_first_particle[cell - grid_size + 1];
                CalcCellForce(particle_position, cell_start, cell_end);
            }

            if ((z_cell_coordinate * cell_size - particle_z) * (z_cell_coordinate * cell_size - particle_z) + 
                    (y_cell_coordinate * cell_size - particle_y) * (y_cell_coordinate * cell_size - particle_y) < da2) {
                cell_start = cell_first_particle[cell - grid_size - 1];
                cell_end = cell_first_particle[cell - grid_size];
                CalcCellForce(particle_position, cell_start, cell_end);
            }

            if (x_cell_coordinate * cell_size - particle_x < density_area) {
                cell_start = cell_first_particle[cell - grid_size * grid_size];
                cell_end =
                    cell_first_particle[cell - grid_size * grid_size + 1];
                CalcCellForce(particle_position, cell_start, cell_end);
            }

            if ((z_cell_coordinate * cell_size - particle_z) * (z_cell_coordinate * cell_size - particle_z) + 
                    (z_cell_coordinate * cell_size - particle_z) * (z_cell_coordinate * cell_size - particle_z) < da2) {
                cell_start = cell_first_particle[cell - 1 - grid_size * grid_size];
                cell_end = cell_first_particle[cell - grid_size * grid_size];
                CalcCellForce(particle_position, cell_start, cell_end);
            }

            if ((x_cell_coordinate * cell_size - particle_x) * (x_cell_coordinate * cell_size - particle_x) + 
                    (y_cell_coordinate * cell_size - particle_y) * (y_cell_coordinate * cell_size - particle_y) < da2) {
                cell_start = cell_first_particle[cell - grid_size - grid_size * grid_size];
                cell_end = cell_first_particle[cell - grid_size - grid_size * grid_size + 1];
                CalcCellForce(particle_position, cell_start, cell_end);
            }

            if ((z_cell_coordinate * cell_size - particle_z) * (z_cell_coordinate * cell_size - particle_z) + 
                    (y_cell_coordinate * cell_size - particle_y) * (y_cell_coordinate * cell_size - particle_y) * 
                    (x_cell_coordinate * cell_size - particle_y) < da2 * density_area) {
                cell_start = cell_first_particle[cell - grid_size * grid_size - grid_size - 1];
                cell_end = cell_first_particle[cell - grid_size * grid_size - grid_size];
                CalcCellForce(particle_position, cell_start, cell_end);
            }
        }

    } 

    void PhysicsUpdate() {
        // grid = create_grid();

        cell_first_particle = std::get<0>(grid);
        cell_particle_ids = std::get<1>(grid);
    }

    void PhysicsInit() {
        grid_size = std::cbrt(cell_first_particle.size());
        cell_size = 15; // wip?
        density_area = 15; // wip?
        da2 = density_area * density_area;
    }

}
