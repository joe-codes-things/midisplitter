# MIDI Explode Algorithm - Adapted from Sibelius

## Overview

This document extracts the core Sibelius Explode algorithm and adapts it specifically for real-time MIDI processing. Notation-specific features have been removed to focus on the essential distribution logic.

---

## Core Distribution Algorithm

### 1. Note Analysis (Per Time Window)

```
For each time slice (5-10ms buffer window):
  - Count total simultaneous notes
  - Group notes that arrive within tolerance window
  - Sort notes by pitch (descending: highest to lowest)
```

### 2. Ratio-Based Distribution

The fundamental algorithm from Sibelius:

```
ratio1 = totalNotes / numVoices  // Notes per voice

if (ratio1 == floor(ratio1)) {
    // Even distribution possible
    notesPerVoice = ratio1;
} else {
    // Uneven - need remainder handling
    notesPerVoice = floor(ratio1);
    extraNotes = totalNotes - (numVoices * notesPerVoice);
}
```

**Three Distribution Scenarios:**

**Scenario A: More Notes Than Voices**
- Example: 10 notes → 4 voices
- Distribution: 2-2-3-3 or 3-3-2-2
- Extra notes distributed to specified voices

**Scenario B: Equal Notes and Voices**
- Example: 4 notes → 4 voices
- Distribution: 1-1-1-1
- Perfect one-to-one mapping

**Scenario C: Fewer Notes Than Voices**
- Example: 2 notes → 4 voices
- Distribution: Duplicate notes or leave voices empty
- User configurable behavior

### 3. Note Assignment

```
Sort incoming notes by pitch (highest to lowest)

voiceIndex = 0
for each note in sortedNotes:
    assignToVoice[voiceIndex]
    voiceIndex = (voiceIndex + 1) % numVoices

    // Handle extra notes
    if (voiceIndex == 0 && extraNotesRemaining > 0):
        assignToVoice[0]  // Give extra to Voice 1
        extraNotesRemaining--
```

### 4. MIDI Event Preservation

For each distributed note, preserve:
- **Pitch**: MIDI note number (0-127)
- **Velocity**: Note-on velocity (1-127)
- **Duration**: Time between note-on and note-off
- **Timing**: Exact timestamp relative to buffer start
- **Channel**: Original MIDI channel (optional)
- **CC Data**: Sustain pedal, modulation, etc.

---

## Real-Time Processing Flow

### Buffer-Based Processing

```
┌─────────────────────────────────────┐
│  MIDI Input Stream                  │
│  (from DAW track)                   │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Time Window Buffer (5-10ms)        │
│  - Collect simultaneous notes       │
│  - Group by timestamp tolerance     │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Sort Notes by Pitch                │
│  (Highest → Lowest)                 │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Calculate Distribution              │
│  - notesPerVoice = count / voices   │
│  - extraNotes = count % voices      │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Assign Notes to Voices             │
│  - Round-robin distribution         │
│  - Handle remainder notes           │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Send via IPC to Receivers          │
│  - Voice 1 → Receiver 1             │
│  - Voice 2 → Receiver 2             │
│  - Voice N → Receiver N             │
└─────────────────────────────────────┘
```

---

## Key Implementation Patterns

### 1. Timestamp Tolerance

**Problem:** MIDI events rarely arrive at exactly the same time

**Solution:** Use tolerance window for "simultaneous" detection
```cpp
const int TOLERANCE_MS = 5;  // 5ms window

bool areSimultaneous(MidiEvent a, MidiEvent b) {
    return abs(a.timestamp - b.timestamp) < TOLERANCE_MS;
}
```

### 2. Note Caching

**Problem:** Can't modify MIDI buffer while iterating

**Solution:** Cache notes first, then process
```cpp
std::vector<MidiNote> noteBuffer;

// Collect phase
for (auto& event : midiInput) {
    if (event.isNoteOn()) {
        noteBuffer.push_back(event);
    }
}

// Process phase
distributeNotes(noteBuffer);
```

### 3. Pitch-Based Sorting

```cpp
std::sort(noteBuffer.begin(), noteBuffer.end(),
    [](const MidiNote& a, const MidiNote& b) {
        return a.pitch > b.pitch;  // Descending order
    });
```

### 4. Round-Robin Distribution

```cpp
int voiceIndex = 0;
for (const auto& note : sortedNotes) {
    voiceAssignments[voiceIndex].push_back(note);
    voiceIndex = (voiceIndex + 1) % numVoices;
}

// Handle extra notes
for (int i = 0; i < extraNotes; i++) {
    // Assign to Voice 1 or distribute evenly
    voiceAssignments[i % numVoices].push_back(extraNotesList[i]);
}
```

---

## Distribution Modes

### Mode 1: Even Distribution (Default)
- Distributes notes as evenly as possible
- Extra notes go to Voice 1
- Best for general orchestration

### Mode 2: Top-Heavy Distribution
- Extra notes go to higher voices (Voice 1, 2, etc.)
- Useful for melody-focused arrangements
- Emphasizes upper register

