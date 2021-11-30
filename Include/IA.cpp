#include <glm/gtc/matrix_transform.hpp>
#include "IA.h"
#include <cmath>
#include <iostream>

using namespace glm;
using namespace std;


IA::IA() {
    pos = glm::vec3(20.3f, 0.3f, -12.70f);
    front = glm::vec3(0.0f, 0.0f, 1.0f);
    active = false;
    target = glm::vec3(29.26f, 0.0f, -24.32f); //glm::vec3(14.1339, 0.8, -12);
    dir = glm::vec3(0.0f, 0.0f, 1.0f);
    speed = 0.0f;
}

void IA::setPos(glm::vec3 p) {
    this->pos = p;
}
void IA::setFront(glm::vec3 fr) {
    this->front = fr;
}
void IA::setTarget(glm::vec3 tar) {
    this->target = tar;
}
void IA::setDirection(glm::vec3 dir) {
    this->dir = dir;
}
void IA::setSpeed(float s) {
    this->speed = 0.02f * s;
}
void IA::start(glm::vec3 tar, int diff) {
    //diff = 0;
    this->active = true;
    this->speed = 0.01f;// *diff;
    update(tar);
}
void IA::update(glm::vec3 target) {
    if (target != this->target) {
        this->setTarget(target);
        this->setDirection(glm::normalize(this->target - this->pos));
    }
    this->pos.x += this->getSpeed() * this->getDirection().x;
    this->pos.z += this->getSpeed() * this->getDirection().z;
    this->front = glm::normalize(this->dir);
}


glm::vec3 IA::getPos() { return this->pos; }
glm::vec3 IA::getFront() { return this->front; }
glm::vec3 IA::getTarget() { return this->target; }
glm::vec3 IA::getDirection() { return glm::normalize(this->dir); }
float IA::getSpeed() { return this->speed; }
bool IA::isActive() { return this->active; }

bool IA::gameOver() { 
    if ((glm::distance(this->target.x, this->pos.x) <= this->distanceGameOver) && (glm::distance(this->target.z, this->pos.z) <= this->distanceGameOver)){
        this->active = false;
        this->dir = glm::vec3(0.f);
        return true; //true
    }
    return false;
}

