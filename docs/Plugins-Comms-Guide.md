# Cross-Format Inter-Plugin Communication Guide (VST3 & AU)

## Objective

Enable multiple instances of the same plugin (loaded on different tracks in a DAW) to exchange data bidirectionally within the same plugin format. The specific use case involves sharing MIDI-derived data between plugin instances, similar to how Izotope Neutron handles its EQ masking feature.

## Scope and Format Requirements

**Important**: This implementation supports **same-format communication only**:

- **AU to AU**: For Logic Pro users, all instances must use the AU format
- **VST3 to VST3**: For Cubase users, all instances must use the VST3 format
- **Ableton Live users**: Must use a single plugin format (either AU or VST3) for all instances of the MIDI Splitter plugin

**Cross-format communication (AU ↔ VST3) is NOT supported.** Users must ensure all instances of the plugin in their session use the same format to enable inter-plugin communication.

## Architectural Overview

The fundamental challenge is that audio plugins have strict restrictions that prevent standard IPC mechanisms:

- **No background threads** during plugin initialization (sandbox violations)
- **Audio thread must never block** (no network I/O, file I/O, or locks)
- **No official inter-plugin API** exists in either VST3 or AU

**The Solution**: We use a **shared memory approach** that works for both formats:
- When plugins run in the **same process** (typical case), shared memory enables zero-copy communication
- When plugins run in **separate processes**, the InProcessMIDIBus automatically falls back to file-based discovery
- This approach is **DAW-agnostic** and works reliably across Logic Pro, Cubase, and other hosts

---

## Part 1: VST3 Inter-Plugin Communication

### Critical Research Finding: IConnectionPoint is NOT for Inter-Plugin Communication

**Important Correction**: After extensive research and testing, we discovered that **VST3's `IConnectionPoint` and `IMessage` interfaces are NOT designed for communication between separate plugin instances**.

- **IConnectionPoint Purpose**: Connects a plugin's **own processor to its own controller** within the same instance
- **Host Role**: The host establishes this connection, not the plugin itself
- **Steinberg Documentation**: Explicitly states this is for "communication of separate components" (meaning components of the same plugin, not different instances)

### The Real Solution: File-Based IPC (IPCManagerVST3_Socket)

After extensive testing, we discovered that **Cubase DOES sandbox VST3 plugins into separate processes**. This means shared memory approaches don't work.

Instead, we use **file-based IPC** for cross-process communication:

#### Why File-Based IPC Works

1. **Cubase sandboxes VST3 plugins** - each instance runs in a separate process
2. **Files are visible across all processes** - standard filesystem IPC
3. **Atomic writes** - JUCE's TemporaryFile ensures data integrity
4. **Works across all hosts** - not dependent on specific host behavior

#### The File-Based IPC Architecture

```
┌──────────────────────────────────────────────────────────┐
│                    DAW Process 1                         │
│  ┌─────────────────┐                                     │
│  │  VST3 Sender    │──┐                                │
│  │  (Process A)    │  │ writes to                      │
│  └─────────────────┘  │ /tmp/MIDI_Splitter_IPC/         │
└───────────────────────┼─────────────────────────────────┘
                        │ voice_1.dat, voice_2.dat, etc.
┌───────────────────────┼─────────────────────────────────┐
│  DAW Process 2        │                                 │
│  ┌─────────────────┐  │                                 │
│  │  VST3 Receiver  │◄─┘ reads from                    │
│  │  (Process B)    │    /tmp/MIDI_Splitter_IPC/       │
│  └─────────────────┘      voice_N.dat (for its voice)   │
└──────────────────────────────────────────────────────────┘
```

#### Key Implementation Details

- **IPCManagerVST3_Socket**: Manages file-based communication
- **Session-based directories**: Each session gets its own IPC directory in temp
- **Voice-specific files**: Each voice writes to a separate .dat file
- **Periodic polling**: Receivers poll for new data during processDiscovery()
- **Atomic file writes**: Uses JUCE TemporaryFile to prevent corruption

#### VST3 Communication Architecture (Actual Implementation)

The working approach uses **shared memory via InProcessMIDIBus**:

1. **Sender Instance**:
   - Captures MIDI input and distributes to voices
   - Attempts to send via IPCManagerVST3 (legacy compatibility)
   - **Falls back to InProcessMIDIBus::publish()** which writes to shared memory
   - Updates `lastSuccessfulSendTime` for connection status tracking

2. **Receiver Instance**:
   - Subscribes to voice-specific channel via `InProcessMIDIBus::subscribe()`
   - Receives callbacks when data arrives in shared memory
   - Updates `lastSuccessfulReceiveTime` for connection status tracking
   - Outputs MIDI to host

3. **Connection Status**:
   - UI shows "Connected" if either:
     - IPC reports connected (legacy), OR
     - Successful send/receive within last 5 seconds
   - This accurately reflects actual data flow regardless of transport method

4. **Periodic Discovery**:
   - `processBlock()` calls `processDiscovery()` every 100 blocks
   - Prevents "disconnected" status during idle periods
   - Enables quick reconnection when MIDI activity resumes

#### VST3 Implementation Steps

##### Step 1: Define a Custom Data Protocol

Design a clear binary format for the messages you'll be exchanging. Example structure:

```text
Message Type (1 byte) | Channel/Instance ID (1 byte) | MIDI Status Byte (1 byte) | Data 1 (1 byte) | Data 2 (1 byte) | Additional custom data...
```

This could represent things like:

- Note-on/off events
- Control change messages
- Custom parameter states
- Analysis data (frequency content, amplitude envelope, etc.)

##### Step 2: Sending Data (Sender Side)

```cpp
// Pseudo-code example for the sending side
void sendDataToConnectedInstances() {
    // Create a message
    IMessage* message = allocateMessage();

    // Prepare your binary data
    uint8_t data[] = {
        MESSAGE_TYPE_NOTE_ON,
        thisInstanceID,
        0x90,  // Note On status
        60,    // Note number
        100    // Velocity
    };

    // Store the data in the message's attributes
    message->getAttributes()->setBinary("MIDIData", data, sizeof(data));

    // Send to all connected receivers
    for (auto& connection : connectedInstances) {
        connection->notify(message);
    }

    message->release();
}
```

##### Step 3: Receiving Data (Receiver Side)

```cpp
// Pseudo-code example for the receiving side
tresult PLUGIN_API MyProcessor::notify(IMessage* message) {
    const void* data;
    uint32 size;

    // Extract the binary data
    if (message->getAttributes()->getBinary("MIDIData", data, size) == kResultOk) {
        // Parse your custom protocol
        uint8_t* byteData = (uint8_t*)data;
        uint8_t messageType = byteData[0];
        uint8_t senderID = byteData[1];
        uint8_t midiStatus = byteData[2];
        uint8_t midiData1 = byteData[3];
        uint8_t midiData2 = byteData[4];

        // Process the received data
        handleReceivedData(messageType, midiStatus, midiData1, midiData2);
    }

    return kResultOk;
}
```

##### Step 4: Instance Discovery and Connection

Your plugin will need a mechanism to find other instances. Common approaches include:

- Using the host's IConnectionPoint querying capabilities
- Broadcasting availability through shared context
- Manual connection setup via user interface (similar to sidechain routing)

---

## Part 2: AU (Audio Units) Inter-Plugin Communication

### Key Technical Constraints

#### Process Isolation in AU

- Modern macOS DAWs (Logic Pro, GarageBand, etc.) often run each AU plugin in its own separate process for stability and security
- This means in-process communication methods will not work between different plugin instances
- Simple approaches like global variables, singletons, or direct memory pointers are ineffective across process boundaries
- You must implement Inter-Process Communication (IPC) to exchange data between instances

### IPC Methods for Audio Units

#### Method 1: Mach Ports (macOS Native)

- **Description**: Low-level kernel-provided communication ports that are fundamental to macOS inter-process messaging
- **Pros**: Fast, native to the OS, no network stack overhead
- **Cons**: Complex API, requires careful memory management, macOS-specific
- **Best for**: High-performance scenarios where latency is critical

#### Method 2: Shared Memory

