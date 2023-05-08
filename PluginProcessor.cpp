/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RaumsimulationAudioProcessor::RaumsimulationAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
     , parameters(*this, nullptr, juce::Identifier("Parameters"),
                 {
                         std::make_unique<juce::AudioParameterFloat>("gain", "Gain", -24.0f, 24.0f, 0.0f)
                 })
#endif
{
    parameters.state.appendChild(settings, nullptr);
    gainParameter = parameters.getRawParameterValue("gain");
}

RaumsimulationAudioProcessor::~RaumsimulationAudioProcessor()
{
}

//==============================================================================
const juce::String RaumsimulationAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RaumsimulationAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RaumsimulationAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RaumsimulationAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RaumsimulationAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RaumsimulationAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RaumsimulationAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RaumsimulationAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String RaumsimulationAudioProcessor::getProgramName(int index)
{
    return {};
}

void RaumsimulationAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void RaumsimulationAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    reset();

    const auto channels = jmax(getTotalNumInputChannels(), getTotalNumOutputChannels());

    if (channels == 0)
        return;

    juce::dsp::ProcessSpec processSpec{sampleRate, (uint32) samplesPerBlock, (uint32) channels};

    convolution.loadImpulseResponse(std::move(ir), processSpec.sampleRate, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::yes, juce::dsp::Convolution::Normalise::no);
    convolution.prepare(processSpec);

    gain.setGainDecibels(*gainParameter);
    gain.prepare(processSpec);

    globalSampleRate = sampleRate;

    irBufferPosition = 0;
}

void RaumsimulationAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RaumsimulationAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void RaumsimulationAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (jmax (getTotalNumInputChannels(), getTotalNumOutputChannels()) == 0)
        return;

    ScopedNoDenormals noDenormals;

    setLatencySamples(convolution.getLatency());

    const auto numChannels = jmax(getTotalNumInputChannels(), getTotalNumOutputChannels());
    const auto numSamples = buffer.getNumSamples();

    auto inoutBlock = dsp::AudioBlock<float>(buffer).getSubsetChannelBlock(0, (size_t) numChannels);
    juce::dsp::ProcessContextReplacing<float> processContext{inoutBlock};

    convolution.process(processContext);

    if (play && ir.getNumSamples() > 0) {

        auto lengthInSecs = ir.getNumSamples()/globalSampleRate;

        auto* writePtrArray = buffer.getArrayOfWritePointers();
        auto* readPtrArray = ir.getArrayOfReadPointers();

        for (auto sample = 0; sample < numSamples; ++sample) {
            for (auto channel = 0; channel < numChannels; ++channel) {
                writePtrArray[channel][sample] += + readPtrArray[channel][irBufferPosition];
            }
            irBufferPosition++;

            if (irBufferPosition >= ir.getNumSamples() - 1) {
                irBufferPosition = 0;
                play = false;
                break;
            }
        }
    }

    gain.process(processContext);
}

//==============================================================================
bool RaumsimulationAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* RaumsimulationAudioProcessor::createEditor()
{
    return new RaumsimulationAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void RaumsimulationAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void RaumsimulationAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState = getXmlFromBinary(data, sizeInBytes);

    if (xmlState != nullptr)
        if (xmlState->hasTagName(parameters.state.getType())) {
            parameters.replaceState(juce::ValueTree::fromXml (*xmlState));
        }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RaumsimulationAudioProcessor();
}

void RaumsimulationAudioProcessor::updateParameters()
{
    auto const irFileURL = static_cast<const juce::URL>(parameters.state.getProperty("ir_file_url"));
    convolution.loadImpulseResponse(irFileURL.getLocalFile(), juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::yes, 0);

    gain.setGainDecibels(*gainParameter);
}

void RaumsimulationAudioProcessor::reset()
{
    convolution.reset();
    gain.reset();
}

void RaumsimulationAudioProcessor::playIR()
{
    irBufferPosition = 0;
    play = true;
}
