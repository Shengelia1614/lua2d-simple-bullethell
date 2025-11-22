
#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include "SFML/include/SFML/Graphics.hpp"
#include "SFML/include/SFML/Window.hpp"
#include "SFML/include/SFML/System.hpp"
#include "generic_object.cpp"
#include <random>
#include <algorithm>

#define VIRTUAL_WIDTH 800
#define VIRTUAL_HEIGHT 600
constexpr float PI = 3.14159265358979323846f;

class bullet : public generic_object
{
private:
    float velocity_life_time = 0;
    float hue;
    float saturation;
    float value;
    float alpha;
    std::pair<int, int> *player_position;

    int animationSet;
    int animationSequence[8] = {1, 2, 3, 4, 5, 4, 3, 2};
    int animationIndex = 1;
    float animationTimer = 0;
    float animationSpeed = 0.08;
    float scale;

public:
    std::pair<int, int> bullets;

    bullet(int x, int y, std::pair<int, int> *player_position, int midi, int key_velocity, int colorscheme, int base_size = 10, int base_speed = 120, float velocity_decay_rate = 4) : generic_object(x, y, base_size, base_size, "assets/bullet/")
    {
        this->speed = speed;
        this->base_size = base_size;

        this->player_position = player_position;

        float dx = player_position->first - x;
        float dy = player_position->second - y;
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

    void homing(float dt, std::pair<int, int> *enemy_position);
    void update(float dt);

    ~bullet();

    int speed;
    int base_size;
    int base_speed;

    std::pair<float, float> velocity;

    float velocity_boost;
    float velocity_decay_rate;
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

    float maxTurnRate = (baseTurnDeg + homingBoostDeg) * (3.14159265f / 180.0f);

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

void bullet::update(float dt)
{
}