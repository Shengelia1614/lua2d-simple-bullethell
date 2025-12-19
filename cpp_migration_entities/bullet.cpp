#include "bullet.h"

void bullet::homing(float dt, std::pair<int, int> *enemy_position, std::pair<int, int> *player_position)
{

    std::pair<float, float> toPlayer = {player_position->first - this->position.first, player_position->second - this->position.second};
    std::pair<float, float> toEnemy = {enemy_position->first - player_position->first, enemy_position->second - player_position->second};

    float pte_distance = sqrt(toEnemy.first * toEnemy.first + toEnemy.second * toEnemy.second);

    float maxX = VIRTUAL_WIDTH - enemy_position->first;
    float maxY = VIRTUAL_HEIGHT - enemy_position->second;

    float largest_distance = sqrt((maxX * maxX) + (maxY * maxY));

    float distanceRatio = pte_distance / largest_distance;

    float proximity = 1 - std::min(1.0f, std::max(0.0f, distanceRatio));

    float baseTurnDeg = 5;
    float maxExtraDeg = 175;
    float exponent = 6;

    float homingBoostDeg = maxExtraDeg * pow(proximity, exponent);

    float maxTurnRate = (baseTurnDeg + homingBoostDeg) * (PI / 180.0f);

    float angleCurr = atan2(this->velocity.second, this->velocity.first);
    float angleTarget = atan2(toPlayer.second, toPlayer.first);

    float delta = angleTarget - angleCurr;
    while (delta > PI)
    {
        delta = delta - 2 * PI;
    }
    while (delta < -PI)
    {
        delta = delta + 2 * PI;
    }

    float maxTurn = maxTurnRate * (dt ? dt : 1);

    if (delta > maxTurn)
    {
        delta = maxTurn;
    }
    else if (delta < -maxTurn)
    {
        delta = -maxTurn;
    }

    float newAngle = angleCurr + delta;
    float speed = sqrt(this->velocity.first * this->velocity.first + this->velocity.second * this->velocity.second);
    if (speed > 0)
    {
        this->velocity.first = cos(newAngle) * speed;
        this->velocity.second = sin(newAngle) * speed;
    }
}

void bullet::update(float dt, std::pair<int, int> enemy_position, std::pair<int, int> player_position)
{
    if (!this->active)
    {
        return;
    };

    this->animationTimer = this->animationTimer + dt;
    if (this->animationTimer >= this->animationSpeed)
    {
        this->animationTimer = this->animationTimer - this->animationSpeed;
        this->animationIndex = this->animationIndex + 1;
        if (this->animationIndex > FRAME_COUNT)
        {
            this->animationIndex = 1;
        }
    }

    if (this->velocity_boost > 0)
    {
        this->velocity_life_time = this->velocity_life_time + dt;

        float initialBoost = (this->base_speed);
        this->velocity_boost = initialBoost * exp(-this->velocity_decay_rate * this->velocity_life_time);

        if (this->velocity_boost < 0.5)
        {
            this->velocity_boost = 0;
        }

        float currentMag = sqrt(this->velocity.first * this->velocity.first + this->velocity.second * this->velocity.second);
        if (currentMag > 0)
        {
            float dirX = this->velocity.first / currentMag;
            float dirY = this->velocity.second / currentMag;
            this->velocity.first = dirX * (this->base_speed + this->velocity_boost);
            this->velocity.second = dirY * (this->base_speed + this->velocity_boost);
        }
    }

    if (this->bounce_count == 0)
    {
        this->homing(dt, &enemy_position, &player_position);
    }

    this->position.first = this->position.first + this->velocity.first * dt;
    this->position.second = this->position.second + this->velocity.second * dt;
    bool bounced = false;

    if (this->position.first < 0)
    {
        this->position.first = 0;
        this->velocity.first = std::abs(this->velocity.first);
        bounced = true;
    }
    else if (this->position.first + this->width > VIRTUAL_WIDTH)
    {
        this->position.first = VIRTUAL_WIDTH - this->width;
        this->velocity.first = -std::abs(this->velocity.first);
        bounced = true;
    }

    if (this->position.second < 0)
    {
        this->position.second = 0;
        this->velocity.second = std::abs(this->velocity.second);
        bounced = true;
    }
    else if (this->position.second + this->height > VIRTUAL_HEIGHT)
    {
        this->position.second = VIRTUAL_HEIGHT - this->height;
        this->velocity.second = -std::abs(this->velocity.second);
        bounced = true;
    }

    if (bounced)
    {
        this->bounce_count = this->bounce_count + 1;
        if (this->bounce_count > this->max_bounces)
        {
            this->active = false;
        }
        // float mag = sqrt(this->velocity.first * this->velocity.first + this->velocity.second * this->velocity.second);
        // if (mag > 0) {
        //     float normVx = this->velocity.first / mag;
        //     float normVy = this->velocity.second / mag;
        //     this->perpX = -normVy;
        //     this->perpY = normVx;
        // }
    }
}

void bullet::homeless_update(float dt)
{
}
void bullet::draw(sf::RenderWindow &window)
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
    sprite.setPosition(sf::Vector2f(position.first, position.second));

    // Get texture size and scale sprite to match width/height
    const sf::Texture &currentTexture = sprite.getTexture();
    sf::Vector2u texSize = currentTexture.getSize();

    if (firstDraw == false) // only print once after firstDraw check
    {
        static int debugCount = 0;
        if (debugCount == 0)
        {

            // Try to draw a simple test rectangle to verify rendering works
            sf::RectangleShape testRect(sf::Vector2f(width, height));
            testRect.setPosition(sf::Vector2f(position.first, position.second));
            testRect.setFillColor(sf::Color::Red);
            window.draw(testRect);

            debugCount++;
        }
    }

    if (texSize.x > 0 && texSize.y > 0)
    {
        float scaleX = static_cast<float>(this->width) / static_cast<float>(texSize.x);
        float scaleY = static_cast<float>(this->height) / static_cast<float>(texSize.y);
        sprite.setScale(sf::Vector2f(scaleX, scaleY));
        }

    sprite.setColor(sf::Color::White);

    window.draw(sprite);
}