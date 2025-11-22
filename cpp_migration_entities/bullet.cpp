#pragma once

#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#include "SFML/System.hpp"
#include "generic_object.cpp"
#include <random>
#include <algorithm>
#include "main.cpp"

#define FRAME_COUNT 8
constexpr float PI = 3.14159265358979323846f;

void bullet_garbage_collector(std::vector<bullet *> &bullets)
{
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                 [](bullet *b)
                                 {
                                     if (!b->active)
                                     {
                                         delete b;
                                         return true;
                                     }
                                     return false;
                                 }),
                  bullets.end());
}

class bullet : public generic_object
{
private:
    float velocity_life_time = 0;
    float hue;
    float saturation;
    float value;
    float alpha;
    std::pair<int, int> *player_position;
    std::pair<int, int> starting_player_position;

    int animationSet;
    int animationSequence[FRAME_COUNT] = {1, 2, 3, 4, 5, 4, 3, 2};
    int animationIndex = 1;
    float animationTimer = 0;
    float animationSpeed = 0.08;
    float scale;

    int speed;
    int base_size;
    int base_speed;
    int bounce_count = 0;
    int max_bounces;

    std::pair<float, float> velocity;

    float velocity_boost;
    float velocity_decay_rate;

    void homing(float dt, std::pair<int, int> *enemy_position);

public:
    bool active = true;

    bullet(int x, int y, std::pair<int, int> *target, int midi, int key_velocity, int colorscheme, int max_bounces = 3, int base_size = 10, int base_speed = 120, float velocity_decay_rate = 4) : generic_object(x, y, base_size, base_size, "assets/bullet/")
    {
        this->speed = speed;
        this->base_size = base_size;
        this->max_bounces = max_bounces;

        this->player_position = target;
        this->starting_player_position = *target;

        float dx = target->first - x;
        float dy = target->second - y;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Normalize direction
        if (distance > 0)
        {
            dx = dx / distance;
            dy = dy / distance;
        }

        int midi_clamped = std::clamp(midi - 21, 0, 88);
        float scaleFactor = 3 - ((midi_clamped - 1) / (88 - 1) * 2); // Scale factor between 1.0 and 3.0
        width = static_cast<int>(base_size * scaleFactor);
        height = static_cast<int>(base_size * scaleFactor);
        this->base_speed = base_speed;

        this->speed = static_cast<int>(base_speed * (4 - scaleFactor));

        this->velocity_boost = key_velocity / 127.0f * speed;

        this->hue = colorscheme / 360.0;

        this->saturation = 0.4 + (midi / 128) * 0.6;

        this->value = 0.5 + (key_velocity / 127) * 0.5;

        this->alpha = 0.6 + (key_velocity / 127) * 0.4;

        float real_speed = this->speed + this->velocity_boost;
        velocity.first = (real_speed * dx);
        velocity.second = (real_speed * dy);

        this->scale = 2.7 * scaleFactor;

        this->animationSet = std::rand() % 4 + 1;
    }

    void homeless_update(float dt);
    void update(float dt, std::pair<int, int> enemy_position);

    ~bullet()
    {
        delete player_position;
    };
};

