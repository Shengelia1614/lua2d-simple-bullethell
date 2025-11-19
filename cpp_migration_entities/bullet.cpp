
#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include "SFML/include/SFML/Graphics.hpp"
#include "SFML/include/SFML/Window.hpp"
#include "SFML/include/SFML/System.hpp"
#include "generic_object.cpp"

class bullet : public generic_object
{
private:
    float velocity_life_time = 0;
    float hue;
    float saturation;
    float value;
    float alpha;

public:
    std::pair<int, int> bullets;

    bullet(int x, int y, int targetX, int targetY, int midi, int key_velocity, int colorscheme, int base_size = 10, int base_speed = 120, float velocity_decay_rate = 4) : generic_object(x, y, base_size, base_size, "assets/bullet/")
    {
        this->speed = speed;
        this->base_size = base_size;

        float dx = targetX - x;
        float dy = targetY - y;
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
        velocity.first = (targetX - x);
        velocity.second = (targetY - y);
    }

    ~bullet();

    int speed;
    int base_size;
    int base_speed;

    std::pair<float, float> velocity;

    float velocity_boost;
    float velocity_decay_rate;
};