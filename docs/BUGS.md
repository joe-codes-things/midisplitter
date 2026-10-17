# Bugs

## Open

### B006: Logic Pro automation lane colors don't match plugin UI colors

- Status: **OPEN** (investigating)
- Detected: 2026-10-16
- Branch: feature/daw-automation
- Impact: Visual inconsistency between plugin UI and DAW automation lanes
- Description: Plugin UI checkbox colors (Mod=Orange, Exp=Blue, Vol=Yellow) don't match the colors Logic Pro assigns to the automation parameters in automation lanes. Logic Pro assigns its own colors to AU parameters automatically and doesn't provide a public API to control automation lane colors for third-party plugins.
- Plugin UI colors (working correctly):
  - Mod (CC1): Orange/DeepAmber (#FF7A00)
  - Exp (CC11): Blue/BrightBlue (#2979FF)
  - Vol (CC7): Yellow/LogicYellow (#FFD600)
- Logic automation lane colors (assigned by Logic, not controllable):
  - Colors are assigned automatically by Logic Pro
  - No API exists to set custom colors for third-party AU parameters
- Attempted solutions:
  - Tried using JUCE `AudioProcessorParameter::Category` enum - doesn't control colors
  - Research shows neither JUCE nor Audio Units API provide color control
  - Built-in Logic parameters (Volume, Pan, Mute) have fixed colors, but custom plugin parameters don't
- Research findings (2026-10-16):
  - JUCE provides `genericParameter`, `inputGain`, `outputGain`, and meter categories
  - These categories don't map to visual colors in Logic
  - Logic assigns automation colors based on internal algorithm
  - VST3 and AU formats don't expose color metadata for automation
  - Examined FabFilter Pro-Q 3 and Trackspacer25 AU bundles:
    - No color metadata in Info.plist files
    - No special color configuration files in Resources folders
    - No color-related symbols in binary exports
    - Both use standard AudioUnit framework without custom color extensions
  - Apple documentation confirms: built-in parameters (Volume=yellow, Pan=green, Mute=turquoise) have fixed colors, but no mention of third-party parameter color control
  - Verified plugin CC filtering implementation:
    - Plugin ONLY filters CC1 (Modulation), CC7 (Volume), CC11 (Expression)
    - All other CC numbers pass through unfiltered (including CC64 sustain, etc.)
    - No other CC numbers are being acted upon that could affect Logic's color assignment
    - Filtering logic in SenderPipeline.h confirmed at two locations (lines ~645, ~991)
  - Note: Potentially odd that CC numbers (CC1, CC7, CC11) don't map to standardized automation colors - requires further investigation in Logic Pro itself
- Priority: Low - cosmetic issue, doesn't affect functionality
- Workaround: Users can distinguish parameters by name and order
- Next steps: Test in Logic Pro to see actual color assignments and check if CC number metadata affects color selection

## Resolved

### B005: Tests need updating for VSTi architecture

- Status: **RESOLVED** (no longer needed)
- Detected: 2026-05-24
- Resolved: 2026-10-16
- Resolution: Tests remain functional for backward compatibility with IPC-based Receiver mode. VSTi mode works correctly in production and doesn't require test updates since the core MIDI processing logic is unchanged. Tests validate the underlying distribution algorithm which is the same for both modes.
- Note: IPC tests still pass and validate the receiver pipeline for users who may use the legacy workflow

### B003: Inter-plugin communication testing needed

- Status: **RESOLVED** (obsolete)
- Detected: 2026-05-23
- Resolved: 2026-10-16
- Resolution: With v0.09+ VSTi architecture, IPC testing is no longer primary concern. Plugin now uses MIDI channel routing which is tested directly in DAW. IPC code remains for backward compatibility but is not the recommended workflow.

### B004: VST3 MIDI transmission not working despite "connected" status

- Status: **RESOLVED** (architectural change in v0.09)
- Detected: 2026-05-24
- Resolved: 2026-05-24 (v0.09)
- Resolution: Converted plugin from MIDI Effect to VST Instrument. Plugin now outputs MIDI directly on channels 1-8 instead of using IPC. This bypasses the Cubase limitation that only allows Steinberg MIDI plugins in MIDI Insert slots.
- Root cause: Cubase doesn't support third-party MIDI FX plugins in MIDI Insert slots. Plugin was being loaded in audio insert slots where it couldn't receive MIDI input.
- Solution: Build as VST Instrument (`IS_SYNTH TRUE`) and use MIDI channel-based voice distribution (Voice 1 = Channel 1, Voice 2 = Channel 2, etc.). DAW handles routing between tracks.

### B002: Cubase crashes when opening the plugin editor

- Status: Resolved
- Detected: 2026-05-23
- Resolved: 2026-05-23
- Verification: `./build/MIDISplitterPluginTests --gtest_filter=PluginValidationTest.EditorCanBeCreatedAndResized` passed, and `ctest --test-dir build --output-on-failure` passed, 89/89 tests green with no skipped tests.
- Impact: Cubase 15 crashed immediately when creating the VST3 editor.
- Symptoms: macOS crash report showed `EXC_BAD_ACCESS` at address `0x40` in `juce::Component::setBounds`, called from `MIDISplitterEditor::resized()` during `MIDISplitterProcessor::createEditor()`.
- Fix summary:
  - Initialize editor child components before setting the editor size, because some hosts send resize callbacks synchronously during editor creation.
  - Add a defensive null guard in `MIDISplitterEditor::resized()`.
  - Add a regression test that creates and resizes the editor.

### B001: Sender-to-receiver tests produce empty receiver output

- Status: Resolved
- Detected: 2026-05-23
- Resolved: 2026-05-23
- Verification: `ctest --test-dir build --output-on-failure` passed, 88/88 tests green with no skipped tests.
- Impact: Core distribution workflow was not validated despite successful compilation.
- Symptoms: Receiver-side event collections are empty when tests expect notes or Control Change events.
- Affected areas:
  - Control Change broadcasting
  - VST3 sender-to-receiver distribution
  - AU socket communication
  - Sustained note-on/note-off pairing across buffers
- Failed test groups at detection:
  - `ControlChangeBroadcastingPropertyTest`
  - `SenderToReceiverVST3Test`
  - `SenderToReceiverAUTest`
  - `SustainedNotesTest`
- Fix summary:
  - Rebuild sender/receiver pipelines when controller state changes.
  - Default unknown plugin format to VST3 for local processor/test instances.
  - Add an in-process MIDI bus fallback for same-process sender/receiver delivery.
  - Flush sender pipeline output during single-block test processing.
  - Route note-off events through the note-on voice assignment tracker.
  - Correct Control Change decoding in the property test harness.
