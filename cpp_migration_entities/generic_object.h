#pragma once
#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#define VIRTUAL_WIDTH 1280
#define VIRTUAL_HEIGHT 720

extern sf::Texture default_error_texture;

class generic_object
{
protected:
    void load_sprites(std::string sprite_folder);

    int current_frame = 0;
    float animation_speed = 0.1f;
    float animation_timer = 0;
    std::vector<sf::Texture> textures;

public:
    generic_object(float x, float y, int w, int h, std::string sprite_folder);
    virtual ~generic_object() = default;

    std::pair<float, float> position;
    int width;
    int height;

    sf::Sprite sprite;

    std::pair<float, float> get_collision();
};
