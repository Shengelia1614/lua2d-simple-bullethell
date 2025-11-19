
#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include "SFML/include/SFML/Graphics.hpp"
#include "SFML/include/SFML/Window.hpp"
#include "SFML/include/SFML/System.hpp"

class generic_object
{
protected:
    void load_sprites(std::string sprite_folder);

    int current_frame = 0;
    int animation_speed = 0.1;
    float animation_timer = 0;
    std::vector<sf::Sprite> animated_sprite;

public:
    generic_object::generic_object(int x, int y, int w, int h, std::string sprite_folder)
    {
        load_sprites(sprite_folder);
        position = std::make_pair(x, y);
        width = w;
        height = h;
    }

    ~generic_object();

    std::pair<int, int> position;
    int width;
    int height;

    sf::Sprite sprite;

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
            if (texture.loadFromFile(entry.path().string()))
            {
                sf::Sprite sprite;
                sprite.setTexture(texture);
                this->animated_sprite.push_back(sprite);
            }
            else
            {
                std::cerr << "Failed to load texture: " << entry.path().string() << "\n";
            }
        }
    }
}