### Mode 3: Bottom-Heavy Distribution
- Extra notes go to lower voices
- Useful for bass-heavy arrangements
- Emphasizes lower register

### Mode 4: Alternating Distribution
- Extra notes alternate between voices
- Most balanced distribution
- Good for rhythmic patterns

---

## Edge Cases & Solutions

### 1. Single Note Input
**Scenario:** Only 1 note, but 8 voices configured
**Solution:**
- Send to Voice 1 only
- Or duplicate to all voices (user option)

### 2. More Notes Than Voices
**Scenario:** 12 notes, 8 voices
**Solution:**
- 8 voices get 1 note each
- 4 extra notes distributed to Voices 1-4

### 3. Rapid Note Bursts
**Scenario:** Notes arrive faster than buffer can process
**Solution:**
- Increase buffer size
- Process in chunks
- Maintain note-on/note-off pairing

### 4. Sustained Notes Across Buffers
**Scenario:** Note-on in buffer 1, note-off in buffer 3
**Solution:**
- Track active notes across buffers
- Maintain voice assignment for duration
- Send note-off to same voice as note-on

---

## Performance Considerations

### 1. Buffer Size Optimization
- **Too small (1-2ms)**: May split chords incorrectly
- **Too large (20-50ms)**: Introduces noticeable latency
- **Optimal (5-10ms)**: Balances accuracy and latency

### 2. Sorting Efficiency
- Use stable sort to preserve input order for equal pitches
- Consider pre-allocated buffers to avoid memory allocation

### 3. IPC Efficiency
- Batch multiple notes per IPC message when possible
- Use binary protocol (not JSON/XML) for speed
- Minimize message overhead

---

## Comparison: Sibelius vs MIDI Plugin

| Feature | Sibelius Explode | MIDI Plugin |
|---------|------------------|-------------|
| Input | Notation (bars/staves) | Real-time MIDI stream |
| Processing | Bar-by-bar | Buffer-by-buffer (5-10ms) |
| Voice limit | 4 voices | 8 voices |
| Text recognition | Yes (a1, 1., etc.) | No (not applicable) |
| Rest handling | Yes (optimize/combine) | No (MIDI has no rests) |
| Accidental management | Yes | No (MIDI uses note numbers) |
| Cue passages | Yes | No (notation-specific) |
| Duplicate detection | Position-based tolerance | Timestamp-based tolerance |
| Distribution algorithm | Ratio-based | Ratio-based (same core) |
| Object copying | Text, lines, symbols | MIDI events only |

---

## Algorithm Pseudocode

```cpp
class MIDIExploder {
    int numVoices = 4;
    int bufferSizeMs = 5;
    std::vector<MidiNote> noteBuffer;

    void processBlock(MidiBuffer& input) {
        // 1. Collect notes in time window
        for (auto event : input) {
            if (event.isNoteOn()) {
                noteBuffer.push_back(event);
            }
        }

        // 2. Check if buffer is ready to process
        if (bufferReady()) {
            // 3. Sort by pitch (descending)
            std::sort(noteBuffer.begin(), noteBuffer.end(),
                [](auto& a, auto& b) { return a.pitch > b.pitch; });

            // 4. Calculate distribution
            int totalNotes = noteBuffer.size();
            int notesPerVoice = totalNotes / numVoices;
            int extraNotes = totalNotes % numVoices;

            // 5. Distribute notes
            int voiceIndex = 0;
            for (auto& note : noteBuffer) {
                assignNoteToVoice(note, voiceIndex);
                voiceIndex = (voiceIndex + 1) % numVoices;
            }

            // 6. Handle extra notes
            for (int i = 0; i < extraNotes; i++) {
                // Assign to Voice 1 or distribute
            }

            // 7. Send to receivers via IPC
            sendToReceivers();

            // 8. Clear buffer
            noteBuffer.clear();
        }
    }

    bool bufferReady() {
        if (noteBuffer.empty()) return false;

        auto now = getCurrentTime();
        auto oldestNote = noteBuffer.front().timestamp;

        return (now - oldestNote) >= bufferSizeMs;
    }
};
```

---

## Key Takeaways

✅ **Use ratio-based distribution** - Core algorithm from Sibelius
✅ **Sort by pitch** - Highest notes to Voice 1, descending
✅ **Buffer-based processing** - Group simultaneous notes (5-10ms window)
✅ **Preserve MIDI context** - Maintain note-on/off pairs, CC data
✅ **Handle edge cases** - Single notes, extra notes, sustained notes
✅ **Optimize for real-time** - Minimize latency, efficient IPC

❌ **Skip notation features** - Text recognition, rest optimization, accidentals
❌ **Skip cue passages** - Notation-specific feature
❌ **Skip object copying** - Only MIDI events, no text/symbols/lines

---

*Adapted from Sibelius Explode.plg source code analysis for real-time MIDI processing*
