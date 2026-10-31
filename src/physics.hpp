#pragma once

struct Physics {
    void Update(float timestep);
private:
    std::vector<glm::vec3> m_pos;
    std::vector<glm::vec3> m_color;
    std::vector<float>     m_density;
};

