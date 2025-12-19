
#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <regex>
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include <cmath>
#include "../cpp_migration_entities/player.h"
#include "../cpp_migration_entities/enemy.h"
#include "../cpp_migration_entities/bullet.h"

int PIANO_KEYS = 88;                   // Standard piano (MIDI 21-108)
int PIANO_WIDTH = VIRTUAL_WIDTH * 0.3; // 30% of screen width
int WHITE_KEY_HEIGHT = 50;             // Height of white keys
int BLACK_KEY_HEIGHT = 32;             // Height of black keys (shorter)
int PIANO_Y = 80;                      // Y position (top of screen)
int PIANO_ARCH_DEPTH = 80;             // How deep the arch curves downward

// Piano key pattern: true = white key, false = black key
// Pattern repeats every 12 semitones: C, C#, D, D#, E, F, F#, G, G#, A, A#, B
bool KEY_PATTERN[] = {true, false, true, false, true, true, false, true, false, true, false, true};

struct MusicEvent
{
    float time;
    int midi;
    float velocity;
    float duration;
    bool note_on;

    MusicEvent() : time(0), midi(0), velocity(0), duration(0), note_on(true) {}
};

bool decodeJSON(const std::string &str, std::vector<MusicEvent> &outEvents)
{
    // Remove whitespace to simplify pattern searches (keeps parser small and dependency-free)
    std::string cleanStr = std::regex_replace(str, std::regex("\\s+"), "");

    // Find the events array
    size_t eventsStart = cleanStr.find("\"events\":[");
    if (eventsStart == std::string::npos)
    {
        std::cerr << "Could not find events array" << std::endl;
        return false;
    }

    std::vector<MusicEvent> events;
    size_t currentPos = eventsStart + 10; // skip past "events":[

    while (true)
    {
        // Find next event object
        size_t objStart = cleanStr.find("{", currentPos);
        if (objStart == std::string::npos)
        {
            break;
        }

        size_t objEnd = cleanStr.find("}", objStart);
        if (objEnd == std::string::npos)
        {
            break;
        }

        std::string objStr = cleanStr.substr(objStart, objEnd - objStart + 1);

        // Extract the fields: time (float), midi_number (int), and duration if present.
        // Keep backwards compatibility with older key 'midi'.
        std::smatch match;

        // Extract time
        std::regex timeRegex("\"time\":([\\d\\.\\-]+)");
        float time = 0.0f;
        if (std::regex_search(objStr, match, timeRegex))
        {
            time = std::stof(match[1].str());
        }
        else
        {
            currentPos = objEnd + 1;
            continue; // Skip if no time found
        }

        // Extract midi_number (try new key first, then old key for backwards compatibility)
        std::regex midiRegex("\"midi_number\":(\\d+)");
        std::regex midiOldRegex("\"midi\":(\\d+)");
        int midi_num = 0;
        if (std::regex_search(objStr, match, midiRegex))
        {
            midi_num = std::stoi(match[1].str());
        }
        else if (std::regex_search(objStr, match, midiOldRegex))
        {
            midi_num = std::stoi(match[1].str());
        }
        else
        {
            currentPos = objEnd + 1;
            continue; // Skip if no midi found
        }

        // Extract optional fields
        MusicEvent ev;
        ev.time = time;
        ev.midi = midi_num;

        // Extract velocity
        std::regex velocityRegex("\"velocity\":([\\d\\.\\-]+)");
        if (std::regex_search(objStr, match, velocityRegex))
        {
            ev.velocity = std::stof(match[1].str());
        }

        // Extract duration
        std::regex durationRegex("\"duration\":([\\d\\.\\-]+)");
        if (std::regex_search(objStr, match, durationRegex))
        {
            ev.duration = std::stof(match[1].str());
        }

        // Extract note_on (if missing, assume true for backwards compatibility)
        std::regex noteOnRegex("\"note_on\":(true|false)");
        if (std::regex_search(objStr, match, noteOnRegex))
        {
            std::string noteOnStr = match[1].str();
            ev.note_on = (noteOnStr == "true");
        }
        else
        {
            ev.note_on = true; // Default to true
        }

        events.push_back(ev);

        currentPos = objEnd + 1;

        // Check if we've reached the end of the array
        if (currentPos < cleanStr.length() && cleanStr[currentPos] == ']')
        {
            break;
        }
    }

    // Sort events by time to ensure chronological order
    std::sort(events.begin(), events.end(), [](const MusicEvent &a, const MusicEvent &b)
              { return a.time < b.time; });

    outEvents = events;
    return true;
}

