#!/usr/bin/env python3
"""
Extract piano note events from audio files by analyzing spectrograms.

This script:
1. Computes a spectrogram (STFT) with high frequency resolution
2. Maps the 88 piano keys (A0 to C8) to frequency bins
3. Goes through each time frame and detects peaks in the spectrum
4. Compares peak magnitudes to local context to identify note onsets
5. Outputs timestamped note events as JSON

Usage:
  python py_helper/extract_notes.py
  python py_helper/extract_notes.py --input notes/audio/song.wav --out notes/note_data/song_notes.json

Piano note range: A0 (27.5 Hz) to C8 (4186 Hz) = 88 keys
"""
import argparse
import json
import os
import numpy as np
from scipy import signal
from scipy.signal import find_peaks

try:
    import soundfile as sf
    _HAS_SF = True
except Exception:
    _HAS_SF = False

try:
    from scipy.io import wavfile
    _HAS_WAV = True
except Exception:
    _HAS_WAV = False


# Piano note frequencies (A0 = 27.5 Hz to C8 = 4186.01 Hz)
# Using equal temperament: f(n) = 440 * 2^((n-49)/12) where n=1..88
def get_piano_frequencies():
    """Return array of 88 piano note frequencies and their names."""
    notes = ['A', 'A#', 'B', 'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#']
    freqs = []
    names = []
    
    # Piano keys numbered 1-88, where key 49 = A4 = 440 Hz
    for key in range(1, 89):
        freq = 440.0 * (2.0 ** ((key - 49) / 12.0))
        freqs.append(freq)
        
        # Determine octave and note name
        # Key 1 = A0, Key 4 = C1, etc.
        # Offset to align: A0 is key 1, so (key-1) gives 0-based index
        idx = (key - 1) % 12
        # A0 starts at key 1; C1 starts at key 4 (offset +3 from A0)
        # Octave calculation: for A0-G#0 (keys 1-3) octave=0, C1-B1 (keys 4-15) octave=1
        octave = (key + 8) // 12  # adjusted formula to match piano standard
        note_name = notes[idx]
        names.append(f"{note_name}{octave}")
    
    return np.array(freqs), names


def read_audio(path):
    """Read audio file and return (data, samplerate)."""
    if _HAS_SF:
        data, sr = sf.read(path)
        return data, sr
    if _HAS_WAV:
        sr, data = wavfile.read(path)
        if data.dtype.kind in "iu":
            maxv = np.iinfo(data.dtype).max
            data = data.astype(np.float32) / float(maxv)
        return data, sr
    raise RuntimeError("No supported audio reader found. Install 'soundfile' or use WAV with scipy.")


def compute_spectrogram(x, sr, nperseg=4096, noverlap=None, window='hann'):
    """Compute STFT spectrogram. Higher nperseg = better frequency resolution."""
    if x.ndim > 1:
        x = np.mean(x, axis=1)
    
    if noverlap is None:
        noverlap = nperseg // 2
    
    f, t, Zxx = signal.stft(x, fs=sr, window=window, nperseg=nperseg, 
                            noverlap=noverlap, boundary=None)
    S = np.abs(Zxx)
    
    # Normalize to 0-1 range for consistent thresholding
    S_max = np.max(S)
    if S_max > 0:
        S = S / S_max
    
    return f, t, S


def map_notes_to_bins(freqs_hz, piano_freqs):
    """
    Map each of 88 piano frequencies to the nearest frequency bin.
    Returns array of bin indices for each piano note.
    """
    bins = []
    for pf in piano_freqs:
        # Find closest bin
        idx = np.argmin(np.abs(freqs_hz - pf))
        bins.append(idx)
    return np.array(bins)


def detect_note_events(S, t, f, piano_freqs, piano_names, 
                       onset_threshold=0.02, peak_prominence=0.01):
    """
    Detect piano note events from spectrogram.
    
    Strategy:
    1. For each time frame, extract magnitude at each piano note bin
    2. Find local peaks (notes that stand out in the spectrum)
    3. Compare current frame magnitude to previous frame to detect onsets
    4. Output note events: (time, note_name, magnitude)
    
    Parameters:
    - S: spectrogram magnitude (freq_bins x time_frames), normalized 0-1
    - t: time array
    - f: frequency array
    - piano_freqs: 88 piano frequencies
    - piano_names: 88 piano note names
    - onset_threshold: minimum increase in magnitude to trigger onset (default 0.02 = 2%)
    - peak_prominence: minimum prominence for peak detection in spectrum (default 0.01 = 1%)
    """
    note_bins = map_notes_to_bins(f, piano_freqs)
    
    # Extract magnitude time series for each piano note
    # note_series[i, j] = magnitude of piano note i at time frame j
    note_series = S[note_bins, :]  # shape: (88, num_frames)
    
    events = []
    prev_mags = np.zeros(88)
    
    # Track active notes to avoid duplicate detections
    active_notes = np.zeros(88, dtype=bool)
    
    for frame_idx in range(note_series.shape[1]):
        current_mags = note_series[:, frame_idx]
        timestamp = t[frame_idx]
        
        # Detect peaks in this frame's spectrum across the 88 notes
        # A peak means this note's magnitude is higher than neighboring notes
        peaks, properties = find_peaks(current_mags, prominence=peak_prominence)
        
        # For each peak, check if it's an onset (significant increase from previous frame)
        for peak_note_idx in peaks:
            mag = current_mags[peak_note_idx]
            prev_mag = prev_mags[peak_note_idx]
            
            # Onset detection: current magnitude significantly higher than previous
            # OR absolute magnitude is high and this note wasn't recently active
            is_onset = (mag > prev_mag + onset_threshold) or \
                       (mag > 0.1 and not active_notes[peak_note_idx] and mag > prev_mag * 1.5)
            
            if is_onset:
                events.append({
                    'time': float(timestamp),
                    'note': piano_names[peak_note_idx],
                    'frequency': float(piano_freqs[peak_note_idx]),
                    'magnitude': float(mag),
                    'midi': int(peak_note_idx + 21)  # MIDI note number (A0=21, C8=108)
                })
                active_notes[peak_note_idx] = True
        
        # Reset active notes when magnitude drops significantly
        for i in range(88):
            if active_notes[i] and current_mags[i] < prev_mags[i] * 0.5:
                active_notes[i] = False
        
        prev_mags = current_mags.copy()
    
    return events


