

#include "generic_object.h"

#define VIRTUAL_WIDTH 1280
#define VIRTUAL_HEIGHT 720

sf::Texture default_error_texture = sf::Texture();

generic_object::generic_object(float x, float y, int w, int h, std::string sprite_folder)
    : sprite(default_error_texture)
{
    load_sprites(sprite_folder);
    position = std::make_pair(static_cast<float>(x), static_cast<float>(y));
    width = w;
    height = h;
}

std::pair<float, float> generic_object::get_collision()
{
    return std::make_pair(position.first + width / 4.0f,
                          position.second + height / 4.0f);
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