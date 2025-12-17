#pragma once
#include "generic_object.h"
#include <SFML/Graphics.hpp>

#define VIRTUAL_WIDTH 1280
#define VIRTUAL_HEIGHT 720

class Player : public generic_object
{
private:
public:
    Player(float x, float y, int w = 20, int h = 20, int base_speed_param = 300);
    ~Player() = default;

    float base_speed;
    float current_speed;

    void update(float dt, int view_w, int view_h);
    void draw(sf::RenderWindow &window);

    std::pair<float, float> get_collision();
};