class BossStageSate
{
    std::string track;
    std::string jsonPath;
    std::vector<MusicEvent> events;

    int colorscheme;
    Player player;
    Enemy enemy;
    float bossInitialX;
    float bossInitialY;
    std::vector<bullet *> bullets;
    sf::SoundBuffer soundBuffer[88];
    sf::Sound *soundPlayers[88];
    float gameTime;
    size_t currentEventIndex;
    float startDelay;
    float lastBulletTime;
    int bulletClusterCount;

    struct PianoKey
    {
        bool active;
        float fadeTimer;
        float fadeDuration;
    };
    std::vector<PianoKey> pianoKeys;

public:
    BossStageSate() : player(0, 0), enemy(0, 0), gameTime(0), currentEventIndex(0), startDelay(0), lastBulletTime(0), bulletClusterCount(0) {

                      };
    void set(std::string track);
    void update(float dt, const sf::RenderWindow &window);
    void draw(sf::RenderWindow &window);
};

void BossStageSate::set(std::string track)
{
    this->track = track;

    // Remove extension to get base name
    std::string baseName = track;
    size_t lastDot = track.find_last_of('.');
    if (lastDot != std::string::npos)
    {
        baseName = track.substr(0, lastDot);
    }

    // Build note JSON path using the naming scheme: <base>_notes.json in notes/note_data
    this->jsonPath = "notes/note_data/" + baseName + "_notes.json";

    // Load and parse JSON file
    std::ifstream file(jsonPath);

    if (!file)
    {
        std::cerr << "Failed to open JSON file: " << jsonPath << std::endl;
        return;
    }

    for (int i = 0; i < 88; ++i)
    {
        std::string soundFile = "notes/keys/" + std::to_string(i + 1) + ".mp3";
        if (!soundBuffer[i].loadFromFile(soundFile))
        {
            std::cerr << "Failed to load sound file: " << soundFile << std::endl;
        }
        soundPlayers[i] = new sf::Sound(soundBuffer[i]);
    }

    std::string jsonContent((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

    if (!decodeJSON(jsonContent, events))
    {
        std::cerr << "Failed to decode JSON content from: " << jsonPath << std::endl;
        return;
    }

    std::cout << "Loaded " << events.size() << " music events from " << jsonPath << std::endl;

    // Choose color scheme for this bossfight: random hue (0-360 degrees)
    // Examples: 0=red, 60=yellow, 120=green, 180=cyan, 240=blue, 300=magenta
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 360);
    this->colorscheme = dis(gen);
    std::cout << "Color scheme hue: " << this->colorscheme << "°" << std::endl;

    // Initialize player
    this->player = Player(VIRTUAL_WIDTH / 2.0f - 10.0f, VIRTUAL_HEIGHT / 2.0f - 10.0f);
    this->enemy = Enemy(VIRTUAL_WIDTH / 2.0f - 30.0f, 10.0f, 60, 40); // Example enemy initialization

    // Initialize boss enemy at top middle of screen with slow movement
    float bossX = VIRTUAL_WIDTH / 2.0f - 30.0f; // center horizontally (60 is enemy width)
    float bossY = 10.0f;                        // top of screen with some padding
    // this->boss = Enemy(bossX, bossY, 60, 40); // TODO: Implement Enemy class

    // Store initial boss position for piano spread mode
    this->bossInitialX = bossX + 30.0f; // center of boss (width/2)
    this->bossInitialY = bossY + 20.0f; // center of boss (height/2)

    // this->barrier = Barrier(this->player); // TODO: Implement Barrier class
    this->bullets.clear();
    this->gameTime = 0.0f;
    this->currentEventIndex = 0; // C++ uses 0-based indexing

    // Add a start delay before bullets begin spawning (in seconds)
    this->startDelay = 4.0f; // 4 second grace period at the start

    // Track recent bullet spawn times for shotgun spread effect
    this->lastBulletTime = -999.0f;
    this->bulletClusterCount = 0;

    // Piano visualization: track active keys with fade-out effect
    this->pianoKeys.clear();
    this->pianoKeys.resize(PIANO_KEYS);
    for (int i = 0; i < PIANO_KEYS; i++)
    {
        pianoKeys[i].active = false;
        pianoKeys[i].fadeTimer = 0.0f;
        pianoKeys[i].fadeDuration = 0.3f; // How long the key stays lit after being pressed
    }
}

void BossStageSate::update(float dt, const sf::RenderWindow &window)
{
    // Update game time
    gameTime += dt;

    // Update player
    player.update(dt, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);

    // Update boss only in player-targeting mode (not implemented yet)
    // TODO: Add BOSS_MODE_PIANO_SPREAD flag and boss update logic

    // Update barrier (not implemented yet)
    // TODO: Add barrier update and spacebar check

    // Spawn bullets based on note events
    // Each event has a "time" field that determines when (in seconds)
    // the bullet should spawn after the bossfight starts
    // Account for start delay: bullets only spawn after the grace period
    while (currentEventIndex < events.size())
    {
        MusicEvent &event = events[currentEventIndex];

        // Check if enough time has passed to spawn this bullet (accounting for start delay)
        if (gameTime >= (event.time + startDelay))
        {
            // Only create bullets for note_on events
            if (event.note_on)
            {
                // Spawn bullet from boss towards player
                float bossX = bossInitialX;
                float bossY = bossInitialY;
                auto playerCollision = player.get_collision();

                float dx, dy, targetX, targetY;

                int noteIndex = event.midi - 20;                                            // Convert MIDI (21-108) to index (1-88)
                float normalizedPos = (noteIndex - 1) / static_cast<float>(PIANO_KEYS - 1); // 0 to 1

                // Map to angle: -75° to +75° (150° spread)
                // Center is straight down (90° in standard coords, or PI/2 radians)
                // REVERSED: lower notes (left) should shoot left, higher notes (right) should shoot right
                float spreadDegrees = 180;
                float minAngle = 90 - (spreadDegrees / 2); // 15°
                float maxAngle = 90 + (spreadDegrees / 2); // 165°

                // Reverse the mapping: 0 -> maxAngle (165°, left), 1 -> minAngle (15°, right)
                float angleDegrees = maxAngle - normalizedPos * spreadDegrees;
                float angleRadians = angleDegrees * (PI / 180.0f);
                // Convert angle to direction vector
                dx = std::cos(angleRadians);
                dy = std::sin(angleRadians);

                // Calculate target position
                targetX = bossX + dx * 1000;
                targetY = bossY + dy * 1000;
                std::pair<float, float> target = std::make_pair(targetX, targetY);

                // Create bullet with MIDI-based properties
                int keyVelocity = static_cast<int>(std::clamp(event.velocity * 127.0f, 1.0f, 127.0f));
                bullet *newBullet = new bullet(
                    bossInitialX,
                    bossInitialY,
                    target,
                    event.midi,
                    keyVelocity,
                    colorscheme,
                    3,
                    20.0f, // base size
                    100.0f // base speed
                );

                bullets.push_back(newBullet);

                // Play audio note
                soundPlayers[event.midi - 21]->setVolume(66 + (event.velocity / 381));
                soundPlayers[event.midi - 21]->stop();
                soundPlayers[event.midi - 21]->play();
            }

            // Activate piano key visualization for ALL events (both note_on and note_off)
            // MIDI notes 21-108 map to piano keys 0-87 (C++ uses 0-based indexing)
            int keyIndex = event.midi - 21; // Convert MIDI (21-108) to key index (0-87)
            if (keyIndex >= 0 && keyIndex < PIANO_KEYS)
            {
                pianoKeys[keyIndex].active = true;
                pianoKeys[keyIndex].fadeTimer = pianoKeys[keyIndex].fadeDuration;
            }

            currentEventIndex++;
        }
        else
        {
            // Haven't reached the time for this event yet, stop checking
            break;
        }
        std::cout << bullets.size() << std::endl;
    }

    // Update piano key fade timers
    for (int i = 0; i < PIANO_KEYS; i++)
    {
        if (pianoKeys[i].fadeTimer > 0)
        {
            pianoKeys[i].fadeTimer = pianoKeys[i].fadeTimer - dt;
            if (pianoKeys[i].fadeTimer <= 0)
            {
                pianoKeys[i].active = false;
                pianoKeys[i].fadeTimer = 0;
            }
        }
    }

    // Update bullets
    for (auto it = bullets.begin(); it != bullets.end();)
    {
        bullet *b = *it;

        // Update bullet
        // auto playerPos = player.get_collision();
        b->update(dt, enemy.position, player.get_collision());

        // TODO: Check barrier collision for bullets
        // if (self.barrier:isActive()) then ... end

        // Remove inactive bullets
        if (!b->active)
        {
            delete b;
            it = bullets.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // TODO: Check collisions with player
    // if (Collision.checkAABB(...)) then return 'gameover' end
}
void BossStageSate::draw(sf::RenderWindow &window)
{
    // Draw player
    player.draw(window);

    // Draw bullets
    for (auto &b : bullets)
    {
        b->draw(window);
    }

    // Draw piano visualization
    float whiteKeyWidth = static_cast<float>(PIANO_WIDTH) / 52.0f; // 52 white keys on a standard piano
    float blackKeyWidth = whiteKeyWidth * 0.6f;
    float pianoX = (VIRTUAL_WIDTH - PIANO_WIDTH) / 2.0f;

    // First pass: Draw all white keys
    for (int i = 0, whiteKeyIndex = 0; i < PIANO_KEYS; i++)
    {
        bool isWhite = KEY_PATTERN[i % 12];

        if (isWhite)
        {
            float keyX = pianoX + whiteKeyIndex * whiteKeyWidth;
            float keyY = PIANO_Y;
            float keyWidth = whiteKeyWidth;
            float keyHeight = WHITE_KEY_HEIGHT;

            sf::RectangleShape keyShape(sf::Vector2f(keyWidth - 1, keyHeight)); // -1 for spacing
            keyShape.setPosition(sf::Vector2f(keyX, keyY));

            if (pianoKeys[i].active)
            {
                float intensity = pianoKeys[i].fadeTimer / pianoKeys[i].fadeDuration;
                std::uint8_t alpha = static_cast<std::uint8_t>(255 * intensity);
                keyShape.setFillColor(sf::Color(255, 255, 0, alpha)); // Yellow for active white keys
            }
            else
            {
                keyShape.setFillColor(sf::Color::White);
            }

            window.draw(keyShape);
            whiteKeyIndex++;
        }
        else if (isWhite)
        {
            whiteKeyIndex++;
        }
    }

    // Second pass: Draw all black keys on top
    for (int i = 0, whiteKeyIndex = 0; i < PIANO_KEYS; i++)
    {
        bool isWhite = KEY_PATTERN[i % 12];

        if (isWhite)
        {
            whiteKeyIndex++;
        }
        else
        {
            float keyX = pianoX + (whiteKeyIndex - 1) * whiteKeyWidth + whiteKeyWidth - (blackKeyWidth / 2.0f);
            float keyY = PIANO_Y;
            float keyWidth = blackKeyWidth;
            float keyHeight = BLACK_KEY_HEIGHT;

            sf::RectangleShape keyShape(sf::Vector2f(keyWidth - 1, keyHeight)); // -1 for spacing
            keyShape.setPosition(sf::Vector2f(keyX, keyY));

            if (pianoKeys[i].active)
            {
                float intensity = pianoKeys[i].fadeTimer / pianoKeys[i].fadeDuration;
                std::uint8_t alpha = static_cast<std::uint8_t>(255 * intensity);
                keyShape.setFillColor(sf::Color(255, 165, 0, alpha)); // Orange for active black keys
            }
            else
            {
                keyShape.setFillColor(sf::Color::Black);
            }

            window.draw(keyShape);
        }
    }
}