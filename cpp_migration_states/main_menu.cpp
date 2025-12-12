#pragma once
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

class MainMenuState
{
private:
    static constexpr int VIRTUAL_WIDTH = 1280;
    static constexpr int VIRTUAL_HEIGHT = 720;

    int selectedIndex;
    std::vector<std::string> audioFiles;
    bool startButtonHovered;

    // Button dimensions
    struct Button
    {
        float x, y, width, height;
    } startButton;

    // List area
    float listY;
    float itemHeight;
    float scrollOffset;

    // Font and text
    sf::Font font;
    bool fontLoaded;

    // Mouse tracking
    sf::Vector2f virtualMousePos;

    // Key state tracking for single press detection
    bool upKeyWasPressed;
    bool downKeyWasPressed;
    bool enterKeyWasPressed;
    bool spaceKeyWasPressed;
    bool mouseWasPressed;

public:
    MainMenuState()
        : selectedIndex(0), startButtonHovered(false), listY(200.0f),
          itemHeight(40.0f), scrollOffset(0.0f), fontLoaded(false),
          upKeyWasPressed(false), downKeyWasPressed(false),
          enterKeyWasPressed(false), spaceKeyWasPressed(false),
          mouseWasPressed(false)
    {
    }

    void enter(std::vector<std::int32_t> &selectedTracks)
    {
        selectedIndex = 1;
        audioFiles.clear();
        startButtonHovered = false;
        scrollOffset = 0.0f;
        upKeyWasPressed = false;
        downKeyWasPressed = false;
        enterKeyWasPressed = false;
        spaceKeyWasPressed = false;
        mouseWasPressed = false;

        // Try to load a font (you may need to adjust the path)
        fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");
        if (!fontLoaded)
        {
            // Try alternative paths
            fontLoaded = font.openFromFile("arial.ttf") ||
                         font.openFromFile("fonts/arial.ttf");
        }

        // Scan notes/midi directory for MIDI files
        std::string midiDir = "notes/midi";

        try
        {
            if (fs::exists(midiDir) && fs::is_directory(midiDir))
            {
                for (const auto &entry : fs::directory_iterator(midiDir))
                {
                    if (entry.is_regular_file())
                    {
                        std::string filename = entry.path().filename().string();
                        std::string ext = entry.path().extension().string();

                        // Convert extension to lowercase for comparison
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                        if (ext == ".mid" || ext == ".midi")
                        {
                            audioFiles.push_back(filename);
                        }
                    }
                }
            }
        }
        catch (const fs::filesystem_error &e)
        {
            // Handle filesystem errors silently
        }

        // Sort files alphabetically
        std::sort(audioFiles.begin(), audioFiles.end());

        // If no MIDI files found, add placeholder
        if (audioFiles.empty())
        {
            audioFiles.push_back("No MIDI files found");
        }

        // Initialize button
        startButton.x = VIRTUAL_WIDTH / 2.0f - 100.0f;
        startButton.y = VIRTUAL_HEIGHT - 150.0f;
        startButton.width = 200.0f;
        startButton.height = 60.0f;
    }

    void update(float dt, const sf::RenderWindow &window, int &selectedTrackIndex)
    {
        // Get mouse position and convert to virtual coordinates
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2u windowSize = window.getSize();

        float scaleX = static_cast<float>(windowSize.x) / VIRTUAL_WIDTH;
        float scaleY = static_cast<float>(windowSize.y) / VIRTUAL_HEIGHT;
        float scale = std::min(scaleX, scaleY);

        float offsetX = (windowSize.x - (VIRTUAL_WIDTH * scale)) / 2.0f;
        float offsetY = (windowSize.y - (VIRTUAL_HEIGHT * scale)) / 2.0f;

        virtualMousePos.x = (mousePos.x - offsetX) / scale;
        virtualMousePos.y = (mousePos.y - offsetY) / scale;

        // Check for single key press (only trigger once per press)
        bool upKeyPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
        if (upKeyPressed && !upKeyWasPressed)
        {
            selectedIndex = std::max(1, selectedIndex - 1);

            // Scroll if needed
            float selectedY = listY + ((selectedIndex - 1) * itemHeight) - scrollOffset;
            if (selectedY < listY)
            {
                scrollOffset -= itemHeight;
            }
        }
        upKeyWasPressed = upKeyPressed;

        bool downKeyPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
        if (downKeyPressed && !downKeyWasPressed)
        {
            selectedIndex = std::min(static_cast<int>(audioFiles.size()), selectedIndex + 1);

            // Scroll if needed
            float selectedY = listY + ((selectedIndex - 1) * itemHeight) - scrollOffset;
            if (selectedY + itemHeight > VIRTUAL_HEIGHT - 200)
            {
                scrollOffset += itemHeight;
            }
        }
        downKeyWasPressed = downKeyPressed;

        bool enterPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter);
        bool spacePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
        if ((enterPressed && !enterKeyWasPressed) || (spacePressed && !spaceKeyWasPressed))
        {
            selectedTrackIndex = selectedIndex - 1;
        }
        enterKeyWasPressed = enterPressed;
        spaceKeyWasPressed = spacePressed;

