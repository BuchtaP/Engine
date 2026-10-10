#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{   
public:
    glm::vec3 position;
   
    float yaw;
    float pitch;
    float fov = 70.0f;
    float pitch;
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 200.0f;

    Camera(glm::vec3 startPos)
        : position(startPos), yaw(-90.0f), pitch(0.0f) 
        {}

    glm::vec3 GetFront() const
    {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::normalize(front);
    }

    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(position, position + GetFront(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 GetProjectionMatrix() const
    {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }   
};

//dodelat render