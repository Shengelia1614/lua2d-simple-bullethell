#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include <random>
#include <algorithm>
#include <optional>
#include <variant>
#include <SFML/Graphics.hpp>
#include "cpp_migration_entities/player.cpp"

#define VIRTUAL_WIDTH 1280
#define VIRTUAL_HEIGHT 720

// small helper for std::visit
template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

int main()
{
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(VIRTUAL_WIDTH, VIRTUAL_HEIGHT)),
        "Purgatorium+ Bullet Hell",
        sf::Style::Resize | sf::Style::Close);
    sf::View view(
        sf::FloatRect(
            sf::Vector2f(0.f, 0.f),
            sf::Vector2f(static_cast<float>(VIRTUAL_WIDTH), static_cast<float>(VIRTUAL_HEIGHT))));

    player main_player = player(VIRTUAL_WIDTH / 2 - 10, VIRTUAL_HEIGHT / 2 - 10);

    sf::Clock clock; // add clock to measure dt

    while (window.isOpen())
    {
        // SFML 3: pollEvent() returns std::optional<sf::Event>
        while (const std::optional event = window.pollEvent())
        {
            // Close window: exit
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        // compute actual dt (seconds) since last frame
        float dt = clock.restart().asSeconds();

        // optional: clamp dt to avoid huge steps after a pause/stop
        if (dt > 0.25f)
            dt = 0.25f;

        window.setView(view);
        window.clear(sf::Color::Black);

        main_player.update(dt, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
        main_player.draw(window);

        window.display();
    }
}