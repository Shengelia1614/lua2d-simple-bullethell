#pragma once
#include "generic_object.h"
#include <SFML/Graphics.hpp>

class enemy : public generic_object
{
private:
public:
    enemy(float x, float y, int w = 20, int h = 20, std::string sprite_folder = "sprites/enemy/");
    ~enemy();
};