//
//  camera.h
//  skinned-animation
//
//  Created by tigertang on 2018/8/4.
//  Copyright © 2018 tigertang. All rights reserved.
//

#pragma once

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

class Camera {
public:
    Camera();
    //SETTERS
    void setPos(glm::vec3 p);
    void setFront(glm::vec3 fr);
    void setUp(glm::vec3 u);
    void setTarget(glm::vec3 tar);
    void setDirection(glm::vec3 dir);
    void setRight(glm::vec3 r);
    //GETTERS
    glm::vec3 getPos();
    glm::vec3 getFront();
    glm::vec3 getUp();
    glm::vec3 getTarget();
    glm::vec3 getDirection();
    glm::vec3 getRight();
    
    void Rotate(double y, double p);

private:
    /*
    //Cam Rotation
    float yaw = -90.0f;
    float pitch = 0.0f;
    float speed = 2.5f;
    const float sensitivity = 0.1f;
    float zoom = 45.0f;*/

    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up; 
    //CamDir
    glm::vec3 target;
    glm::vec3 direction;
    //Ref Axis
    glm::vec3 right;
    
};
