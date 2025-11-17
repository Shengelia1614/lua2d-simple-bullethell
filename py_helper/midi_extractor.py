"""
MIDI to JSON converter
Extracts note events from MIDI files and saves them as JSON
"""

import mido
import json
import os
from pathlib import Path


def midi_to_json(midi_file_path, output_path=None):
    """
    Convert a MIDI file to JSON format with note events.
    
    Args:
        midi_file_path: Path to the MIDI file
        output_path: Optional output path for JSON file. If None, uses same name as MIDI file
    
    Returns:
        Dictionary containing MIDI data
    """
    # Load the MIDI file
    midi = mido.MidiFile(midi_file_path)
    
    # Initialize data structure
    data = {
        "source_file": str(Path(midi_file_path).absolute()),
        "ticks_per_beat": midi.ticks_per_beat,
        "tempo": 500000,  # Default tempo (120 BPM), will be updated if tempo event found
        "duration": 0.0,
        "num_events": 0,
        "events": []
    }
    
    # Track absolute time in seconds
    current_time = 0.0
    
    # First pass: collect ALL events (tempo + notes) from all tracks with their tick times
    # We need to process tempo changes chronologically before converting ticks to seconds
    all_track_events = []
    
    for track_idx, track in enumerate(midi.tracks):
        track_ticks = 0
        
        for msg in track:
            track_ticks += msg.time
            
            if msg.type == 'set_tempo':
                all_track_events.append({
                    "ticks": track_ticks,
                    "type": "tempo",
                    "tempo": msg.tempo,
                    "track": track_idx
                })
            
            elif msg.type == 'note_on' or msg.type == 'note_off':
                # Treat note_on with velocity 0 as note_off
                is_on = msg.type == 'note_on' and msg.velocity > 0
                
                all_track_events.append({
                    "ticks": track_ticks,
                    "type": "on" if is_on else "off",
                    "note": msg.note,
                    "note_name": midi_note_to_name(msg.note),
                    "velocity": msg.velocity,
                    "channel": msg.channel,
                    "track": track_idx
                })
    
    # Sort ALL events by tick time
    all_track_events.sort(key=lambda x: x["ticks"])
    
    # Second pass: convert ticks to seconds, applying tempo changes chronologically
    tempo = 500000  # Default: 120 BPM
    last_tick = 0
    current_time = 0.0
    raw_events = []
    
    for event in all_track_events:
        # Calculate time delta from last event
        tick_delta = event["ticks"] - last_tick
        if tick_delta > 0:
            delta_seconds = (tick_delta * tempo) / (midi.ticks_per_beat * 1000000)
            current_time += delta_seconds
        
        if event["type"] == "tempo":
            tempo = event["tempo"]
            data["tempo"] = tempo
        
        else:  # note on/off
            raw_events.append({
                "time": round(current_time, 6),
                "type": event["type"],
                "note": event["note"],
                "note_name": event["note_name"],
                "velocity": event["velocity"],
                "channel": event["channel"]
            })
        
        last_tick = event["ticks"]

    
    # Third pass: pair note_on and note_off to create events with duration
    # active_notes maps (note, channel) -> list of start dicts (FIFO queue)
    active_notes = {}

    def key_for(n, ch):
        return f"{n}:{ch}"

    for ev in raw_events:
        k = key_for(ev["note"], ev["channel"])
        if ev["type"] == "on":
            active_notes.setdefault(k, []).append(ev)
        else:
            # note off: match the earliest unmatched note_on (FIFO)
            starts = active_notes.get(k)
            if starts and len(starts) > 0:
                start = starts.pop(0)
                duration = round(ev["time"] - start["time"], 6)
                if duration < 0:
                    duration = 0.0

                event = {
                    "time": start["time"],
                    "midi_number": start["note"],
                    "note_name": start["note_name"],
                    "velocity": start["velocity"],
                    "duration": duration,
                    "channel": start["channel"]
                }
                data["events"].append(event)
            else:
                # unmatched note_off; ignore
                pass

    # Any remaining active notes (no note_off) get a duration until end of file
    for k, starts in active_notes.items():
        for start in starts:
            duration = round(max(0.0, current_time - start["time"]), 6)
            event = {
                "time": start["time"],
                "midi_number": start["note"],
                "note_name": start["note_name"],
                "velocity": start["velocity"],
                "duration": duration,
                "channel": start["channel"]
            }
            data["events"].append(event)
    
    # sort events by time
    data["events"].sort(key=lambda x: x["time"])
    
    data["num_events"] = len(data["events"])
    data["duration"] = round(current_time, 6)
    
    data["bpm"] = round(60000000 / tempo, 2)
    
    # save to json
    if output_path is None:
        output_path = Path(midi_file_path).with_suffix('.json')
    else:
        output_path = Path(output_path)
        # If output_path is a directory, create filename in that directory
        if output_path.is_dir() or (not output_path.suffix and not output_path.exists()):
            midi_filename = Path(midi_file_path).stem
            output_path = output_path / f"{midi_filename}_notes.json"
    
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2)
    
    print(f"✓ Converted MIDI file: {midi_file_path}")
    print(f"  - Duration: {data['duration']:.2f} seconds")
    print(f"  - Tempo: {data['bpm']} BPM")
    print(f"  - Events: {data['num_events']}")
    print(f"  - Output: {output_path}")
    
    return data


def midi_note_to_name(midi_number):
    """
    Convert MIDI note number to note name.
    
    Args:
        midi_number: MIDI note number (0-127)
    
    Returns:
        Note name (e.g., "C4", "A#5")
    """
    notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
    octave = (midi_number // 12) - 1
    note = notes[midi_number % 12]
    return f"{note}{octave}"


def batch_convert(input_dir, output_dir=None):
    """
    Convert all MIDI files in a directory to JSON.
    
    Args:
        input_dir: Directory containing MIDI files
        output_dir: Directory for output JSON files. If None, uses same directory as input
    """
    input_path = Path(input_dir)
    
    if not input_path.exists():
        print(f"Error: Input directory '{input_dir}' does not exist")
        return
    
    # Find all MIDI files
    midi_files = list(input_path.glob('*.mid')) + list(input_path.glob('*.midi'))
    
    if not midi_files:
        print(f"No MIDI files found in '{input_dir}'")
        return
    
    print(f"\nFound {len(midi_files)} MIDI file(s)")
    print("=" * 60)
    
    for midi_file in midi_files:
        try:
            if output_dir:
                output_path = Path(output_dir) / f"{midi_file.stem}_notes.json"
            else:
                output_path = midi_file.with_suffix('.json')
            
            midi_to_json(midi_file, output_path)
            print()
        except Exception as e:
            print(f"✗ Error converting {midi_file}: {e}\n")
    
    print("=" * 60)
    print("Batch conversion complete!")


if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description='Convert MIDI files to JSON format')
    parser.add_argument('input', nargs='?', help='Input MIDI file or directory', default="../notes/midi/")
    parser.add_argument('-o', '--output', help='Output JSON file or directory', default="../notes/note_data/")
    parser.add_argument('-b', '--batch', action='store_true', 
                        help='Process all MIDI files in input directory')
    
    args = parser.parse_args()
    
    # Check if input is a directory - if so, automatically use batch mode
    input_path = Path(args.input)
    if input_path.is_dir() or args.batch:
        batch_convert(args.input, args.output)
    else:
        midi_to_json(args.input, args.output)
