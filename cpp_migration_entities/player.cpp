#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include "SFML/include/SFML/Graphics.hpp"
#include "SFML/include/SFML/Window.hpp"
#include "SFML/include/SFML/System.hpp"
#include "generic_object.cpp"

class player : public generic_object
{
private:
public:
    player::player(int x, int y, int w = 20, int h = 20, int base_speed = 200) : generic_object(x, y, w, h, "assets/player/")
    {
        this->base_speed = base_speed;
    }

    ~player();

    int base_speed = 200;
    int current_speed = base_speed;

    sf::Sprite sprite;

    void update(float dt, int view_w, int view_h);
    void draw(sf::RenderWindow &window);

    std::pair<int, int> get_collision();

    // Player class implementation
};

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

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
    {
        current_speed = base_speed * 0.5;
    }
    else
    {
        current_speed = base_speed;
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