
#pragma once
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <regex>
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include "../cpp_migration_entities/player.h"
#include "../cpp_migration_entities/enemy.h"
#include "../cpp_migration_entities/bullet.h"

int PIANO_KEYS = 88;                   // Standard piano (MIDI 21-108)
int PIANO_WIDTH = VIRTUAL_WIDTH * 0.3; // 30% of screen width
int WHITE_KEY_HEIGHT = 50;             // Height of white keys
int BLACK_KEY_HEIGHT = 32;             // Height of black keys (shorter)
int PIANO_Y = 20;                      // Y position (top of screen)
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
    float bossInitialX;
    float bossInitialY;
    std::vector<bullet *> bullets;
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
    BossStageSate() : player(0, 0), gameTime(0), currentEventIndex(0), startDelay(0), lastBulletTime(0), bulletClusterCount(0) {

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

    // Process music events based on game time and start delay
    while (currentEventIndex < events.size() && (events[currentEventIndex].time <= gameTime - startDelay))
    {
        MusicEvent &ev = events[currentEventIndex];

        int midiNum = ev.midi;
        if (midiNum < 21 || midiNum > 108)
        {
            std::cerr << "Skipping out-of-range MIDI note: " << midiNum << std::endl;
            currentEventIndex++;
            continue;
        }

        int pianoIndex = midiNum - 21; // Map MIDI 21-108 to pianoKeys 0-87

        if (ev.note_on)
        {
            // Note ON event: spawn bullets and light piano key
            pianoKeys[pianoIndex].active = true;
            pianoKeys[pianoIndex].fadeTimer = pianoKeys[pianoIndex].fadeDuration;

            // Spawn bullet(s) based on note properties
            float velocity = ev.velocity; // 0.0 to 1.0
            float duration = ev.duration; // in seconds

            // Calculate direction from boss to player
            auto playerPos = player.get_collision();
            float dirX = playerPos.first - bossInitialX;
            float dirY = playerPos.second - bossInitialY;
            float length = sqrt(dirX * dirX + dirY * dirY);
            if (length > 0)
            {
                dirX /= length;
                dirY /= length;
            }
            else
            {
                dirX = 0;
                dirY = 1; // Default downwards
            }

            // Create bullet(s) here based on velocity and duration
            int keyVelocity = static_cast<int>(std::clamp(velocity * 127.0f, 1.0f, 127.0f));
            int midiClamped = std::clamp(midiNum - 21, 0, 88);
            float scaleFactor = 3 - ((midiClamped - 1) / (88 - 1) * 2); // Scale factor between 1.0 and 3.0
            int baseSize = static_cast<int>(10 * scaleFactor);
            int baseSpeed = static_cast<int>(120 * (4 - scaleFactor));
            bullet *newBullet = new bullet((bossInitialX), (bossInitialY), &player.position, midiNum, keyVelocity, colorscheme, 3, (baseSize), (baseSpeed));
            bullets.push_back(newBullet);
        }
        else
        {
            // Note OFF event: could implement if needed
        }
        currentEventIndex++;
    }
    // Update player
    player.update(dt, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    // Update bullets
    for (auto it = bullets.begin(); it != bullets.end();)
    {
        bullet *b = *it;
        if (b->active)
        {
            auto playerCollision = player.get_collision();
            b->update(dt, std::make_pair(static_cast<int>(playerCollision.first), static_cast<int>(playerCollision.second)));
            ++it;
        }
        else
        {
            delete b;
            it = bullets.erase(it);
        }
    }
    // Update piano keys fade timers
    for (auto &key : pianoKeys)
    {
        if (key.active)
        {
            key.fadeTimer -= dt;
            if (key.fadeTimer <= 0.0f)
            {
                key.active = false;
                key.fadeTimer = 0.0f;
            }
        }
    }
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
    float whiteKeyWidth = static_cast<float>(PIANO_WIDTH) / std::count(std::begin(KEY_PATTERN), std::end(KEY_PATTERN), true);
    float blackKeyWidth = whiteKeyWidth * 0.6f;
    float pianoX = (VIRTUAL_WIDTH - PIANO_WIDTH) / 2.0f;

    for (int i = 0, whiteKeyIndex = 0; i < PIANO_KEYS; i++)
    {
        bool isWhite = KEY_PATTERN[i % 12];
        float keyX;
        float keyY = PIANO_Y;
        float keyWidth;
        float keyHeight;

        if (isWhite)
        {
            keyX = pianoX + whiteKeyIndex * whiteKeyWidth;
            keyWidth = whiteKeyWidth;
            keyHeight = WHITE_KEY_HEIGHT;
            whiteKeyIndex++;
        }
        else
        {
            keyX = pianoX + (whiteKeyIndex - 1) * whiteKeyWidth + whiteKeyWidth - (blackKeyWidth / 2.0f);
            keyWidth = blackKeyWidth;
            keyHeight = BLACK_KEY_HEIGHT;
        }

        sf::RectangleShape keyShape(sf::Vector2f(keyWidth - 1, keyHeight)); // -1 for spacing
        keyShape.setPosition(sf::Vector2f(keyX, keyY));

        if (pianoKeys[i].active)
        {
            float intensity = pianoKeys[i].fadeTimer / pianoKeys[i].fadeDuration;
            std::uint8_t alpha = static_cast<std::uint8_t>(255 * intensity);

            if (isWhite)
                keyShape.setFillColor(sf::Color(255, 255, 0, alpha)); // Yellow for active white keys
            else
                keyShape.setFillColor(sf::Color(255, 165, 0, alpha)); // Orange for active black keys
        }
        else
        {
            if (isWhite)
                keyShape.setFillColor(sf::Color::White);
            else
                keyShape.setFillColor(sf::Color::Black);
        }

        window.draw(keyShape);
    }
}