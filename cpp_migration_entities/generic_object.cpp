#pragma once
#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

sf::Texture default_error_texture = sf::Texture();

class generic_object
{
protected:
    void load_sprites(std::string sprite_folder);

    int current_frame = 0;
    float animation_speed = 0.1f; // was int
    float animation_timer = 0;
    std::vector<sf::Sprite> animated_sprite = {};
    std::vector<sf::Texture> textures; // keep textures alive for sprites

public:
    generic_object(int x, int y, int w, int h, std::string sprite_folder) // removed extra qualification
    {
        load_sprites(sprite_folder);
        position = std::make_pair(x, y);
        width = w;
        height = h;
    }

    ~generic_object() = default;

    std::pair<int, int> position;
    int width;
    int height;

    sf::Sprite sprite = sf::Sprite(default_error_texture);

    std::pair<int, int> get_collision();

    // Player class implementation
};

std::pair<int, int> generic_object::get_collision()
{
    return std::make_pair(position.first + width / 4, position.second + height / 4);
}

void generic_object::load_sprites(std::string sprite_folder)
{
    std::string base_path = sprite_folder;
    for (const auto &entry : std::filesystem::directory_iterator(base_path))
    {
        if (entry.path().extension() == ".png")
        {
            this->textures.emplace_back();
            if (this->textures.back().loadFromFile(entry.path().string()))
            {
                sf::Sprite sprite(this->textures.back());
                this->animated_sprite.push_back(sprite);
            }
            else
            {
                std::cerr << "Failed to load texture: " << entry.path().string() << "\n";
                this->textures.pop_back();
            }
        }
    }
}