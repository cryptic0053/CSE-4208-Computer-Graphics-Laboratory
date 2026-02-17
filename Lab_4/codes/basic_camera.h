//
//  basic_camera.h
//  test
//
//  Created by Nazirul Hasan on 10/9/23.
//  modified by Badiuzzaman on 3/11/24.
//

#ifndef basic_camera_h
#define basic_camera_h

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
};

class BasicCamera {
public:

    glm::vec3 eye;
    glm::vec3 lookAt;
    glm::vec3 V;
    float Yaw, Pitch, Roll;
    float Zoom;
    glm::mat4 viewRot = glm::mat4(1.0f); // for pitch, yaw, and roll implementation

    BasicCamera(float eyeX = 0.0, float eyeY = 1.0, float eyeZ = 3.0, float lookAtX = 0.0, float lookAtY = 0.0, float lookAtZ = 0.0, glm::vec3 viewUpVector = glm::vec3(0.0f, 1.0f, 0.0f))
    {
        eye = glm::vec3(eyeX, eyeY, eyeZ);
        lookAt = glm::vec3(lookAtX, lookAtY, lookAtZ);
        V = viewUpVector;

        Yaw = 0.0f;
        Pitch = 0.0f;
        Roll = 0.0f;
        Zoom = 45.0;
    }

    glm::mat4 createViewMatrix()
    {
        return glm::lookAt(eye, lookAt, V);
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

    void move(Camera_Movement direction, float deltaTime)
    {
        float velocity = 2.5 * deltaTime;

        if (direction == FORWARD) {
            eye[2] -= velocity;
            lookAt[2] -= velocity;
        }
        if (direction == BACKWARD) {
            eye[2] +=  velocity;
            lookAt[2] +=  velocity;
        }
        if (direction == LEFT) {
            eye[0] -=  velocity;
            lookAt[0] -=  velocity;
        }
        if (direction == RIGHT) {
            eye[0] +=  velocity;
            lookAt[0] +=  velocity;
        }
    }
};

#endif /* basic_camera_h */
#pragma once

