//
//  camera.cpp
//  skinned-animation
//
//  Created by tigertang on 2018/8/4.
//  Copyright © 2018 tigertang. All rights reserved.
//

#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

using namespace glm;

//const double Camera::kMaxElevationAngle = 5 * pi<double>() / 12;

Camera::Camera() {
    pos = glm::vec3(29.26f, 0.31f, -24.32f);
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    up = glm::vec3(0.0f, 1.0f, 0.0f);
    target = glm::vec3(0.0f, 0.0f, 12.0f); //glm::vec3(14.1339, 0.8, -12);
    direction = glm::normalize(pos - target);
    right = glm::normalize(glm::cross(up, direction));
}

void Camera::setPos(glm::vec3 p) {
    this->pos = p;
}
void Camera::setFront(glm::vec3 fr) {
    this->front = fr;
}
void Camera::setUp(glm::vec3 u) {
    this->up = u;
}
void Camera::setTarget(glm::vec3 tar) {
    this->target = tar;
}
void Camera::setDirection(glm::vec3 dir) {
    this->direction = dir;
}
void Camera::setRight(glm::vec3 r) {
this->right = r;
}

glm::vec3 Camera::getPos() { return this->pos; }
glm::vec3 Camera::getFront() { return this->front; }
glm::vec3 Camera::getUp() { return this->up; }
glm::vec3 Camera::getTarget() { return this->target; }
glm::vec3 Camera::getDirection() { return this->direction; }
glm::vec3 Camera::getRight() { return this->right; }

void Camera::Rotate(double y, double p) {

    this->direction.x = cos(glm::radians(y)) * cos(glm::radians(p));
    this->direction.y = sin(glm::radians(p));
    this->direction.z = sin(glm::radians(y)) * cos(glm::radians(p));
    this->front= glm::normalize(this->direction);
}

