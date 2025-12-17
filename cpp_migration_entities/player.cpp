#include "player.h"

Player::Player(float x, float y, int w, int h, int base_speed_param)
    : generic_object(x, y, w, h, "sprites/player/")
{
    this->base_speed = static_cast<float>(base_speed_param);
    this->current_speed = this->base_speed;
}

void Player::update(float dt, int view_w, int view_h)
{
    // animation: only if we have frames
    if (!textures.empty())
    {
        animation_timer += dt;
        if (animation_timer >= animation_speed)
        {
            animation_timer = 0;
            current_frame = (current_frame + 1) % static_cast<int>(textures.size());
            sprite.setTexture(textures[current_frame]);

            // Must also set texture rect in SFML 3
            sf::Vector2u texSize = textures[current_frame].getSize();
            sprite.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(texSize.x, texSize.y)));
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
    this->position.first += dx * current_speed * dt;
    this->position.second += dy * current_speed * dt;

    // clamp in float space
    float min_x = 0.f;
    float min_y = 0.f;
    float max_x = static_cast<float>(view_w - width);
    float max_y = static_cast<float>(view_h - height);

    if (this->position.first < min_x)
        this->position.first = min_x;
    if (this->position.second < min_y)
        this->position.second = min_y;
    if (this->position.first > max_x)
        this->position.first = max_x;
    if (this->position.second > max_y)
        this->position.second = max_y;
}

void Player::draw(sf::RenderWindow &window)
{
    static bool firstDraw = true;
    if (firstDraw)
    {

        // Check sprite color
        sf::Color spriteColor = sprite.getColor();

        // Check sprite bounds
        sf::FloatRect bounds = sprite.getGlobalBounds();

        firstDraw = false;
    }

    if (textures.empty())
    {
        return;
    }

    // draw using precise float position for smooth movement
    sprite.setPosition(sf::Vector2f(this->position.first, this->position.second));

    // Get texture size and scale sprite to match width/height
    const sf::Texture &currentTexture = sprite.getTexture();
    sf::Vector2u texSize = currentTexture.getSize();

    if (texSize.x > 0 && texSize.y > 0)
    {
        float scaleX = static_cast<float>(this->width) / static_cast<float>(texSize.x);
        float scaleY = static_cast<float>(this->height) / static_cast<float>(texSize.y);
        sprite.setScale(sf::Vector2f(scaleX, scaleY));
    }

    // Make sure sprite has proper color (not transparent)
    sprite.setColor(sf::Color::White);

    window.draw(sprite);
}

std::pair<float, float> Player::get_collision()
{
    return std::pair<float, float>(this->position.first + this->width / 2.0f, this->position.second + this->height / 2.0f);
}