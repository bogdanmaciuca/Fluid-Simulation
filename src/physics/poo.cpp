#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <tuple>
#include <vector>

std::tuple<std::vector<int>> grid;

std::vector<int> cel_first_particle;
std::vector<int> cel_particle_ids;

int grid_size;
int cell_size;
float density_area;
float da2; // precalculation for density_area^2


float Poly6Kernel(float r){
    float diff = da2 - r;

    if (diff <= 0.0f){
        return 0.0f;
    }

    return 4.0 / (3 * density_area) * diff * diff *diff;
}

float TaitEquation(float rest_density, float exponent, float stiffness, float density){
    return stiffness * rest_density / exponent * (std::pow(density / density_area, exponent) - 1);
}

glm::vec3 SpikyGradient(const glm::vec3& r_vec, float r) {
	if (r <= 1e-6f || r >= density_area) {
		return glm::vec3(0.0f);
	}
	float diff = density_area - r;
	float coef = -30.0f / (3 * da2 * da2 * density_area) * diff * diff;
	return r_vec * coef;
}

class Particle{
    private:

    protected:
    float x, y, z; // coordonate

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
};

std::vector<Particle*> particles;

class Liquid: public Particle{
    private:

    float radious;
    float density, presure;

    public:

    Liquid(float x, float y, float z, float r): Particle(x, y, z), radious(r){}

    void CalcCellPresure(int cell_start, int cell_end){
        for (int i = cell_start; i < cell_end; i++){
            Particle* I = particles[cel_particle_ids[i]];
            float xi = I->GetX(), yi = I->GetY(), zi = I->GetZ();

            //from coordonates to distances
            xi = xi - x;
            yi = yi - y;
            zi = zi - z;
            
            float range = xi * xi + yi * yi + zi * zi;

            float density_impact = Poly6Kernel(range);

            density += density_impact;
            static_cast<Liquid*>(I)->IncreaseDensity(density_impact);
        }
    }

    void CalcPreasure(){
        int x_cell_coordinate = floor(x / cell_size), y_cell_coordinate = floor(y / cell_size),
            z_cell_coordinate = floor(z / cell_size);

        int cell = x_cell_coordinate * grid_size * grid_size
                    + y_cell_coordinate * grid_size + z_cell_coordinate;
        
        int cell_start = cel_first_particle[cell];
        int cell_end = cel_first_particle[cell++];

        CalcCellPresure(cell_start, cell_end);

        if ((z_cell_coordinate + 1) * cell_size - z < density_area){
            cell_start = cel_first_particle[cell++];
            cell_end = cel_first_particle[cell + 2];

            CalcCellPresure(cell_start, cell_end);
        }

        if ((y_cell_coordinate + 1) * cell_size - y < density_area){
            cell_start = cel_first_particle[cell + grid_size];
            cell_end = cel_first_particle[cell + grid_size + 1];

            CalcCellPresure(cell_start, cell_end);
        }

        if (((z_cell_coordinate + 1) * cell_size - z) * ((y_cell_coordinate + 1) * cell_size - y) < da2){
            cell_start = cel_first_particle[cell + grid_size + 1];
            cell_end = cel_first_particle[cell + grid_size + 2];

            CalcCellPresure(cell_start, cell_end);
        }

        if ((x_cell_coordinate + 1) * cell_size - x < density_area){
            cell_start = cel_first_particle[cell + grid_size * grid_size];
            cell_end = cel_first_particle[cell + grid_size * grid_size + 1];

            CalcCellPresure(cell_start, cell_end);
        }

        if (((z_cell_coordinate + 1) * cell_size - z) * ((x_cell_coordinate + 1) * cell_size - x) < da2){
            cell_start = cel_first_particle[cell + 1 + grid_size * grid_size];
            cell_end = cel_first_particle[cell + 2 + grid_size * grid_size];

            CalcCellPresure(cell_start, cell_end);
        }

        if (((x_cell_coordinate + 1) * cell_size - x) * ((y_cell_coordinate + 1) * cell_size - y) < da2){
            cell_start = cel_first_particle[cell + grid_size + grid_size * grid_size];
            cell_end = cel_first_particle[cell + grid_size + grid_size * grid_size + 1];

            CalcCellPresure(cell_start, cell_end);
        }

        if (((z_cell_coordinate + 1) * cell_size - z) * ((y_cell_coordinate + 1) * cell_size - y) * ((x_cell_coordinate + 1) * cell_size - y) < da2 * density_area){
            cell_start = cel_first_particle[cell + grid_size * grid_size + grid_size + 1];
            cell_end = cel_first_particle[cell + grid_size * grid_size + grid_size + 2];

            CalcCellPresure(cell_start, cell_end);
        }

        presure = TaitEquation(0.003, 7, 2, density);


        CalcCellPresure(cell_start, cell_end);

        if ((z_cell_coordinate + 1) * cell_size - z < density_area){
            cell_start = cel_first_particle[cell++];
            cell_end = cel_first_particle[cell + 2];

            CalcCellPresure(cell_start, cell_end);
        }

        if ((y_cell_coordinate + 1) * cell_size - y < density_area){
            cell_start = cel_first_particle[cell + grid_size];
            cell_end = cel_first_particle[cell + grid_size + 1];

            CalcCellPresure(cell_start, cell_end);
        }

        if (((z_cell_coordinate + 1) * cell_size - z) * ((y_cell_coordinate + 1) * cell_size - y) < da2){
            cell_start = cel_first_particle[cell + grid_size + 1];
            cell_end = cel_first_particle[cell + grid_size + 2];

            CalcCellPresure(cell_start, cell_end);
        }

        if ((x_cell_coordinate + 1) * cell_size - x < density_area){
            cell_start = cel_first_particle[cell + grid_size * grid_size];
            cell_end = cel_first_particle[cell + grid_size * grid_size + 1];

            CalcCellPresure(cell_start, cell_end);
        }

        if (((z_cell_coordinate + 1) * cell_size - z) * ((x_cell_coordinate + 1) * cell_size - x) < da2){
            cell_start = cel_first_particle[cell + 1 + grid_size * grid_size];
            cell_end = cel_first_particle[cell + 2 + grid_size * grid_size];

            CalcCellPresure(cell_start, cell_end);
        }

        if (((x_cell_coordinate + 1) * cell_size - x) * ((y_cell_coordinate + 1) * cell_size - y) < da2){
            cell_start = cel_first_particle[cell + grid_size + grid_size * grid_size];
            cell_end = cel_first_particle[cell + grid_size + grid_size * grid_size + 1];

            CalcCellPresure(cell_start, cell_end);
        }

        if (((z_cell_coordinate + 1) * cell_size - z) * ((y_cell_coordinate + 1) * cell_size - y) * ((x_cell_coordinate + 1) * cell_size - y) < da2 * density_area){
            cell_start = cel_first_particle[cell + grid_size * grid_size + grid_size + 1];
            cell_end = cel_first_particle[cell + grid_size * grid_size + grid_size + 2];

            CalcCellPresure(cell_start, cell_end);
        }
    }

    void IncreaseDensity(float d){
        density += d;
    }

    float GetDensity(){
        return density;
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

class LiquidFactory{

};

void PhysicsUpdate(){
    // grid = create_grid();

    cel_first_particle = std::get<0>(grid);
    cel_particle_ids = std::get<1>(grid);
}

void PhysicsInit(){
    grid_size = std::cbrt(cel_first_particle.size());
    cell_size = 10; //wip
    density_area = 15; //wip
    da2 = density_area * density_area;
}