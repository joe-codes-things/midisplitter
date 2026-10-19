# Sibelius Explode and Reduce Functions - Comprehensive Implementation Guide

## Table of Contents
1. [Overview](#overview)
2. [ManuScript Language Basics](#manuscript-language-basics)
3. [Explode Function Deep Dive](#explode-function-deep-dive)
4. [Reduce Function Deep Dive](#reduce-function-deep-dive)
5. [Shared Utility Functions](#shared-utility-functions)
6. [Key Algorithms and Data Structures](#key-algorithms-and-data-structures)
7. [Implementation Patterns](#implementation-patterns)

---

## Overview

The Sibelius Explode and Reduce functions are sophisticated music notation manipulation tools written in **ManuScript**, Sibelius's proprietary plugin scripting language. These functions handle the complex task of distributing music across multiple staves (Explode) or combining multiple staves into one (Reduce).

**Source Files:**
- `Explode.plg` - Located in Sibelius.app/Contents/Resources/en.lproj/Plugins/Composing Tools/
- `Reduce.plg` - Located in Sibelius.app/Contents/Resources/en.lproj/Plugins/Composing Tools/

**Key Concepts:**
- **Voice**: Sibelius uses 4 voices per staff (numbered 1-4), plus voice 0 for objects not tied to a specific voice
- **Position**: Internal time units (likely 256ths of a quarter note) used to track where objects occur in a bar
- **Duration**: How long an object lasts, also in internal time units
- **Staff/Bar Navigation**: Access to score structure through NthStaff() and NthBar() methods

---

## ManuScript Language Basics

### Language Characteristics

ManuScript is a Pascal-like scripting language with these features:

**Variable Declaration:**
```manuscript
_variable = "value"
counter = 0
flag = True
```

**Arrays:**
```manuscript
myarray = CreateArray();
myarray[0] = 'value1';
myarray[1] = 'value2';
```

**Control Structures:**
```manuscript
if (condition = True)
{
    // code
}

while (counter < 10)
{
    counter = counter + 1;
}

for each object in collection
{
    // process object
}
```

**Dynamic Variable Names:**
```manuscript
temp = 'variable' & counter;
@temp = value;  // Sets the variable named 'variable1', 'variable2', etc.
```

**Object Access:**
```manuscript
score = Sibelius.ActiveScore;
staff = score.NthStaff(staffnumber);
bar = staff.NthBar(barnumber);
```


---

## Explode Function Deep Dive

### Purpose
Takes a single staff with multiple voices and distributes the music across 2-4 separate destination staves (parts).

### Main Algorithm Flow

1. **Initialization & Validation**
   - Check that exactly one staff is selected
   - Get user preferences (destination, options)
   - Determine number of parts (2-4)

2. **Voice Analysis (per bar)**
   ```manuscript
   notearr[1-4]  // Count notes in each voice
   restarr[1-4]  // Count rests in each voice
   oldarr[1-4]   // Track previous state
   ```

3. **Part Assignment Logic**
   - Calculate `ratio1 = totalnotes / _parts` (notes per part)
   - Calculate `ratio2 = _parts / totalnotes` (parts per note)
   - Handle three scenarios:
     - **More notes than parts**: Extra notes go to specified part
     - **Fewer notes than parts**: Duplicate specified part
     - **Equal distribution**: One-to-one voice-to-part mapping

4. **Text Recognition for Part Assignment**
   - Scans for technique text like "a1", "1.", "2.", "a2"
   - `CheckText()` function parses these markings
   - Determines which parts should receive notes based on orchestral conventions

5. **Note Distribution**
   ```manuscript
   CopyOtherVoice(oldvoice, newvoice, oldstaff, newstaff, oldbar, newbar, x, y)
   ```
   - Copies notes from source voice to destination staff
   - Preserves pitch, duration, articulations, ties

6. **Object Copying**
   - Copies all associated musical objects
   - Uses `CopyVoiceCheckDupe()` to avoid duplicates
   - Handles: text, lines, symbols, lyrics, clefs, instrument changes

7. **Post-Processing**
   - `FilterForPart()`: Removes notes not meant for this part
   - `TidyUpRests()`: Combines small rests, cleans up formatting
   - Adds cue markings if enabled
   - Adds "Play" text when cue ends

### Key Features

**Cue Passage Creation:**
- Creates cue-sized notes in non-playing parts
- Optionally adds bar rest in voice 2
- Optionally adds "Play" text when solo resumes

**Doubling Logic:**
- When fewer notes than parts, can duplicate a specified part
- Useful for unison passages in orchestral scores

**Overwrite Protection:**
- Option to preserve existing music in destination staves
- Warns user if data will be overwritten

---

## Reduce Function Deep Dive

### Purpose
Combines 2-4 separate staves into a single staff with up to 4 voices.

### Main Algorithm Flow

1. **Initialization & Validation**
   - Check that 2-4 staves are selected
   - Get user preferences (mode, options)
   - Determine destination staff

2. **Voice Assignment**

   **Mode 1: Separate All Parts**
   ```manuscript
   if (_impsep = True)
   {
       Staff 1 → Voice 1
       Staff 2 → Voice 2
       Staff 3 → Voice 3
       Staff 4 → Voice 4
   }
   ```

   **Mode 2: Minimum Voices**
   ```manuscript
   if (_impmin = True)
   {
       // Analyze rhythmic independence
       // Combine parts that can share voices
       // Optimize to use fewest voices
   }
   ```

3. **Note Copying (per bar)**
   - Copy notes from each source staff to assigned voice
   - Preserve all note properties
   - Handle tied notes across voices

4. **Duplicate Note Handling**

   **Show Duplicates:**
   ```manuscript
   if (_impshowdupe = True)
   {
       // Keep duplicate pitches as chord notes
       // Useful for seeing all parts
   }
   ```

   **Delete Duplicates:**
   ```manuscript
   DeleteDupes(staff, bar, first, last)
   {
       // Remove duplicate pitches from chords
       // Cleaner visual appearance
   }
   ```

5. **Solo/Divisi Marking**
   ```manuscript
   if (_solotype != "Nothing")
   {
       // Add text: "1 a2", "1. a2.", "1° a2", etc.
       // Marks when parts play in unison
       text = bar.AddText(position, _solotype, 'text.staff.technique');
   }
   ```

6. **Rest Management**
   ```manuscript
   HideRests(staff, bar, first, last)
   {
       // Hide redundant rests when multiple voices rest
       // Voice 2+ rests hidden if Voice 1 has rest at same position
   }
   ```

7. **Accidental Management**
   ```manuscript
   Accidental(staff, bar, first, last)
   {
       // Check previous notes in same voice
       // Check simultaneous notes in other voices
       // Force accidental if pitch differs
       note.AccidentalStyle = 2;  // Force show
   }
   ```

---

## Shared Utility Functions

### CopyVoiceCheckDupe

The core function for copying musical objects while avoiding duplicates.

**Signature:**
```manuscript
CopyVoiceCheckDupe(oldvoice, newvoice, oldstaffnum, newstaffnum, oldbarnum, newbarnum, first, last)
```

**Process:**
1. Select source passage
2. Cache all objects in temporary array (using dynamic variable names)
3. For each cached object:
   - Check if it matches voice criteria
   - Check for duplicates in destination
   - Copy if no duplicate found

**Duplicate Detection:**
```manuscript
_impduperange = 1;  // Tolerance in internal units

for each existing object in destination
{
    if ((newObj.Position < (oldObj.Position + _impduperange)) AND
        (newObj.Position > (oldObj.Position - _impduperange)) AND
        (newObj.StyleId = oldObj.StyleId))
    {
        dupe = True;
    }
}
```

### Object Types Handled

Both functions copy these object types:

1. **NoteRest**: Notes and rests with all properties
2. **Text**: All text styles (technique, expression, tempo, rehearsal marks)
3. **LyricItem**: Lyrics with syllable types and positioning
4. **Lines**:
   - Slur
   - Crescendo/Diminuendo
   - Glissando
   - Trill
   - Octava
   - Pedal
   - Arpeggio
5. **Clef**: Clef changes (concert and transposing)
6. **SymbolItem**: Musical symbols (fermatas, accents, etc.)
7. **InstrumentChange**: Instrument change markers

### Object Copying Pattern

For each object type, the pattern is:

```manuscript
if (o.Type = 'Text')
{
    dupe = False;
    for each Text p in newbar
    {
        if ((p.Position ≈ o.Position) AND
            (p.StyleId = o.StyleId) AND
            (p.Text = o.Text))
        {
            dupe = True;
        }
    }
    if (dupe = False)
    {
        text = newbar.AddText(o.Position, o.Text, o.StyleId);
        text.Dx = o.Dx;  // Preserve horizontal offset
        text.Dy = o.Dy;  // Preserve vertical offset
        text.Hidden = o.Hidden;
    }
}
```

---

## Key Algorithms and Data Structures

### Voice System

Sibelius uses a 4-voice polyphonic system:

- **Voice 1**: Typically highest notes, stems up
- **Voice 2**: Second voice, stems down
- **Voice 3**: Third voice, stems up
- **Voice 4**: Fourth voice, stems down
- **Voice 0**: Objects not tied to specific voice (clefs, key signatures, barlines)

### Position and Duration

- **Position**: Internal time units (likely 256ths of a quarter note)
- **Duration**: Length of object in same units
- **Bar**: Container for all objects at a specific measure
- **Staff**: Container for all bars in a single staff line

### Part-to-Voice Mapping (Explode)

```manuscript
partvoice[1-4]  // Maps which voice each part should use

// Example: If partvoice[1] = 2, then Part 1 gets notes from Voice 2
```

### Ratio-Based Distribution

```manuscript
ratio1 = totalnotes / _parts;  // Notes per part

if (ratio1 = RoundDown(ratio1))
{
    // Even distribution possible
    notecount = ratio1;
}
else
{
    // Uneven - need extra note handling
    extra = totalnotes - (_parts * RoundDown(ratio1));
}
```

---

## Implementation Patterns

### 1. Dynamic Variable Assignment

ManuScript's `@` operator enables dynamic variable names:

```manuscript
count = 1;
for each note in chord
{
    temp = 'note' & count;  // Creates "note1", "note2", etc.
    @temp = note;           // Assigns to dynamically named variable
    count++;
}

// Later retrieval:
checknote = @'note1';
```

### 2. Object Caching to Avoid Iterator Invalidation

```manuscript
// Cache objects first
objcount = 0;
for each o in selection
{
    temp = 'obj' & objcount;
    @temp = o;
    objcount++;
}

// Process cached objects
q = 0;
while (q < objcount)
{
    temp = 'obj' & q;
    o = @temp;
    // Safe to modify/delete
    q++;
}
```

### 3. Selection Management

```manuscript
score = Sibelius.ActiveScore;
selection = score.Selection;

// Select a passage
selection.SelectPassage(startBar, endBar, topStaff, bottomStaff, startPos, endPos);

// Copy/paste with clipboard slot
selection.Copy(9);   // Slot 9 is temporary clipboard
selection.Paste(9);

// Clear selection
selection.Clear();
```

### 4. Progress Indication

```manuscript
Sibelius.CreateProgressDialog("Exploding...", 0, lastbar - firstbar);

x = firstbar;
while (x <= lastbar)
{
    Sibelius.UpdateProgressDialog(x - firstbar, "Processing bar " & x);
    // Process bar
    x++;
}

Sibelius.DestroyProgressDialog();
```

### 5. Dialog Creation

```manuscript
Settings "Dialog"
{
    Title "Explode Settings"
    X "194"
    Y "149"
    Width "252"
    Height "235"
    Controls
    {
        CheckBox
        {
            Title "Overwrite existing music"
            Value "_overwrite"
            ID "IDC_OVERWRITE_CHECK"
        }

        ComboBox
        {
            ListVar "_comboparts"
            Value "_extra"
            ID "IDC_EXTRA_COMBO"
        }

        Button
        {
            Title "OK"
            DefaultButton "1"
            EndDialog "1"
        }
    }
}

// Show dialog
if (Sibelius.ShowDialog(Settings, self) = False)
    return false;
```

### 6. Preference Storage

```manuscript
// Save preferences
prefOpen = Preferences.Open('explode', '020000');
Preferences.SetKey('_parts', _parts);
Preferences.SetKey('_create', _create);
Preferences.Close('explode', '020000');

// Load preferences
prefOpen = Preferences.Open('explode', '020000');
_parts = Preferences.GetKey('_parts');
_create = Preferences.GetKey('_create');
```

### 7. Error Handling and Validation

```manuscript
// Validation checks
if (Sibelius.ScoreCount = 0)
{
    Sibelius.MessageBox("There are no scores open");
    return False;
}

if (selection.StavesCount != 1)
{
    Sibelius.MessageBox("Please copy a single staff to be Exploded");
    return False;
}

// Confirmation dialogs
result = Sibelius.YesNoMessageBox("Data will be overwritten. Continue?");
if (result = False)
    return False;
```

---

## Advanced Techniques

### 1. Text-Based Part Assignment

The `CheckText()` function recognizes orchestral notation conventions:

```manuscript
CheckText(text, voice, voicenum, dy)
{
    // Recognizes: "a1", "1.", "2.", "a2", "1. 2.", etc.
    // Returns: which part number should receive the notes
    // Handles: solo markings, divisi markings, unison markings
}
```

**Examples:**
- "a1" or "1." = Part 1 only
- "a2" or "2." = Part 2 only
- "a2" (in context) = Parts 1 and 2 in unison
- No marking = All parts (tutti)

### 2. Accidental Intelligence

The Reduce function has sophisticated accidental logic:

```manuscript
for each note in current chord
{
    // Look back in same voice
    lastPitch = FindPreviousNoteInVoice(note.DiatonicPitch, voice);

    // Check simultaneous notes in other voices
    simultaneousPitch = FindSimultaneousNote(note.DiatonicPitch, otherVoices);

    // Force accidental if:
    // 1. Different pitch than previous in same voice
    // 2. Different pitch than simultaneous in other voice
    if (needsAccidental)
        note.AccidentalStyle = 2;  // Force show
}
```

### 3. Rest Optimization

```manuscript
TidyUpRests(staff, bar, position, last)
{
    // Combine small rests into larger rests
    // Remove unnecessary rests
    // Ensure proper rest positioning
    // Handle multi-bar rests
}
```

### 4. Voice Filtering

```manuscript
FilterForPart(partcount, part, staff, bar, position, y, sourcestaff, sourcebar, voice)
{
    // Keep only notes meant for this part
    // Delete notes meant for other parts
    // Handle solo vs. tutti markings
    // Preserve part-specific articulations
}
```

---

## Common Pitfalls and Solutions

### 1. Voice Number Confusion

**Problem:** Voice 0 represents "no specific voice" (clefs, key signatures, etc.)

**Solution:** Always check for both specific voice and voice 0:
```manuscript
if ((o.VoiceNumber = targetVoice) OR (o.VoiceNumber = 0))
    // Process object
```

### 2. Position Precision

**Problem:** Floating-point position comparisons can fail due to rounding

**Solution:** Use tolerance range for position matching:
```manuscript
tolerance = 1;
if (ABS(pos1 - pos2) < tolerance)
    // Positions match
```

### 3. Object Deletion During Iteration

**Problem:** Deleting objects while iterating causes iterator invalidation and skips

**Solution:** Cache objects first, then delete:
```manuscript
// Cache objects to delete
delcount = 0;
for each object
{
    if (shouldDelete)
    {
        @('del' & delcount) = object;
        delcount++;
    }
}

// Delete cached objects in reverse order
while (delcount > 0)
{
    delcount--;
    @('del' & delcount).Delete();
}
```

### 4. Tied Notes

**Problem:** Tied notes need special handling to preserve ties

**Solution:** Check tie status and preserve:
```manuscript
if (note.Tied = True)
{
    newNote = AddNote(pitch, True, diatonicPitch);  // Preserve tie
}
```

### 5. Line Duration

**Problem:** Lines (slurs, crescendos) can span multiple bars

**Solution:** Check if line extends beyond current bar and handle accordingly:
```manuscript
if ((dupe = False) and (o.Duration > duration))
{
    // Only copy if this line is longer than existing
    line = newbar.AddLine(o.Position, o.Duration, o.StyleId, o.Dx, o.Dy, voice, o.Hidden);
}
```

---

## Performance Considerations

### 1. Bar-by-Bar Processing

Process one bar at a time to manage memory:

```manuscript
x = firstbar;
while (x <= lastbar)
{
    bar = staff.NthBar(x);
    // Process this bar completely
    // Before moving to next bar
    x++;
}
```

### 2. Duplicate Detection Optimization

Use position-based early exit:

```manuscript
// Only check objects near the target position
if ((p.Position < (o.Position + range)) AND
    (p.Position > (o.Position - range)))
{
    // Detailed comparison only for nearby objects
    if (p.StyleId = o.StyleId AND p.Text = o.Text)
        dupe = True;
}
```

### 3. Object Caching

Cache objects to avoid repeated lookups:

```manuscript
// Cache frequently accessed objects
score = Sibelius.ActiveScore;
staff = score.NthStaff(staffnum);
bar = staff.NthBar(barnum);

// Use cached references
for each o in bar
{
    // Process using cached bar reference
}
```

---

## Workflow Diagrams

### Explode Workflow

```
┌─────────────────────────────────────┐
│  User selects source staff          │
│  and destination configuration      │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  For each bar in selection:         │
│  1. Analyze voices (count notes)    │
│  2. Check for part assignment text  │
│  3. Calculate note distribution     │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Determine part-to-voice mapping    │
│  - Handle extra notes               │
│  - Handle doubling                  │
│  - Handle cue passages              │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  For each destination part:         │
│  1. Copy assigned notes             │
│  2. Copy associated objects         │
│  3. Filter for this part            │
│  4. Tidy up rests                   │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Post-processing:                   │
│  - Add cue markings                 │
│  - Add "Play" text                  │
│  - Clean up formatting              │
└─────────────────────────────────────┘
```

### Reduce Workflow

```
┌─────────────────────────────────────┐
│  User selects source staves (2-4)   │
│  and reduction mode                 │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Assign voices to source staves     │
│  - Separate mode: 1:1 mapping       │
│  - Minimum mode: optimize voices    │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  For each bar in selection:         │
│  For each source staff:             │
│    Copy notes to assigned voice     │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Handle duplicates:                 │
│  - Show or delete duplicate notes   │
│  - Mark solo/divisi passages        │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Copy associated objects:           │
│  - Text, lines, symbols             │
│  - Check for duplicates             │
│  - Preserve formatting              │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Post-processing:                   │
│  - Hide redundant rests             │
│  - Manage accidentals               │
│  - Clean up bar rests               │
└─────────────────────────────────────┘
```

---

## Conclusion

The Sibelius Explode and Reduce functions demonstrate sophisticated music notation algorithms that handle:

1. **Voice Management**: Intelligent mapping between voices and parts
2. **Object Handling**: Comprehensive copying of all musical elements
3. **Duplicate Detection**: Position-based tolerance system
4. **Orchestral Conventions**: Recognition of standard notation practices (a1, 1., etc.)
5. **User Flexibility**: Multiple modes and options for different use cases
6. **Musical Intelligence**: Accidental management, rest optimization, tie preservation

### Key Takeaways for Implementation

- **Use position-based duplicate detection** with tolerance ranges
- **Cache objects before modification** to avoid iterator invalidation
- **Process bar-by-bar** for memory efficiency
- **Handle all object types** (notes, text, lines, symbols, lyrics, clefs)
- **Preserve musical context** (ties, accidentals, voice assignments)
- **Provide user feedback** (progress dialogs, validation messages)
- **Store preferences** for consistent user experience

### Adaptability

These patterns and algorithms can be adapted to:
- Other music notation software (MuseScore, Finale, Dorico)
- MIDI manipulation tools
- Music XML processing
- Custom notation systems
- Automated arranging tools

The ManuScript language, while proprietary to Sibelius, demonstrates clear patterns that translate well to other programming environments. The core algorithms (voice distribution, duplicate detection, object copying) are universal concepts in music notation software development.

---

## Additional Resources

- **ManuScript Language Reference**: Available in Sibelius Help menu (Help > Plug-ins > ManuScript Language)
- **Plugin Development**: File > Plug-ins > Edit Plug-ins (opens the plugin editor)
- **Sibelius Object Model**: Documented in the ManuScript language guide
- **Community Plugins**: Available on Sibelius user forums and plugin library
- **Source Files**: Located in this project's `Sibelius-code/` folder

---

*This guide was created by analyzing the actual source code of Sibelius's Explode.plg and Reduce.plg files. All code examples are derived from the original ManuScript implementations.*