- **Description**: A block of memory that multiple processes can access simultaneously
- **Pros**: Extremely fast (memory-speed transfers), good for large data like audio buffers
- **Cons**: Requires synchronization primitives (semaphores, mutexes), risk of race conditions, needs careful cleanup
- **Best for**: High-bandwidth data like FFT analysis results or audio sample data

#### Method 3: XPC Services (macOS Modern)

- **Description**: Apple's recommended high-level IPC framework for process-to-process communication
- **Pros**: Managed lifecycle, built-in security model, async communication patterns, well-documented by Apple
- **Cons**: Higher overhead than Mach ports, asynchronous nature can complicate real-time audio thread use
- **Best for**: General plugin communication where slight latency is acceptable

#### Method 4: Sockets/TCP (Cross-Platform)

- **Description**: Standard network sockets, typically using localhost (127.0.0.1)
- **Pros**: Cross-platform compatible, well-understood programming model, easy to debug
- **Cons**: Higher latency, requires serialization/deserialization, network stack overhead
- **Best for**: When you also need to support other platforms beyond macOS, or for simpler implementation needs

#### Method 5: Juce Framework Features (Recommended for Juce Users)

- **Description**: If using the Juce framework, it provides high-level IPC abstractions
- **Key Classes**:
  - `InterprocessConnection` - Simple socket-based IPC connection
  - `InterprocessConnectionServer` - Server that accepts multiple connections
  - `MemoryBlock` with shared memory utilities
- **Pros**: Handles complex IPC details, works cross-platform, integrates with plugin lifecycle
- **Cons**: Requires using Juce framework, adds library dependency
- **Best for**: Rapid development and cross-platform support

### AU Communication Architecture

The architecture for AU communication differs fundamentally from VST3. Instead of a host-mediated messaging API, you must:

1. **Establish Transport**: Set up a communication channel (Mach port, shared memory, socket, etc.)
2. **Implement Discovery**: Create a mechanism for plugin instances to find each other
3. **Manage Lifecycle**: Handle channel creation when plugins load and cleanup when they unload
4. **Serialize Data**: Convert your internal data structures to a transmittable format (binary, JSON, protocol buffers)
5. **Handle Thread Safety**: Audio threads must communicate carefully with network/messaging threads

### AU Implementation Considerations

#### Process Separation Architecture

```text
┌─────────────────────┐      ┌─────────────────────┐
│   Plugin Instance A │      │   Plugin Instance B │
│   (Process 1)       │      │   (Process 2)       │
│                     │ IPC  │                     │
│  Audio Thread ──┐   │<────>│   ┌── Audio Thread  │
│                 │   │      │   │                 │
│  Message Queue ─┘   │      │   └── Message Queue │
│                     │      │                     │
└─────────────────────┘      └─────────────────────┘
```

#### Thread Safety Pattern

```cpp
// Conceptual example of thread-safe IPC bridge
class IPCManager {
    std::mutex messageQueueMutex;
    std::queue<Message> outgoingQueue;
    std::queue<Message> incomingQueue;

    // Called from audio thread
    void sendToOtherInstances(const Message& msg) {
        std::lock_guard<std::mutex> lock(messageQueueMutex);
        outgoingQueue.push(msg);
    }

    // Called from IPC thread (NOT audio thread)
    void processIPC() {
        std::lock_guard<std::mutex> lock(messageQueueMutex);
        while (!incomingQueue.empty()) {
            Message msg = incomingQueue.front();
            incomingQueue.pop();
            // Forward to audio processing
        }
    }
};
```

#### Recommended IPC Approach by Use Case

| Use Case | Recommended Method | Notes |
| -------- | ------------------ | ----- |
| High-frequency MIDI data | Mach Ports or Shared Memory | Low latency critical for musical timing |
| Bulk analysis data (FFT, etc.) | Shared Memory | Efficient for large data blocks |
| Parameter sync, configuration | XPC Services or Sockets | Easier to implement, slight latency acceptable |
| Cross-platform support | Sockets + Abstracted Interface | Write once, deploy on multiple platforms |
| Rapid prototyping with Juce | Juce InterprocessConnection | Simplest implementation path |

---

## Part 3: Implementation Strategy

### Format-Specific Approach

