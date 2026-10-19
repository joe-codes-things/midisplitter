MIDI SPLITTER FEATURE IDEAS

Preset System

Save/recall distribution settings
Include voice count, CC filters, distribution mode
Factory presets: "Orchestral", "Pad Layers", "Rhythmic", etc.


Voice Groups

Group voices together (e.g., Voices 1-4 = Strings, 5-8 = Brass)
Apply settings to entire groups
Different distribution algorithms per group


Delay/Timing Offset

Slight timing offset per voice (0-100ms)
Creates natural ensemble feel
Humanization and width


Adjustable note-entry grouping window

Allow users to configure how far apart incoming note starts can be and still
be treated as one chord for voice distribution. The current default setup uses
a 7ms processing buffer and a 1ms timestamp tolerance; the audio host's block
size also affects actual grouping.


MIDI humanization

Add an optional humanization mode that varies note start and end times and
reshapes incoming CC data before the plugin distributes it. The aim is to turn
quantized, exact MIDI into a more natural performance, with controls for the
amount of timing and CC variation.


Per-Voice Transposition

Transpose each voice independently (+/- 24 semitones)


## Parameter Automation ✅ IMPLEMENTED (0.0.17)

Full DAW automation support for key plugin parameters has been implemented.

### Automatable Parameters:
- **Bypass Plugin** - Enable/disable plugin processing (default: disabled)
  - When enabled: Passes all MIDI through unchanged (no distribution, no CC filtering)
  - When disabled: Plugin processes normally
- **Voice Count** (2-16) - Dynamically adjust number of active voices (default: 4)
  - Changes voice distribution in real-time
  - 15 discrete steps corresponding to 2-16 voices
- **Modulation (CC1) Filter** - Toggle whether to broadcast Mod Wheel (default: enabled)
- **Expression (CC11) Filter** - Toggle whether to broadcast Expression (default: enabled)
- **Volume (CC7) Filter** - Toggle whether to broadcast Volume (default: enabled)

### Use Cases:
- Automate voice count for dynamic texture changes (e.g., 2 voices → 16 voices for crescendo)
- Toggle CC filters mid-performance (e.g., disable mod wheel for certain sections)
- Bypass automation for quick A/B comparison or routing changes
- Create evolving textures by changing voice count over time

### Implementation Details:
- All parameters exposed to DAW automation lanes via JUCE AudioProcessorParameter system
- Works identically in both VST3 and AU formats (JUCE handles format translation)
- Parameters sync bidirectionally with UI controls
- Voice count changes handled gracefully without disrupting active notes
- CC filter toggles take effect immediately
- Bypass provides clean MIDI passthrough when enabled
- State persistence automatic via JUCE parameter system
