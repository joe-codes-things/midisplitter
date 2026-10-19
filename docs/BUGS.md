# Bugs

## Open

### B007: JUCE VST3 SDK emits a shadow-field compiler warning

- Status: **OPEN** (monitoring; no functional failure observed)
- Detected: 2026-10-19
- Impact: Adds a compiler warning to plugin build output; builds and tests still succeed.
- Description: Clang reports `-Wshadow-field-in-constructor` for the `FReleaser` constructor parameter named `u` shadowing its member named `u` in JUCE's bundled VST3 SDK header at `JUCE_framework/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/funknown.h` (constructor around line 549; member around line 555 in the currently pinned JUCE checkout).
- Scope: Observed when building the plugin target in both the `main` and `development` worktrees. The warning originates in JUCE dependency code, not the MIDI Splitter source. Both macOS plugin builds completed successfully, and both CTest suites passed 79/79 tests.
- Priority: Low - non-fatal third-party dependency warning with no known functional impact.
- Workaround: None needed; leave the pinned JUCE files unchanged.
- Next steps: If considering a future JUCE upgrade, first verify that the candidate revision fixes this specific warning, then build and test both supported platforms before adopting it. Do not modify the vendored JUCE files just to silence the warning.

### B006: Logic Pro automation lane colors don't match plugin UI colors

- Status: **OPEN** (investigating)
- Detected: 2026-10-16
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
    - CC filtering remains limited to CC1, CC7, and CC11.
  - Note: Potentially odd that CC numbers (CC1, CC7, CC11) don't map to standardized automation colors - requires further investigation in Logic Pro itself
- Priority: Low - cosmetic issue, doesn't affect functionality
- Workaround: Users can distinguish parameters by name and order
- Next steps: Test in Logic Pro to see actual color assignments and check if CC number metadata affects color selection

## Resolved

### B005: Tests need updating for VSTi architecture

- Status: **RESOLVED** (no longer needed)
- Detected: 2026-05-24
- Resolved: 2026-10-17
- Resolution: Sender processor tests now inspect the MIDI output channels produced by the VSTi architecture instead of expecting raw processor instances to communicate through IPC. The AU-labelled test fixture did not instantiate or validate an AU wrapper, so it is no longer included in CTest. AU host compatibility is not covered by these in-process tests.
- Verification: The macOS and Windows CTest suites each passed with 79/79 tests in the combined build.

### B003: Inter-plugin communication testing needed

- Status: **RESOLVED** (obsolete)
- Detected: 2026-05-23
- Resolved: 2026-10-16
- Resolution: The VST Instrument architecture uses MIDI channel routing as its primary voice output. Processor tests cover channel-routed MIDI output; DAW routing is not covered by those tests. IPC code remains for backward compatibility.

### B004: VST3 MIDI transmission not working despite "connected" status

- Status: **RESOLVED** (architectural change in v0.09)
- Detected: 2026-05-24
- Resolved: 2026-05-24 (v0.09)
- Historical resolution: Converted plugin from MIDI Effect to VST Instrument. The instrument outputs MIDI on channels 1-8 instead of using IPC.
- Historical root-cause report: Cubase at the time was reported to allow only Steinberg MIDI plug-ins in MIDI Insert slots; the third-party plug-in was being loaded as an audio Insert and did not receive MIDI input there.
- Solution: Build as a VST Instrument (`IS_SYNTH TRUE`) and use MIDI channel-based voice distribution (Voice 1 = Channel 1, Voice 2 = Channel 2, etc.). The plugin now supports up to 16 voices; hosts are responsible for routing channels between tracks.
- Current status of the report: this historical behavior has not been re-verified in Cubase 15. Cubase 15's manual documents MIDI Inserts as a separate MIDI signal path and distinguishes them from audio Inserts; see [Cubase 15 MIDI routing research](./IMPLEMENTATION-STATUS.md#cubase-15-midi-output-routing-research).

### B002: Cubase crashes when opening the plugin editor

- Status: Resolved
- Detected: 2026-05-23
- Resolved: 2026-05-23
- Historical verification at resolution: the editor regression test passed, and the then-current CTest suite passed 89/89 tests.
- Impact: Cubase 15 crashed immediately when creating the VST3 editor.
- Symptoms: macOS crash report showed `EXC_BAD_ACCESS` at address `0x40` in `juce::Component::setBounds`, called from `MIDISplitterEditor::resized()` during `MIDISplitterProcessor::createEditor()`.
- Fix summary:
  - Initialize editor child components before setting the editor size, because some hosts send resize callbacks synchronously during editor creation.
  - Add a defensive null guard in `MIDISplitterEditor::resized()`.
  - Add a regression test that creates and resizes the editor.

### B001: Sender-to-receiver tests produce empty receiver output

- Status: Resolved
- Detected: 2026-05-23
- Resolved: 2026-10-17
- Verification: The macOS and Windows CTest suites each passed with 79/79 tests in the combined build.
- Impact: Core distribution workflow was not validated despite successful compilation.
- Symptoms: Receiver-side event collections are empty when tests expect notes or Control Change events.
- Affected areas:
  - Control Change broadcasting
  - VST3 channel-routed sender output
  - Sustained note-on/note-off pairing across buffers
- Failed test groups at detection:
  - `ControlChangeBroadcastingPropertyTest`
  - `SenderToReceiverVST3Test`
  - `SustainedNotesTest`
- Fix summary:
  - Test the processor's actual per-voice MIDI channel output instead of creating independent receiver instances that the processor does not drive.
  - Deliver in-process IPC assignments only to subscribers registered for the matching voice, and use the cross-process transport for assignments without a local subscriber.
  - Preserve the host voice-count parameter when changing the count through the editor and while saving/restoring plugin state.
  - Use monotonic receipt timestamps for stale-note cleanup instead of comparing block-relative MIDI timestamps with wall-clock time.
