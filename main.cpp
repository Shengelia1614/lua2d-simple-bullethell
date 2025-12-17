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

// Include implementation files (they need to be compiled together)
// IMPORTANT: Include in dependency order - base classes first!
#include "cpp_migration_entities/generic_object.cpp"
#include "cpp_migration_entities/player.cpp"
#include "cpp_migration_entities/enemy.cpp"
#include "cpp_migration_entities/bullet.cpp"
#include "cpp_migration_states/main_menu.cpp"
#include "cpp_migration_states/boss_stage.cpp"

#define VIRTUAL_WIDTH 1280
#define VIRTUAL_HEIGHT 720

// g++.exe -fdiagnostics-color=always -g main.cpp -o main.exe -std=gnu++20 -lsfml-graphics -lsfml-window -lsfml-system      to run it ON MY PC

// small helper for std::visit
template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

enum class GameState
{
    MAIN_MENU,
    BOSS_STAGE,
    EXIT
};

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

    sf::Clock clock; // add clock to measure dt

    MainMenuState mainMenu;
    BossStageSate bossStage;
    std::vector<std::string> selectedTracks;
    int selectedTrackIndex = -1;
    mainMenu.enter(selectedTracks);

    GameState gameState = GameState::MAIN_MENU;

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

        if (gameState == GameState::MAIN_MENU)
        {
            if (selectedTrackIndex == -1)
            {
                mainMenu.update(dt, window, selectedTrackIndex);
                mainMenu.draw(window);
            }
            else
            {
                bossStage.set(selectedTracks[selectedTrackIndex]);
                gameState = GameState::BOSS_STAGE;
            }
        }
        else if (gameState == GameState::BOSS_STAGE)
        {
            bossStage.update(dt, window);
            bossStage.draw(window);
        }

        // main_player.update(dt, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
        // main_player.draw(window);

        window.display();
    }
}