# Implementation Status

**Last verified:** 2026-10-19
**Project/plugin version:** `0.0.17`
**Framework:** JUCE 8.0.4
**Build system:** CMake 3.22+
**Supported platforms:** macOS and Windows

## Available builds

- macOS VST3 and AU
- Windows x64 VST3 (built with native MSVC on the ARM64 Windows 11 VM)
- macOS `.pkg`/`.dmg` packaging scripts and a Windows x64 Inno Setup installer

The CMake project version is the source of truth for JUCE plugin metadata and
the UI's generated version header. The Windows installer uses the same
three-component version for its application and file-version metadata (for
example, `0.0.17`, not `0.0.17.0`).

## Implemented functionality

- Distributes incoming MIDI notes across 2-16 voices, mapped to MIDI channels
  1-16.
- Sorts simultaneous notes by pitch and assigns them using the distribution
  algorithm.
- Tracks sustained notes across buffers and broadcasts Control Change messages.
- Provides selectable filtering for Modulation (CC1), Expression (CC11), and
  Volume (CC7).
- Persists plugin state and exposes bypass, voice count, and CC filters as
  automatable parameters.
- Builds platform-appropriate wrappers from shared source: VST3 on both
  platforms and AU on macOS.

## Experimental VST3 inter-instance MIDI

The `experiment/vst3-midi-bridge` branch adds a VST3-only Sender/Receiver
selector, receiver voice selection, and an asynchronous MIDI event queue
between plugin instances. Its VST3 category is `Fx|MIDI`, not `Instrument`;
the receiver emits only events received for its selected voice. The prototype
uses a single experiment session namespace; test it with one Sender and no
more than one Receiver per voice.

This transport and its latency have not been validated in Cubase. It is not
sample-accurate by contract, does not yet offer arbitrary Sender/Receiver
pairing, and must not be treated as a production feature.

### Cubase 15 MIDI output routing research

Steinberg's [Cubase Pro 15 manual: MIDI Inserts](https://www.steinberg.help/r/cubase-pro/15.0/en/cubase_nuendo/topics/midi_realtime_parameters_and_effects/midi_realtime_parameters_midi_effects_midi_inserts_r.html)
describes MIDI Inserts as part of the MIDI signal chain, says MIDI effects can
add events, and documents recording their output as events on a MIDI or
Instrument Track. Its [Instrument Track Inspector documentation](https://www.steinberg.help/r/cubase-pro/15.0/en/cubase_nuendo/topics/tracks_about/tracks_about_instrument_track_inspector_r.html)
lists MIDI Inserts for MIDI-related tracks/channels and distinguishes those
from audio Inserts for audio-related tracks/channels. This supports same-track
instrument triggering when a MIDI effect is in the track's MIDI Insert path;
it does not apply to an ordinary audio Insert.

The [VST 3 processing API](https://steinbergmedia.github.io/vst3_doc/vstinterfaces/structSteinberg_1_1Vst_1_1ProcessData.html)
defines optional outgoing `outputEvents`, and its [MIDI documentation](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/About+MIDI/Index.html)
explains that MIDI is represented with VST 3 events. This establishes that a
VST 3 plug-in can emit events to its host, not that Cubase routes every VST 3
effect's events into the instrument on the same track.

**Unverified in Cubase 15:** whether Cubase accepts this third-party VST 3
`Fx|MIDI` plug-in in its MIDI Inserts selector and forwards its emitted events
to the hosted instrument. The historical report in [B004](./BUGS.md#b004-vst3-midi-transmission-not-working-despite-connected-status)
records an earlier Cubase limitation, but is not a Cubase 15 verification.
Until tested in Cubase 15, this remains a host compatibility question.

## Verification and limitations

- The latest post-reorganization macOS build passed **81/81 CTest tests**.
  The Windows suite was not rerun during the reorganization; the preceding
  combined build passed **79/79 on Windows**.
- The Windows VST3 binary was verified as PE32+ AMD64/x64.
- macOS VST3 and AU bundle metadata and Windows VST3 metadata report version
  `0.0.17`.
- The Windows installer was rebuilt with three-component version metadata.
- The project owner has confirmed that the Windows installation works.
- Automated processor tests do not load a DAW or validate the AU wrapper.
  No DAW/version compatibility matrix is claimed.
- macOS artifacts are ad-hoc signed, not Developer ID signed or notarized.
  Windows plugin and installer artifacts are unsigned.
- Vendored VST3 SDK compiler warnings remain; first-party warnings were
  addressed.

See [WINDOWS_BUILD.md](./WINDOWS_BUILD.md) for the native Windows build setup,
[BUGS.md](./BUGS.md) for tracked issues, and [FEATURE-IDEAS.md](./FEATURE-IDEAS.md)
for the feature backlog.
