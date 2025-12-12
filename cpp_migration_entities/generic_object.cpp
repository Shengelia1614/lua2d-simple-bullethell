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
            sf::Texture texture;
            std::string path = entry.path().string();

            if (texture.loadFromFile(path))
            {

                this->textures.push_back(std::move(texture));
            }
            else
            {
                std::cerr << "Failed to load texture: " << path << std::endl;
            }
        }
    }

    if (!this->textures.empty())
    {
        this->sprite.setTexture(this->textures[0]);

        // CRITICAL FIX: In SFML 3, you must set the texture rect for the sprite to have size
        sf::Vector2u texSize = this->textures[0].getSize();
        this->sprite.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(texSize.x, texSize.y)));

        std::cout << "Sprite texture set to first loaded texture" << std::endl;

        // Verify bounds after setting texture rect
        sf::FloatRect spriteBounds = this->sprite.getGlobalBounds();
        std::cout << "Sprite bounds after setting texture rect: w=" << spriteBounds.size.x << " h=" << spriteBounds.size.y << std::endl;
    }
    else
    {
        std::cerr << "WARNING: No textures loaded for " << sprite_folder << std::endl;
    }
}