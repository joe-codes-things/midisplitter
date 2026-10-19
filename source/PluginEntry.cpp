#include "MIDISplitterProcessor.h"

/**
 * This function is the entry point for JUCE plugins.
 * It creates and returns a new instance of the AudioProcessor.
 */
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MIDISplitterProcessor();
}