void bullet::homing(float dt, std::pair<int, int> *enemy_position)
{

    std::pair<float, float> toPlayer = {player_position->first - this->position.first, player_position->second - this->position.second};
    std::pair<float, float> toEnemy = {enemy_position->first - this->player_position->first, enemy_position->second - this->player_position->second};

    float pte_distance = sqrt(toEnemy.first * toEnemy.first + toEnemy.second * toEnemy.second);

    float maxX = VIRTUAL_WIDTH - enemy_position->first;
    float maxY = VIRTUAL_HEIGHT - enemy_position->second;

    float largest_distance = sqrt((maxX * maxX) + (maxY * maxY));

    float distanceRatio = pte_distance / largest_distance;

    float proximity = 1 - std::min(1.0f, std::max(0.0f, distanceRatio));

    float baseTurnDeg = 5;
    float maxExtraDeg = 175;
    float exponent = 6;

    float homingBoostDeg = maxExtraDeg * pow(proximity, exponent);

    float maxTurnRate = (baseTurnDeg + homingBoostDeg) * (PI / 180.0f);

    float angleCurr = atan2(this->velocity.second, this->velocity.first);
    float angleTarget = atan2(toPlayer.second, toPlayer.first);

    float delta = angleTarget - angleCurr;
    while (delta > PI)
    {
        delta = delta - 2 * PI;
    }
    while (delta < -PI)
    {
        delta = delta + 2 * PI;
    }

    float maxTurn = maxTurnRate * (dt ? dt : 1);

    if (delta > maxTurn)
    {
        delta = maxTurn;
    }
    else if (delta < -maxTurn)
    {
        delta = -maxTurn;
    }

    float newAngle = angleCurr + delta;
    float speed = sqrt(this->velocity.first * this->velocity.first + this->velocity.second * this->velocity.second);
    if (speed > 0)
    {
        this->velocity.first = cos(newAngle) * speed;
        this->velocity.second = sin(newAngle) * speed;
    }
}

void bullet::update(float dt, std::pair<int, int> enemy_position)
{
    if (!this->active)
    {
        return;
    };

    this->animationTimer = this->animationTimer + dt;
    if (this->animationTimer >= this->animationSpeed)
    {
        this->animationTimer = this->animationTimer - this->animationSpeed;
        this->animationIndex = this->animationIndex + 1;
        if (this->animationIndex > FRAME_COUNT)
        {
            this->animationIndex = 1;
        }
    }

    if (this->velocity_boost > 0)
    {
        this->velocity_life_time = this->velocity_life_time + dt;

        float initialBoost = (this->base_speed);
        this->velocity_boost = initialBoost * exp(-this->velocity_decay_rate * this->velocity_life_time);

        if (this->velocity_boost < 0.5)
        {
            this->velocity_boost = 0;
        }

        this->speed = this->base_speed + this->velocity_boost;

        float currentMag = sqrt(this->velocity.first * this->velocity.first + this->velocity.second * this->velocity.second);
        if (currentMag > 0)
        {
            float dirX = this->velocity.first / currentMag;
            float dirY = this->velocity.second / currentMag;
            this->velocity.first = dirX * this->speed;
            this->velocity.second = dirY * this->speed;
        }
    }

    if (this->bounce_count == 0)
    {
        this->homing(dt, &enemy_position);
    }

    this->position.first = this->position.first + this->velocity.first * dt;
    this->position.second = this->position.second + this->velocity.second * dt;
    bool bounced = false;

    if (this->position.first < 0)
    {
        this->position.first = 0;
        this->velocity.first = std::abs(this->velocity.first);
        bounced = true;
    }
    else if (this->position.first + this->width > VIRTUAL_WIDTH)
    {
        this->position.first = VIRTUAL_WIDTH - this->width;
        this->velocity.first = -std::abs(this->velocity.first);
        bounced = true;
    }

    if (this->position.second < 0)
    {
        this->position.second = 0;
        this->velocity.second = std::abs(this->velocity.second);
        bounced = true;
    }
    else if (this->position.second + this->height > VIRTUAL_HEIGHT)
    {
        this->position.second = VIRTUAL_HEIGHT - this->height;
        this->velocity.second = -std::abs(this->velocity.second);
        bounced = true;
    }

    if (bounced)
    {
        this->bounce_count = this->bounce_count + 1;
        if (this->bounce_count > this->max_bounces)
        {
            this->active = false;
        }
        // float mag = sqrt(this->velocity.first * this->velocity.first + this->velocity.second * this->velocity.second);
        // if (mag > 0) {
        //     float normVx = this->velocity.first / mag;
        //     float normVy = this->velocity.second / mag;
        //     this->perpX = -normVy;
        //     this->perpY = normVx;
        // }
    }
}

void bullet::homeless_update(float dt)
{
}