#pragma once

#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#include "SFML/System.hpp"
#include "generic_object.h"
#include <random>
#include <algorithm>

#define FRAME_COUNT 8
constexpr float PI = 3.14159265358979323846f;

#define VIRTUAL_WIDTH 1280
#define VIRTUAL_HEIGHT 720

class bullet : public generic_object
{
private:
    float velocity_life_time = 0;
    float hue;
    float saturation;
    float value;
    float alpha;
    std::pair<float, float> target;
    std::pair<float, float> starting_player_position;

    int animationSet;
    int animationSequence[FRAME_COUNT] = {1, 2, 3, 4, 5, 4, 3, 2};
    int animationIndex = 1;
    float animationTimer = 0;
    float animationSpeed = 0.08;
    // float scale;

    // int base_size;
    int base_speed;
    int bounce_count = 0;
    int max_bounces;

    std::pair<float, float> velocity;

    float velocity_boost;
    float velocity_decay_rate;

    void homing(float dt, std::pair<int, int> *enemy_position, std::pair<int, int> *player_position);

public:
    bool active = true;

    bullet(int x, int y, std::pair<float, float> target, int midi, int key_velocity, int colorscheme, int max_bounces = 3, int base_size = 10, int base_speed = 120, float velocity_decay_rate = 4) : generic_object(x, y, base_size, base_size, "sprites/projectile/")
    {
        // this->base_size = base_size;
        this->max_bounces = max_bounces;

        this->target = target;
        this->starting_player_position = target;

        float dx = target.first - x;
        float dy = target.second - y;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Normalize direction
        if (distance > 0)
        {
            dx = dx / distance;
            dy = dy / distance;
        }

        int midi_clamped = std::clamp(midi - 21, 0, 88);
        float scaleFactor = 5 - (((float)(midi_clamped - 1) / (88 - 1)) * 4); // Scale factor between 1.0 and 3.0
        this->width = static_cast<int>(base_size * scaleFactor);
        this->height = static_cast<int>(base_size * scaleFactor);
        std::cout << midi_clamped << " -  " << midi << " -  " << scaleFactor << std::endl;

        this->base_speed = base_speed;

        // this->base_size = static_cast<int>(base_speed * (4 - scaleFactor));

        this->velocity_boost = (key_velocity / 127.0f) * base_speed;

        this->hue = colorscheme / 360.0;

        this->saturation = 0.4 + (midi / 128) * 0.6;

        this->value = 0.5 + (key_velocity / 127) * 0.5;

        this->alpha = 0.6 + (key_velocity / 127) * 0.4;

        float real_speed = this->base_speed + this->velocity_boost;
        velocity.first = (real_speed * dx);
        velocity.second = (real_speed * dy);

        // this->scale = 2.7 * scaleFactor;

        this->animationSet = std::rand() % 4 + 1;
    }

    void homeless_update(float dt);
    void update(float dt, std::pair<int, int> enemy_position, std::pair<int, int> player_position);
    void draw(sf::RenderWindow &window);
};

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