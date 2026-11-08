#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <tuple>
#include <vector>
#include "../ui.hpp"

std::tuple<std::vector<int>, std::vector<int>> grid;

std::vector<int> cel_first_particle;
std::vector<int> cel_particle_ids;

float cell_size;
int particle_count;
float density_area;
float da2; // precalculation for density_area^2
glm::vec3 gravity(0.0, -9.81, 0.0);
glm::ivec3 grid_size;

// Pi = 3 btw

float Poly6Kernel(float r2){
    float diff = da2 - r2;

    if (diff <= 0.0f){
        return 0.0f;
    }

    return 315.0f / (64.0f * 3.0f * std::pow(density_area,9)) * diff * diff * diff;
}

float TaitEquation(float rest_density, float exponent, float stiffness, float density){
    return stiffness * rest_density / exponent * (std::pow(density / rest_density, exponent) - 1);
}

glm::vec3 SpikyGradient(const glm::vec3& r_vec, float r) {
	if (r <= 1e-6f || r >= density_area) {
		return glm::vec3(0.0f);
	}
	float diff = density_area - r;
	float coef = -45.0f / (3 * da2 * da2 * da2) * diff * diff / r;
	return r_vec * coef;
}

class Particle{
    private:

    protected:
    float x, y, z; // coordonate
    float m;

    Particle(float x, float y, float z): x(x), y(y), z(z){}

    public:

    float GetX(){
        return x;
    }

    void IncreaseX(float x){
        this->x += x;
    }

    float GetY(){
        return y;
    }

    void IncreaseY(float y){
        this->y += y;
    }
    float GetZ(){
        return z;
    }