def main():
    parser = argparse.ArgumentParser(description='Extract piano notes from audio files')
    parser.add_argument('--input', '-i', 
                       help='Input audio file or directory. If omitted, process all files in notes/audio')
    parser.add_argument('--out', '-o', 
                       help='Output JSON path for single file. If omitted, saves to notes/note_data/<name>_notes.json')
    parser.add_argument('--nperseg', type=int, default=4096, 
                       help='STFT window length (samples). Higher = better freq resolution')
    parser.add_argument('--noverlap', type=int, default=None, 
                       help='STFT overlap (samples). Default nperseg//2')
    parser.add_argument('--onset-threshold', type=float, default=0.02,
                       help='Minimum magnitude increase to detect note onset (default 0.02 = 2%%)')
    parser.add_argument('--peak-prominence', type=float, default=0.01,
                       help='Minimum prominence for peak detection (default 0.01 = 1%%)')
    args = parser.parse_args()
    
    # Piano note setup
    piano_freqs, piano_names = get_piano_frequencies()
    
    # Default directories
    default_audio_dir = os.path.join(os.path.dirname(__file__), '..', 'notes', 'audio')
    default_audio_dir = os.path.normpath(default_audio_dir)
    default_out_dir = os.path.join(os.path.dirname(__file__), '..', 'notes', 'note_data')
    default_out_dir = os.path.normpath(default_out_dir)
    
    def _process_file(path, out_dir):
        try:
            print(f"Processing {path}...")
            x, sr = read_audio(path)
        except Exception as e:
            print(f"Skipping {path}: failed to read ({e})")
            return
        
        # Compute spectrogram with high frequency resolution
        f, t, S = compute_spectrogram(x, sr, nperseg=args.nperseg, noverlap=args.noverlap)
        
        # Detect note events
        events = detect_note_events(S, t, f, piano_freqs, piano_names,
                                    onset_threshold=args.onset_threshold,
                                    peak_prominence=args.peak_prominence)
        
        # Save to JSON
        base = os.path.splitext(os.path.basename(path))[0]
        out_path = os.path.join(out_dir, f"{base}_notes.json")
        os.makedirs(out_dir, exist_ok=True)
        
        output = {
            'source_file': path,
            'sample_rate': sr,
            'duration': float(t[-1]) if len(t) > 0 else 0.0,
            'num_events': len(events),
            'events': events
        }
        
        with open(out_path, 'w') as fp:
            json.dump(output, fp, indent=2)
        
        print(f"  Found {len(events)} note events")
        print(f"  Saved to {out_path}")
    
    # Process input
    if args.input:
        inp = args.input
        if os.path.isdir(inp):
            files = [os.path.join(inp, f) for f in os.listdir(inp) 
                    if os.path.splitext(f)[1].lower() in ('.wav', '.flac', '.ogg', '.aiff', '.aif', '.mp3', '.m4a')]
            if not files:
                print(f"No supported audio files found in {inp}")
                return
            for p in sorted(files):
                _process_file(p, default_out_dir)
            return
        
        if os.path.isfile(inp):
            if args.out:
                out_path = args.out
                out_dir = os.path.dirname(out_path)
                if not out_dir:
                    out_dir = default_out_dir
                    out_path = os.path.join(out_dir, os.path.basename(out_path))
                
                print(f"Processing {inp}...")
                x, sr = read_audio(inp)
                f, t, S = compute_spectrogram(x, sr, nperseg=args.nperseg, noverlap=args.noverlap)
                events = detect_note_events(S, t, f, piano_freqs, piano_names,
                                           onset_threshold=args.onset_threshold,
                                           peak_prominence=args.peak_prominence)
                
                os.makedirs(out_dir, exist_ok=True)
                output = {
                    'source_file': inp,
                    'sample_rate': sr,
                    'duration': float(t[-1]) if len(t) > 0 else 0.0,
                    'num_events': len(events),
                    'events': events
                }
                with open(out_path, 'w') as fp:
                    json.dump(output, fp, indent=2)
                print(f"  Found {len(events)} note events")
                print(f"  Saved to {out_path}")
            else:
                _process_file(inp, default_out_dir)
            return
        
        print(f"Input path {inp} not found")
        return
    
    # No input: process all files in notes/audio
    if os.path.isdir(default_audio_dir):
        files = [os.path.join(default_audio_dir, f) for f in os.listdir(default_audio_dir)
                if os.path.splitext(f)[1].lower() in ('.wav', '.flac', '.ogg', '.aiff', '.aif', '.mp3', '.m4a')]
        if not files:
            print(f"No supported audio files found in {default_audio_dir}")
            return
        for p in sorted(files):
            _process_file(p, default_out_dir)
    else:
        print(f"Audio directory not found: {default_audio_dir}")


if __name__ == '__main__':
    main()
