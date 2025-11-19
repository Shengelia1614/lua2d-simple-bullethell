#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include "SFML/include/SFML/Graphics.hpp"
#include "SFML/include/SFML/Window.hpp"
#include "SFML/include/SFML/System.hpp"

class player
{
private:
    void load_sprites();

    int current_frame = 0;
    int animation_speed = 0.1;
    float animation_timer = 0;
    std::vector<sf::Sprite> animated_sprite;

public:
    player::player(int x, int y)
    {
        load_sprites();
        position = std::make_pair(x, y);
    }

    std::pair<int, int> position;
    int width;
    int height;
    int base_speed;
    int current_speed;

    sf::Sprite sprite;

    void update(float dt, int view_w, int view_h);
    void draw(sf::RenderWindow &window);

    std::pair<int, int> get_collision();

    // Player class implementation
};

void player::load_sprites()
{
    std::string base_path = "sprites/player/";
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

void player::update(float dt, int view_w, int view_h)
{
    animation_timer += dt;
    if (animation_timer >= animation_speed)
    {
        animation_timer = 0;
        current_frame = (current_frame + 1) % animated_sprite.size();
        sprite = animated_sprite[current_frame];
    }

    int dx = 0;
    int dy = 0;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        dy = dy - 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        dy = dy + 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        dx = dx - 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        dx = dx + 1;

    if (dx != 0 || dy != 0)
    {
        float magnitude = std::sqrt(dx * dx + dy * dy);
        dx = dx / magnitude;
        dy = dy / magnitude;
    }

    position.first += dx * current_speed * dt;
    position.second += dy * current_speed * dt;

    if (position.first < 0)
        position.first = 0;
    if (position.second < 0)
        position.second = 0;
    if (position.first > view_w - width)
        position.first = view_w - width;
    if (position.second > view_h - height)
        position.second = view_h - height;
}
void player::draw(sf::RenderWindow &window)
{

    sprite.setPosition(static_cast<float>(position.first), static_cast<float>(position.second));
    sprite.setScale(static_cast<float>(this->width) / sprite.getTexture()->getSize().x, static_cast<float>(this->height) / sprite.getTexture()->getSize().y);
    window.draw(sprite);
}
std::pair<int, int> player::get_collision()
{
    return std::pair<int, int>(this->position.first + this->width / 2, this->position.second + this->height / 2);
}