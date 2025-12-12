
#pragma once
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <regex>
#include <iostream>
#include <algorithm>

constexpr int VIRTUAL_WIDTH = 1280;
constexpr int VIRTUAL_HEIGHT = 720;

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
