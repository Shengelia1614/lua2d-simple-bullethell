#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include "generic_object.cpp"

class player : public generic_object
{
private:
    float fx; // precise x position for dt-accurate movement
    float fy; // precise y position for dt-accurate movement

public:
    player(int x, int y, int w = 20, int h = 20, int base_speed_param = 200) : generic_object(x, y, w, h, "sprites/player/")
    {
        this->base_speed = static_cast<float>(base_speed_param);
        this->current_speed = this->base_speed;
        this->fx = static_cast<float>(this->position.first);
        this->fy = static_cast<float>(this->position.second);
    }

    ~player() = default;

    float base_speed;
    float current_speed;

    void update(float dt, int view_w, int view_h);
    void draw(sf::RenderWindow &window);

    std::pair<int, int> get_collision();

    // Player class implementation
};

void player::update(float dt, int view_w, int view_h)
{
    // animation: only if we have frames
    if (!animated_sprite.empty())
    {
        animation_timer += dt;
        if (animation_timer >= animation_speed)
        {
            animation_timer = 0;
            current_frame = (current_frame + 1) % static_cast<int>(animated_sprite.size());
            sprite = animated_sprite[current_frame];
        }
    }

    float dx = 0.f;
    float dy = 0.f;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        dy -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        dy += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        dx -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        dx += 1.f;

    if (dx != 0.f || dy != 0.f)
    {
        float magnitude = std::sqrt(dx * dx + dy * dy);
        if (magnitude != 0.f)
        {
            dx /= magnitude;
            dy /= magnitude;
        }
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
        current_speed = base_speed * 0.5f;
    else
        current_speed = base_speed;

    // use float positions for dt-accurate movement
    fx += dx * current_speed * dt;
    fy += dy * current_speed * dt;

    // clamp in float space
    float min_x = 0.f;
    float min_y = 0.f;
    float max_x = static_cast<float>(view_w - width);
    float max_y = static_cast<float>(view_h - height);

    if (fx < min_x)
        fx = min_x;
    if (fy < min_y)
        fy = min_y;
    if (fx > max_x)
        fx = max_x;
    if (fy > max_y)
        fy = max_y;

    // keep integer base class position in sync (for collision, legacy code)
    this->position.first = static_cast<int>(std::round(fx));
    this->position.second = static_cast<int>(std::round(fy));
}

void player::draw(sf::RenderWindow &window)
{
    // draw using precise float position for smooth movement
    sprite.setPosition(sf::Vector2f(fx, fy));

    sprite.setScale(sf::Vector2f(static_cast<float>(this->width) / sprite.getTexture().getSize().x, static_cast<float>(this->height) / sprite.getTexture().getSize().y));

    window.draw(sprite);
}

std::pair<int, int> player::get_collision()
{
    return std::pair<int, int>(static_cast<int>(std::round(this->fx + this->width / 2.0f)), static_cast<int>(std::round(this->fy + this->height / 2.0f)));
}