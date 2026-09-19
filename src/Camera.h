#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    glm::vec3 position;
    float yaw;    // rotace kolem Y osy (doleva/doprava)
    float pitch;  // rotace nahoru/dolu

    Camera(glm::vec3 startPos)
        : position(startPos), yaw(-90.0f), pitch(0.0f)
    {
    }

    glm::vec3 GetFront()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::normalize(front);
    }

    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(position, position + GetFront(), glm::vec3(0.0f, 1.0f, 0.0f));
    }
};