#include "ImpulseResponseComponent.h"


ImpulseResponseComponent::ImpulseResponseComponent(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts)
    : audioProcessor(p)
    , parameters(pts)
    , thumbnail(256, formatManager, thumbnailCache)
{
    irFileURL = static_cast<const juce::URL>(parameters.state.getProperty("ir_file_url"));

    setOpaque(false);

    thumbnail.addChangeListener(this);
    formatManager.registerFormat(new AiffAudioFormat(), false);
    formatManager.registerFormat(new FlacAudioFormat(), false);
    formatManager.registerFormat(new OggVorbisAudioFormat(), false);
    formatManager.registerFormat(new WavAudioFormat(), false);

    addAndMakeVisible(irFileLoadButton);
    irFileLoadButton.setColour(TextButton::buttonColourId, Colour (0xff797fed));
    irFileLoadButton.setColour(TextButton::textColourOffId, Colours::black);
    irFileLoadButton.onClick = [this] { openFile(); };

    irFileLabel.attachToComponent(&irFileLoadButton, false);
    irFileLabel.setText(irFileURL.toString(false), sendNotificationAsync);

    addAndMakeVisible(irSizeLabel);
    irSizeLabel.setText(juce::String::formatted("%.2f s", thumbnail.getTotalLength()), dontSendNotification);
    irSizeLabel.setJustificationType(Justification::right);

    addAndMakeVisible(playPauseButton);
    playPauseButton.setButtonText("Play");
    playPauseButton.setColour(TextButton::buttonColourId, Colour(0xff009900));
    playPauseButton.setColour(TextButton::textColourOffId, Colours::black);
    playPauseButton.onClick = [this] { audioProcessor.playIR(); };

    setURL(irFileURL); // has to come after registering the audio formats
}

ImpulseResponseComponent::~ImpulseResponseComponent()
{
    thumbnail.removeChangeListener(this);
}

void ImpulseResponseComponent::paint(juce::Graphics & g)
{
    if (thumbnail.getTotalLength() > 0.0) {
        auto thumbArea = getLocalBounds().reduced(5);
        thumbArea.removeFromBottom(50);

        thumbnail.drawChannels(g, thumbArea, 0, thumbnail.getTotalLength(), 1.0f);
    } else {
        g.setFont(14.0f);
        g.drawFittedText("No audio file selected", getLocalBounds(), Justification::centred, 2);
    }
}

void ImpulseResponseComponent::resized()
{
    auto area = getLocalBounds().reduced(5);
    auto bottom = area.removeFromBottom(50);

    auto buttons = bottom.removeFromBottom(25);

    playPauseButton.setBounds(buttons.removeFromLeft(100));
    buttons.removeFromLeft(5);      // little space between buttons
    irFileLoadButton.setBounds(buttons);
    irSizeLabel.setBounds(bottom.removeFromRight(100));
}

void ImpulseResponseComponent::setURL(const URL& url)
{
    if (url.isLocalFile())
    {
        auto source = new FileInputSource(irFileURL.getLocalFile());

        if (source != nullptr) {


            auto stream = rawToUniquePtr(source->createInputStream());

            if (stream != nullptr) {
                auto reader = rawToUniquePtr(formatManager.createReaderFor(std::move(stream)));

                if (reader != nullptr) {
                    audioProcessor.ir.setSize((int) reader->numChannels, (int) reader->lengthInSamples);
                    reader->read(&audioProcessor.ir, 0, audioProcessor.ir.getNumSamples(), 0, true, true);

                    updateThumbnail(reader->sampleRate);
                }
            }
        }
    }
}

void ImpulseResponseComponent::updateThumbnail(double sampleRate)
{
    thumbnail.setSource(&audioProcessor.ir, sampleRate, Uuid().hash());
    irFileLabel.setText(irFileURL.toString(false), sendNotificationAsync);
    irSizeLabel.setText(juce::String::formatted("%.2f s", thumbnail.getTotalLength()), dontSendNotification);
}

void ImpulseResponseComponent::openFile()
{
    if (irFileChooser != nullptr)
        return;

    irFileChooser.reset(new FileChooser("Select an audio file...", File(), "*.aif,*.flac,*.mp3,*.ogg,*.wav"));

    irFileChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
            [this] (const FileChooser& fc) mutable
            {
                if (fc.getURLResults().size() > 0)
                {
                    auto result = fc.getURLResult();
                    irFileURL = result;
                    parameters.state.setProperty("ir_file_url", result.toString(false), nullptr);

                    setURL(result);
                    audioProcessor.updateParameters();
                }

                irFileChooser = nullptr;
            }, nullptr);
}

void ImpulseResponseComponent::changeListenerCallback (ChangeBroadcaster*)
{
    repaint();
}