    void IncreaseZ(float z){
        this->z += z;
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
    float density, presure;

    glm::vec3 force;

    public:

    Liquid(float x, float y, float z, float r): Particle(x, y, z), radious(r){
        force = glm::vec3(0.0f);
    }

    void CalcCellDensity(int cell_start, int cell_end);

    void CalcCellForce(int cell_start, int cell_end);

    void CalcForce(int part_position){
        int x_cell_coordinate = floor(x / cell_size), y_cell_coordinate = floor(y / cell_size),
            z_cell_coordinate = floor(z / cell_size);

        
        int cell = z_cell_coordinate * grid_size.x * grid_size.y + y_cell_coordinate * grid_size.x + x_cell_coordinate;
    
        

        int cell_start = part_position + 1;
        int cell_end = cel_first_particle[cell + 1];

        CalcCellDensity(cell_start, cell_end);

        if ((x_cell_coordinate + 1) * cell_size - x < density_area){
            cell_start = cel_first_particle[cell + 1];
            cell_end = cel_first_particle[cell + 2];

            CalcCellDensity(cell_start, cell_end);
        }

        if ((y_cell_coordinate + 1) * cell_size - y < density_area){
            cell_start = cel_first_particle[cell + grid_size.x];
            cell_end = cel_first_particle[cell + grid_size.x + 1];

            CalcCellDensity(cell_start, cell_end);
        }

        if (((x_cell_coordinate + 1) * cell_size - x) * ((x_cell_coordinate + 1) * cell_size - x) + ((y_cell_coordinate + 1) * cell_size - y) * ((y_cell_coordinate + 1) * cell_size - y) < da2){
            cell_start = cel_first_particle[cell + grid_size.x + 1];
            cell_end = cel_first_particle[cell + grid_size.x + 2];

            CalcCellDensity(cell_start, cell_end);
        }

        if ((z_cell_coordinate + 1) * cell_size - z < density_area){
            cell_start = cel_first_particle[cell + grid_size.x * grid_size.y];
            cell_end = cel_first_particle[cell + grid_size.x * grid_size.y + 1];

            CalcCellDensity(cell_start, cell_end);
        }

        if (((x_cell_coordinate + 1) * cell_size - x) * ((x_cell_coordinate + 1) * cell_size - x) + ((z_cell_coordinate + 1) * cell_size - z) * ((z_cell_coordinate + 1) * cell_size - z) < da2){
            cell_start = cel_first_particle[cell + 1 + grid_size.x * grid_size.y];
            cell_end = cel_first_particle[cell + 2 + grid_size.x * grid_size.y];

            CalcCellDensity(cell_start, cell_end);
        }

        if (((z_cell_coordinate + 1) * cell_size - z) * ((z_cell_coordinate + 1) * cell_size - z) + ((y_cell_coordinate + 1) * cell_size - y) * ((y_cell_coordinate + 1) * cell_size - y) < da2){
            cell_start = cel_first_particle[cell + grid_size.x + grid_size.x * grid_size.z];
            cell_end = cel_first_particle[cell + grid_size.x + grid_size.x * grid_size.z + 1];

            CalcCellDensity(cell_start, cell_end);
        }

        if (((z_cell_coordinate + 1) * cell_size - z) * ((z_cell_coordinate + 1) * cell_size - z) + ((y_cell_coordinate + 1) * cell_size - y) * ((y_cell_coordinate + 1) * cell_size - y) + ((x_cell_coordinate + 1) * cell_size - x) * ((x_cell_coordinate + 1) * cell_size - y) < da2 * density_area){
            cell_start = cel_first_particle[cell + grid_size.x * grid_size.y + grid_size.x + 1];
            cell_end = cel_first_particle[cell + grid_size.x * grid_size.y + grid_size.x + 2];

            CalcCellDensity(cell_start, cell_end);
        }

        presure = TaitEquation(0.003, 7, 2, density);

        cell_start = cel_first_particle[cell];
        cell_end = part_position;

        CalcCellForce(cell_start, cell_end);

        if (x_cell_coordinate * cell_size - x < density_area){
            cell_start = cel_first_particle[cell - 1];
            cell_end = cel_first_particle[cell];

            CalcCellForce(cell_start, cell_end);
        }

        if (y_cell_coordinate * cell_size - y < density_area){
            cell_start = cel_first_particle[cell - grid_size.x];
            cell_end = cel_first_particle[cell - grid_size.x + 1];

            CalcCellForce(cell_start, cell_end);
        }

        if ((x_cell_coordinate * cell_size - x) * (x_cell_coordinate * cell_size - x) + (y_cell_coordinate * cell_size - y) * (y_cell_coordinate * cell_size - y) < da2){
            cell_start = cel_first_particle[cell - grid_size.x - 1];
            cell_end = cel_first_particle[cell - grid_size.x];

            CalcCellForce(cell_start, cell_end);
        }

        if (z_cell_coordinate * cell_size - z < density_area){
            cell_start = cel_first_particle[cell - grid_size.x * grid_size.y];
            cell_end = cel_first_particle[cell - grid_size.x * grid_size.y + 1];

            CalcCellForce(cell_start, cell_end);
        }

        if ((x_cell_coordinate * cell_size - x) * (x_cell_coordinate * cell_size - x) + (z_cell_coordinate * cell_size - z) * (z_cell_coordinate * cell_size - z) < da2){
            cell_start = cel_first_particle[cell - 1 - grid_size.x * grid_size.y];
            cell_end = cel_first_particle[cell - grid_size.x * grid_size.y];

            CalcCellForce(cell_start, cell_end);
        }

        if ((z_cell_coordinate * cell_size - z) * (z_cell_coordinate * cell_size - z) + (y_cell_coordinate * cell_size - y) * (y_cell_coordinate * cell_size - y) < da2){
            cell_start = cel_first_particle[cell - grid_size.x - grid_size.y * grid_size.x];
            cell_end = cel_first_particle[cell - grid_size.x - grid_size.y * grid_size.x + 1];

            CalcCellForce(cell_start, cell_end);
        }

        if ((z_cell_coordinate * cell_size - z) * (z_cell_coordinate * cell_size - z) + (y_cell_coordinate * cell_size - y) * (y_cell_coordinate * cell_size - y) * (x_cell_coordinate * cell_size - y) < da2 * density_area){
            cell_start = cel_first_particle[cell - grid_size.x * grid_size.y - grid_size.x - 1];
            cell_end = cel_first_particle[cell - grid_size.x * grid_size.y - grid_size.x];

            CalcCellForce(cell_start, cell_end);
        }

        force += gravity;
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
};

std::vector<Liquid> particles;

void Liquid::CalcCellDensity(int cell_start, int cell_end){ // Calc stands for Calculate
    for (int i = cell_start; i < cell_end; i++){
        Liquid& I = particles[cel_particle_ids[i]];
        float xi = I.GetX(), yi = I.GetY(), zi = I.GetZ();

        //from coordonates to distances
        xi = xi - x;
        yi = yi - y;
        zi = zi - z;
        
        float range = xi * xi + yi * yi + zi * zi;

        float density_impact = Poly6Kernel(range);

        density += density_impact;
        I.IncreaseDensity(density_impact);
    }
}

void Liquid::CalcCellForce(int cell_start, int cell_end){
    for (int i = cell_start; i < cell_end; i++){
        Liquid& I = particles[cel_particle_ids[i]];
        float xi = I.GetX(), yi = I.GetY(), zi = I.GetZ();

        //from coordonates to distances
        xi = xi - x;
        yi = yi - y;
        zi = zi - z;
        
        float range = std::sqrt(xi * xi + yi * yi + zi * zi);

        glm::vec3 force_impact = I.GetM() * (presure + I.GetPresure()) / (2 * I.GetDensity()) * SpikyGradient(glm::vec3(xi, yi, zi), range);

        force += force_impact;
        I.IncreaseForce(-force_impact);
    }
}

class LiquidFactory{
    private:
    float last_x = 1.0, last_y = 1.0, last_z = 1.0;
    float default_x, default_y, default_z;

    public:
    LiquidFactory(){}
    LiquidFactory(float x, float y, float z):default_x(x), default_y(y), default_z(z){}

    Liquid CreateLiquidParticleBegine(){
        if (last_x < grid_size.x * cell_size){
            last_x++;
        }
        else if(last_y < grid_size.y * cell_size){
            last_x = 1.0;
            last_y++;
        }
        else if(last_y < grid_size.y * cell_size){
            last_x = 1.0;
            last_y = 1.0;
            last_z++;
        }
        return Liquid(last_x, last_y, last_z, 1.0);
    }

    Liquid CreateLiquidParticle(){
        return Liquid(default_x, default_y, default_z, 1.0);
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

void PhysicsUpdate(){
    // grid = create_grid();

    cel_first_particle = std::get<0>(grid);
    cel_particle_ids = std::get<1>(grid);

    for(int i : cel_particle_ids){
        particles[i].CalcForce(i);
    }
}

void PhysicsInit(){
    cell_size = g_ui.grid_cell_size;
    grid_size = g_ui.box_size / cell_size;
    density_area = cell_size; //wip
    da2 = density_area * density_area;
    particle_count = 0;



    liquid_factory = LiquidFactory(cell_size * grid_size.x / 2, cell_size * grid_size.y * 3 / 4, cell_size * grid_size.z / 2);

    for(int i = 0; i < particle_count; i++){
        particles.push_back(liquid_factory.CreateLiquidParticleBegine());
    }
}

void IncreaseParticles(){
    particle_count++;
    particles.push_back(liquid_factory.CreateLiquidParticle());
}

void DecreaseParticles(){
    particles.pop_back();
    particle_count--;
}

void UpdateGridSize(glm::vec3 new_box_size){
    grid_size = g_ui.box_size / cell_size;
    
    liquid_factory.SetX(cell_size * grid_size.x / 2);
    liquid_factory.SetY(cell_size * grid_size.y * 3 / 4);
    liquid_factory.SetZ(cell_size * grid_size.z / 2);
}