        bool mousePressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        if (mousePressed && !mouseWasPressed)
        {
            // Check if clicked on start button
            if (virtualMousePos.x >= startButton.x &&
                virtualMousePos.x <= startButton.x + startButton.width &&
                virtualMousePos.y >= startButton.y &&
                virtualMousePos.y <= startButton.y + startButton.height)
            {
                selectedTrackIndex = selectedIndex - 1;
            }

            // Check if clicked on a list item
            for (size_t i = 0; i < audioFiles.size(); ++i)
            {
                float itemY = listY + (i * itemHeight) - scrollOffset;

                if (virtualMousePos.x >= VIRTUAL_WIDTH / 2.0f - 300.0f &&
                    virtualMousePos.x <= VIRTUAL_WIDTH / 2.0f + 300.0f &&
                    virtualMousePos.y >= itemY &&
                    virtualMousePos.y <= itemY + itemHeight)
                {
                    selectedIndex = static_cast<int>(i) + 1;
                    break;
                }
            }
        }
        mouseWasPressed = mousePressed;

        // Check if mouse is over start button
        startButtonHovered = virtualMousePos.x >= startButton.x &&
                             virtualMousePos.x <= startButton.x + startButton.width &&
                             virtualMousePos.y >= startButton.y &&
                             virtualMousePos.y <= startButton.y + startButton.height;
    }

    void draw(sf::RenderWindow &window)
    {
        window.clear(sf::Color::Black);

        // Title
        if (fontLoaded)
        {
            sf::Text titleText(font, "Sacred Notes", 48);
            titleText.setFillColor(sf::Color::White);
            centerText(titleText, VIRTUAL_WIDTH / 2.0f, 50.0f);
            window.draw(titleText);

            sf::Text subtitleText(font, "Select Music", 32);
            subtitleText.setFillColor(sf::Color::White);
            centerText(subtitleText, VIRTUAL_WIDTH / 2.0f, 100.0f);
            window.draw(subtitleText);
        }

        // Draw audio file list
        for (size_t i = 0; i < audioFiles.size(); ++i)
        {
            float y = listY + (i * itemHeight) - scrollOffset;

            // Only draw if visible
            if (y >= listY && y < VIRTUAL_HEIGHT - 200)
            {
                // Highlight selected item
                if (static_cast<int>(i) == selectedIndex - 1)
                {
                    sf::RectangleShape highlight(sf::Vector2f(600.0f, itemHeight - 5.0f));
                    highlight.setPosition(sf::Vector2f(VIRTUAL_WIDTH / 2.0f - 300.0f, y));
                    highlight.setFillColor(sf::Color(76, 153, 255)); // 0.3, 0.6, 1.0
                    window.draw(highlight);
                }

                // Draw file name (without extension)
                if (fontLoaded)
                {
                    std::string displayName = audioFiles[i];
                    size_t dotPos = displayName.find_last_of('.');
                    if (dotPos != std::string::npos)
                    {
                        displayName = displayName.substr(0, dotPos);
                    }

                    sf::Text fileText(font, displayName, 20);
                    fileText.setFillColor(sf::Color::White);
                    fileText.setPosition(sf::Vector2f(VIRTUAL_WIDTH / 2.0f - 290.0f, y + 10.0f));
                    window.draw(fileText);
                }
            }
        }

        // Draw start button
        sf::RectangleShape buttonRect(sf::Vector2f(startButton.width, startButton.height));
        buttonRect.setPosition(sf::Vector2f(startButton.x, startButton.y));

        if (startButtonHovered)
        {
            buttonRect.setFillColor(sf::Color(102, 204, 102)); // 0.4, 0.8, 0.4
        }
        else
        {
            buttonRect.setFillColor(sf::Color(51, 153, 51)); // 0.2, 0.6, 0.2
        }
        window.draw(buttonRect);

        // Button outline
        sf::RectangleShape buttonOutline(sf::Vector2f(startButton.width, startButton.height));
        buttonOutline.setPosition(sf::Vector2f(startButton.x, startButton.y));
        buttonOutline.setFillColor(sf::Color::Transparent);
        buttonOutline.setOutlineColor(sf::Color::White);
        buttonOutline.setOutlineThickness(2.0f);
        window.draw(buttonOutline);

        // Button text
        if (fontLoaded)
        {
            sf::Text buttonText(font, "START", 24);
            buttonText.setFillColor(sf::Color::White);
            centerText(buttonText, startButton.x + startButton.width / 2.0f, startButton.y + 20.0f);
            window.draw(buttonText);

            // Instructions
            sf::Text instr1(font, "Use UP/DOWN arrows or click to select music", 18);
            instr1.setFillColor(sf::Color(178, 178, 178)); // 0.7, 0.7, 0.7
            centerText(instr1, VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT - 80.0f);
            window.draw(instr1);

            sf::Text instr2(font, "Click START or press ENTER to begin", 18);
            instr2.setFillColor(sf::Color(178, 178, 178));
            centerText(instr2, VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT - 50.0f);
            window.draw(instr2);
        }
    }

    std::string getSelectedTrack() const
    {
        if (selectedIndex > 0 && selectedIndex <= static_cast<int>(audioFiles.size()))
        {
            return audioFiles[selectedIndex - 1];
        }
        return "";
    }

private:
    void centerText(sf::Text &text, float centerX, float y)
    {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setPosition(sf::Vector2f(centerX - bounds.size.x / 2.0f, y));
    }
};
