#include "ImpulseResponseComponent.h"


ImpulseResponseComponent::ImpulseResponseComponent(juce::AudioProcessorValueTreeState& pts)
    : parameters(pts)
    , thumbnail(256, formatManager, thumbnailCache)
{
    irFileURL = static_cast<const juce::URL>(parameters.state.getProperty("ir_file_url"));

    setOpaque(false);

    thumbnail.addChangeListener(this);
    formatManager.registerFormat(new AiffAudioFormat(), false);
    formatManager.registerFormat(new FlacAudioFormat(), false);
    formatManager.registerFormat(new OggVorbisAudioFormat(), false);
    formatManager.registerFormat(new WavAudioFormat(), false);

    setURL(irFileURL); // has to come after registering the audio formats

    addAndMakeVisible(irFileLoadButton);
    irFileLoadButton.setColour(TextButton::buttonColourId, Colour (0xff797fed));
    irFileLoadButton.setColour(TextButton::textColourOffId, Colours::black);
    irFileLoadButton.onClick = [this] { openFile(); };

    irFileLabel.attachToComponent(&irFileLoadButton, false);
    irFileLabel.setText(irFileURL.toString(false), sendNotificationAsync);
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

    irFileLoadButton.setBounds(area.removeFromBottom(25));
}

void ImpulseResponseComponent::setURL(const URL& url)
{
    if (url.isLocalFile())
    {
        irFileLabel.setText(url.toString(false), sendNotificationAsync);

        thumbnail.setSource(new FileInputSource(irFileURL.getLocalFile()));
    }
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
                }

                irFileChooser = nullptr;
            }, nullptr);
}

void ImpulseResponseComponent::changeListenerCallback (ChangeBroadcaster*)
{
    repaint();
}
