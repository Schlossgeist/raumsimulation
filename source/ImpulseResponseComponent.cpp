#include "ImpulseResponseComponent.h"


ImpulseResponseComponent::ImpulseResponseComponent(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts)
    : audioProcessor(p)
    , parameters(pts)
    , thumbnail(128, formatManager, thumbnailCache)
{
    irFileURL = static_cast<const juce::URL>(parameters.state.getProperty("ir_file_url"));

    setOpaque(false);

    thumbnail.addChangeListener(this);
    formatManager.registerFormat(new AiffAudioFormat(), false);
    formatManager.registerFormat(new FlacAudioFormat(), false);
    formatManager.registerFormat(new OggVorbisAudioFormat(), false);
    formatManager.registerFormat(new WavAudioFormat(), false);

    addAndMakeVisible(playPauseButton);
    playPauseButton.onClick = [this] { audioProcessor.playIR(); };

    addAndMakeVisible(irFileLoadButton);
    irFileLoadButton.onClick = [this] { openFile(); };

    addAndMakeVisible(irFileSaveButton);
    irFileSaveButton.onClick = [this] { saveFile(); };

    irFileLabel.attachToComponent(&irFileLoadButton, false);
    irFileLabel.setText(irFileURL.toString(false), sendNotificationAsync);
    irFileLabel.setJustificationType(Justification::centredLeft);

    addAndMakeVisible(irSizeLabel);
    irSizeLabel.setText(juce::String::formatted("%.2f s", thumbnail.getTotalLength()), dontSendNotification);
    irSizeLabel.setJustificationType(Justification::right);

    addAndMakeVisible(clearButton);
    clearButton.onClick = [this] { audioProcessor.clearIR();
                                   irFileURL = {};
                                   updateThumbnail(audioProcessor.globalSampleRate); };

    setURL(irFileURL); // has to come after registering the audio formats
}

ImpulseResponseComponent::~ImpulseResponseComponent()
{
    thumbnail.removeChangeListener(this);
}

void ImpulseResponseComponent::paint(juce::Graphics & g)
{
    auto thumbArea = getLocalBounds().reduced(5);
    thumbArea.removeFromBottom(50);

    if (thumbnail.getTotalLength() > 0.0) {
        double ms = 0.0f;
        double numberOfLines = (int) parameters.state.getProperty("lines_in_waveform");;

        double pixelPerMS = thumbArea.getWidth() / (thumbnail.getTotalLength() * 1000.0f);
        double stepMS = thumbnail.getTotalLength() * 1000.0f / numberOfLines;
        double roundedStep = floor((stepMS + 5.0f) / 10.0f) * 10.0f;
        while (ms < thumbnail.getTotalLength() * 1000.0f) {
            g.setColour(getLookAndFeel().findColour(0x0000002));
            g.setFont(8.0f);

            if (ms < 1000.0f) {
                g.drawText(String(ms) + " ms", thumbArea.getX() + (int) (pixelPerMS * ms) + 3, thumbArea.getY(), 25.0f, 10.0f, Justification::centredLeft, false);
            } else {
                g.drawText(String(ms / 1000.0f) + " s", thumbArea.getX() + (int) (pixelPerMS * ms) + 3, thumbArea.getY(), 25.0f, 10.0f, Justification::centredLeft, false);
            }

            g.setColour(getLookAndFeel().findColour(0x0000001));
            g.fillRect((float) (thumbArea.getX() + pixelPerMS * ms),(float) thumbArea.getY(), 1.0f,(float) thumbArea.getHeight());

            ms += roundedStep;
        }

        g.setColour(getLookAndFeel().findColour(0x0000000));
        thumbnail.drawChannels(g, thumbArea, 0, thumbnail.getTotalLength(), 1.0f);
    } else {
        g.setColour(getLookAndFeel().findColour(TextButton::textColourOffId));
        g.setFont(14.0f);
        g.drawFittedText("No audio file selected", thumbArea, Justification::centred, 2);
    }
}

void ImpulseResponseComponent::resized()
{
    auto area = getLocalBounds();

    auto bottom = area.removeFromBottom(75);

    {   // Buttons
        irSizeLabel.                setBounds(bottom.removeFromTop(25));

        auto buttons = bottom.removeFromBottom(50);

        playPauseButton.            setBounds(buttons.removeFromLeft(100).reduced(5));
        clearButton.                setBounds(buttons.removeFromRight(100).reduced(5));

        irFileLoadButton.           setBounds(buttons.removeFromLeft(buttons.getWidth()/2).reduced(5));
        irFileSaveButton.           setBounds(buttons.reduced(5));
    }
}

void ImpulseResponseComponent::setURL(const URL& url)
{
    if (url.isLocalFile())
    {
        auto source = std::make_unique<FileInputSource>(irFileURL.getLocalFile());

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
    const MessageManagerLock messageManagerLock;

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

void ImpulseResponseComponent::saveFile()
{
    if (irFileChooser != nullptr)
        return;

    irFileChooser.reset(new FileChooser("Select an audio file...", File(), "*.wav"));

    irFileChooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting,
            [this] (const FileChooser& fc) mutable
            {
                if (fc.getURLResults().size() > 0)
                {
                    auto result = fc.getURLResult();

                    auto file = File(result.getLocalFile().getFullPathName());

                    if (file.exists()) {
                        file.moveToTrash();
                    }

                    WavAudioFormat format;
                    std::unique_ptr<AudioFormatWriter> writer;
                    writer.reset(format.createWriterFor(new FileOutputStream (file),
                                                        audioProcessor.globalSampleRate,
                                                        audioProcessor.ir.getNumChannels(),
                                                        24,
                                                        {},
                                                        0));
                    if (writer != nullptr)
                        writer->writeFromAudioSampleBuffer (audioProcessor.ir, 0, audioProcessor.ir.getNumSamples());
                }

                irFileChooser = nullptr;
            }, nullptr);
}

void ImpulseResponseComponent::changeListenerCallback (ChangeBroadcaster*)
{
    repaint();
}

void ImpulseResponseComponent::mouseMove(const MouseEvent &e)
{
    auto thumbArea = getLocalBounds().reduced(5);
    double msPerPixel = (thumbnail.getTotalLength() * 1000.0f) / thumbArea.getWidth();
    double ms = (e.getMouseDownX() - 5) * msPerPixel;

    if (e.getMouseDownX() > 5) {
        if (ms < 1000.0f) {
            setTooltip(juce::String::formatted("%.2f ms", ms, dontSendNotification));
        } else {
            setTooltip(juce::String::formatted("%.2f s", ms / 1000.0f, dontSendNotification));
        }
    }
}
