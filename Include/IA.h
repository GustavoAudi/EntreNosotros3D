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


class IA {
public:
    IA();
    //SETTERS
    void setPos(glm::vec3 p);
    void setFront(glm::vec3 fr);
    void setTarget(glm::vec3 tar);
    void setDirection(glm::vec3 dir);
    void setSpeed(float s);
    void start(glm::vec3 target, int diff);
    void update(glm::vec3 target);
    //GETTERS
    glm::vec3 getPos();
    glm::vec3 getFront();
    glm::vec3 getTarget();
    glm::vec3 getDirection();
    float getSpeed();
    bool isActive();
    bool gameOver();


private:
    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 target;
    glm::vec3 dir;
    float speed;
    bool active;
    const float distanceGameOver = 0.215;


};