Since cross-format communication is not required, implement each format independently:

**VST3 Implementation**:
- Use `IConnectionPoint` and `IMessage` interfaces
- Instances communicate directly through the VST3 SDK's built-in messaging
- Works reliably in Cubase and other VST3 hosts

**AU Implementation**:
- Use IPC mechanism (recommended: Sockets or Juce `InterprocessConnection` for simplicity)
- Handles Logic Pro's separate process model
- All AU instances communicate through the chosen IPC channel

### DAW-Specific Usage Instructions

#### Logic Pro Users
- Load all instances of the MIDI Splitter plugin as **AU format only**
- Do not mix AU and VST3 versions in the same project
- Inter-plugin communication will work between all AU instances

#### Cubase Users
- Load all instances of the MIDI Splitter plugin as **VST3 format only**
- Do not mix AU and VST3 versions in the same project
- Inter-plugin communication will work between all VST3 instances

#### Ableton Live Users
- Choose either AU or VST3 format for the first instance
- Load all subsequent instances using the **same format**
- Mixing formats will prevent inter-plugin communication

### User Interface Guidance

Consider adding a format detection indicator in the plugin UI:
- Display current format (AU or VST3)
- Show warning if mixed formats are detected in the session
- Provide clear instructions to users about format consistency requirements

### Common Message Protocol

Both implementations should use the same internal message structure for consistency:

```cpp
// Common message structure used by both VST3 and AU implementations
struct PluginMessage {
    uint32_t magic;           // Protocol identifier
    uint32_t version;         // Protocol version
    uint32_t messageType;     // Type of message
    uint32_t senderID;        // Unique instance identifier
    uint32_t payloadSize;     // Size of payload data
    uint8_t payload[];        // Variable-length data

    enum MessageType {
        MIDI_DATA = 1,
        ANALYSIS_RESULT = 2,
        PARAMETER_SYNC = 3,
        INSTANCE_DISCOVERY = 4,
        HEARTBEAT = 5
    };
};
```

---

## Summary: Key Differences and Recommendations

| Aspect | VST3 | AU |
| ------ | ---- | -- |
| Communication API | IConnectionPoint + IMessage (built-in) | No built-in API (must implement IPC) |
| Process Model | Usually same process (host-dependent) | Often separate processes |
| Discovery | Host-mediated connection query | Must implement custom discovery |
| Complexity | Lower (API provided by SDK) | Higher (must handle IPC details) |
| Latency | Low (in-process) | Varies by IPC method |
| Primary DAW | Cubase, Reaper, FL Studio | Logic Pro, GarageBand |

### Recommended Implementation Strategy

1. **Implement VST3 First**: Use IConnectionPoint (simpler, well-documented)
2. **Implement AU Second**: Use Sockets or Juce InterprocessConnection for easier debugging
3. **Keep Implementations Separate**: No need for cross-format abstraction layer
4. **Test Format Isolation**: Verify each format works independently in its target DAW
5. **Add Format Detection**: Warn users if they mix formats in a session

### What to Avoid

- **Don't implement cross-format communication**: Not needed for this use case
- **Don't rely on host MIDI routing**: Neither format reliably supports this for inter-plugin communication
- **Don't use global variables**: Won't work across AU process boundaries
- **Don't try to convert MIDI to audio and back**: Unnecessarily complex with added latency
- **Don't skip thread safety**: Audio threads and communication threads must be properly synchronized
- **Don't allow format mixing**: Users must use consistent format (all AU or all VST3) per session

### What Works Well

- **VST3**: Use IConnectionPoint + IMessage (official approach) for Cubase users
- **AU**: Use IPC method appropriate for your performance needs for Logic Pro users
- **Both**: Define a custom binary protocol for maximum flexibility
- **Both**: Keep the data transfer efficient with binary formats
- **Format Consistency**: Enforce single-format usage per session for reliable communication

### Resources to Consult

- VST3 SDK Documentation (specifically IConnectionPoint and IMessage interfaces)
- Apple's XPC Services documentation
- Mach Port programming guide (Apple Developer)
- Juce InterprocessConnection documentation
- Steinberg's VST3 examples for component communication (the again sample project)
