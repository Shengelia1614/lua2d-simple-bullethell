#!/usr/bin/env python3
"""
Generate and save a spectrogram (frequency spectrum over time) from an audio file
or from a synthetic test signal. Saves a PNG by default when run without arguments.

Usage examples:
  python py_helper/spectrogram.py --input path/to/file.wav --out out.png
  python py_helper/spectrogram.py            # creates a test chirp and saves test_spectrogram.png

Dependencies:
  numpy, scipy, matplotlib, soundfile (optional but recommended)

This script uses scipy.signal.stft to compute a short-time Fourier transform and
matplotlib to plot the resulting spectrogram (magnitude in dB).
"""
import argparse
import os
import numpy as np

# Use a non-interactive backend so the script can run in headless environments
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

from scipy import signal

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


def read_audio(path):
    """Read audio file and return (data, samplerate).

    Tries soundfile first (handles many formats); falls back to scipy.io.wavfile
    which supports WAV files.
    """
    if _HAS_SF:
        data, sr = sf.read(path)
        # soundfile returns float arrays for most formats
        return data, sr

    if _HAS_WAV:
        sr, data = wavfile.read(path)
        # convert integers to floats in [-1,1]
        if data.dtype.kind in "iu":
            maxv = np.iinfo(data.dtype).max
            data = data.astype(np.float32) / float(maxv)
        return data, sr

    raise RuntimeError("No supported audio reader found. Install 'soundfile' or use WAV files and scipy.")


def compute_spectrogram(x, sr, nperseg=2048, noverlap=None, window='hann'):
    if x.ndim > 1:
        # if stereo, average to mono
        x = np.mean(x, axis=1)

    if noverlap is None:
        noverlap = nperseg // 2

    f, t, Zxx = signal.stft(x, fs=sr, window=window, nperseg=nperseg, noverlap=noverlap, boundary=None)
    S = np.abs(Zxx)
    # Convert to dB scale
    eps = 1e-10
    S_db = 20.0 * np.log10(S + eps)
    return f, t, S_db


def plot_spectrogram(f, t, S_db, out_path, sr, cmap='viridis', vmax=None, vmin=None, ylabel='Frequency (Hz)', ylog=False):
    plt.figure(figsize=(10, 5))
    if ylog:
        # For log yscale we need to use imshow and transform frequency axis
        # But pcolormesh with log scale works too if we set the axis afterwards.
        mesh = plt.pcolormesh(t, f, S_db, shading='gouraud', cmap=cmap)
        plt.yscale('log')
        plt.ylim([f[f>0].min(), sr/2])
    else:
        mesh = plt.pcolormesh(t, f, S_db, shading='gouraud', cmap=cmap)

    plt.xlabel('Time (s)')
    plt.ylabel(ylabel)
    plt.title('Spectrogram (dB)')
    plt.colorbar(mesh, label='Amplitude (dB)')
    plt.tight_layout()
    plt.savefig(out_path, dpi=200)
    plt.close()


def main():
    parser = argparse.ArgumentParser(description='Compute and save spectrogram(s) from audio')
    parser.add_argument('--input', '-i', help='Input audio file or directory. If omitted, process all files in notes/audio')
    parser.add_argument('--out', '-o', help='Output image path (file) when processing single file. If omitted, defaults to notes/note_data/<name>_spectrogram.png')
    parser.add_argument('--nperseg', type=int, default=2048, help='STFT window length (samples)')
    parser.add_argument('--noverlap', type=int, default=None, help='STFT overlap (samples). Default nperseg//2')
    parser.add_argument('--logy', action='store_true', help='Use log-frequency y-axis')
    args = parser.parse_args()

    # default directories per repo layout
    default_audio_dir = os.path.join(os.path.dirname(__file__), '..', 'notes', 'audio')
    default_audio_dir = os.path.normpath(default_audio_dir)
    default_out_dir = os.path.join(os.path.dirname(__file__), '..', 'notes', 'note_data')
    default_out_dir = os.path.normpath(default_out_dir)

    def _process_file(path, out_dir, nperseg, noverlap, logy):
        try:
            x, sr = read_audio(path)
        except Exception as e:
            print(f"Skipping {path}: failed to read ({e})")
            return

        f, tt, S_db = compute_spectrogram(x, sr, nperseg=nperseg, noverlap=noverlap)
        base = os.path.splitext(os.path.basename(path))[0]
        out_path = os.path.join(out_dir, f"{base}_spectrogram.png")
        os.makedirs(out_dir, exist_ok=True)
        plot_spectrogram(f, tt, S_db, out_path, sr, ylog=logy)
        print(f"Saved spectrogram for {path} -> {out_path}")

    if args.input:
        inp = args.input
        if os.path.isdir(inp):
            files = [os.path.join(inp, f) for f in os.listdir(inp) if os.path.splitext(f)[1].lower() in ('.wav', '.flac', '.ogg', '.aiff', '.aif', '.mp3', '.m4a')]
            if not files:
                print(f"No supported audio files found in {inp}")
                return
            out_dir = default_out_dir
            for p in sorted(files):
                _process_file(p, out_dir, args.nperseg, args.noverlap, args.logy)
            return

        if os.path.isfile(inp):
            out_path = args.out
            if out_path:
                out_dir = os.path.dirname(out_path) or default_out_dir
                os.makedirs(out_dir, exist_ok=True)
                try:
                    x, sr = read_audio(inp)
                except Exception as e:
                    raise
                f, tt, S_db = compute_spectrogram(x, sr, nperseg=args.nperseg, noverlap=args.noverlap)
                plot_spectrogram(f, tt, S_db, out_path, sr, ylog=args.logy)
                print(f"Saved spectrogram to {out_path}")
                return
            else:
                # single file, default output directory
                out_dir = default_out_dir
                _process_file(inp, out_dir, args.nperseg, args.noverlap, args.logy)
                return

        print(f"Input path {inp} not found")
        return

    # No input provided: process all audio files in notes/audio
    if os.path.isdir(default_audio_dir):
        files = [os.path.join(default_audio_dir, f) for f in os.listdir(default_audio_dir) if os.path.splitext(f)[1].lower() in ('.wav', '.flac', '.ogg', '.aiff', '.aif', '.mp3', '.m4a')]
        if not files:
            print(f"No supported audio files found in {default_audio_dir}. Generating test signal instead.")
        else:
            for p in sorted(files):
                _process_file(p, default_out_dir, args.nperseg, args.noverlap, args.logy)
            return

    # fallback: create a test chirp and save it
    sr = 44100
    duration = 5.0
    t = np.linspace(0, duration, int(sr*duration), endpoint=False)
    x = signal.chirp(t, f0=50.0, t1=duration, f1=sr/2.0, method='log')
    os.makedirs(default_out_dir, exist_ok=True)
    f, tt, S_db = compute_spectrogram(x, sr, nperseg=args.nperseg, noverlap=args.noverlap)
    out_path = os.path.join(default_out_dir, 'test_spectrogram.png')
    plot_spectrogram(f, tt, S_db, out_path, sr, ylog=args.logy)
    print(f"Saved test spectrogram to {out_path}")


if __name__ == '__main__':
    main()
