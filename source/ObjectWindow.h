#pragma once

#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "Raytracer.h"

class ObjectWindow : public juce::DocumentWindow
{
public:
    ObjectWindow(RaumsimulationAudioProcessor&, juce::AudioProcessorValueTreeState&, Raytracer&, const String& name, Colour backgroundColour, int requiredButtons, bool addToDesktop);
    ~ObjectWindow();

    enum Mode {
        ADD,
        EDIT
    };

    void openWindow(Mode mode);

private:
    class ObjectComponent : public juce::Component
    {
    public:
        explicit ObjectComponent(ObjectWindow& pw)
        : parentWindow(pw)
        {
            setSize(400, 400);

            addAndMakeVisible(generalLabel);
            generalLabel.setFont(juce::Font(16.0f, juce::Font::bold));

            addAndMakeVisible(objectMenu);
            objectMenu.onChange = [this] { parentWindow.currentObjectName = objectMenu.getText();
                                                  populateObjectProperties(); };

            auto normalDelete = Drawable::createFromImageData(BinaryData::delete_FILL0_wght100_GRAD25_opsz48_svg, BinaryData::delete_FILL0_wght100_GRAD25_opsz48_svgSize);
            auto overDelete   = Drawable::createFromImageData(BinaryData::delete_FILL0_wght100_GRAD200_opsz48_svg, BinaryData::delete_FILL0_wght100_GRAD200_opsz48_svgSize);
            auto downDelete   = Drawable::createFromImageData(BinaryData::delete_FILL0_wght100_GRAD200_opsz48_svg, BinaryData::delete_FILL0_wght100_GRAD200_opsz48_svgSize);

            normalDelete->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOffId));
            overDelete->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOffId));
            downDelete->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOnId));

            addAndMakeVisible(deleteObjectButton);
            deleteObjectButton.setImages(normalDelete.get(),
                                         overDelete.get(),
                                         downDelete.get());
            deleteObjectButton.onClick = [this] { deleteObject(); };

            addAndMakeVisible(nameLabel);
            addAndMakeVisible(nameEditor);
            nameEditor.onTextChange = [this] { if (!nameEditor.getText().isEmpty()) {
                                                   if (checkNameAlreadyTaken(nameEditor.getText())) {
                                                       okButton.setEnabled(false);
                                                       alreadyTakenLabel.setVisible(true);
                                                   } else {
                                                       if (typeMenu.getSelectedId() > 0) {
                                                           okButton.setEnabled(true);
                                                       }
                                                       alreadyTakenLabel.setVisible(false);
                                                   }
                                               } };
            addAndMakeVisible(alreadyTakenLabel);
            alreadyTakenLabel.setJustificationType(Justification::centredRight);

            addAndMakeVisible(activeLabel);
            addAndMakeVisible(activeToggle);
            activeToggle.onStateChange = [this] {};

            addAndMakeVisible(typeLabel);
            addAndMakeVisible(typeMenu);
            typeMenu.addItem("Microphone", Raytracer::Object::Type::MICROPHONE);
            typeMenu.addItem("Speaker", Raytracer::Object::Type::SPEAKER);
            typeMenu.onChange = [this] { if (!nameEditor.getText().isEmpty()) {
                                             if (checkNameAlreadyTaken(nameEditor.getText())) {
                                                 okButton.setEnabled(false);
                                                 alreadyTakenLabel.setVisible(true);
                                             } else {
                                                 if (typeMenu.getSelectedId() > 0) {
                                                     okButton.setEnabled(true);
                                                 }
                                                 alreadyTakenLabel.setVisible(false);
                                             }
                                         } };

            addAndMakeVisible(positionLabel);
            positionLabel.setFont(juce::Font(16.0f, juce::Font::bold));

            addAndMakeVisible(xPositionSlider);
            xPositionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, xPositionSlider.getTextBoxWidth(), xPositionSlider.getTextBoxHeight());
            xPositionSlider.setSliderStyle(juce::Slider::LinearBar);
            xPositionSlider.setRange(-50.0f, 50.0f);
            xPositionSlider.setNumDecimalPlacesToDisplay(2);
            xPositionSlider.setMouseDragSensitivity(1000);
            xPositionSlider.onValueChange = [this] {};
            xPositionLabel.attachToComponent(&xPositionSlider, false);

            addAndMakeVisible(yPositionSlider);
            yPositionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, yPositionSlider.getTextBoxWidth(), yPositionSlider.getTextBoxHeight());
            yPositionSlider.setSliderStyle(juce::Slider::LinearBar);
            yPositionSlider.setRange(-50.0f, 50.0f);
            yPositionSlider.setNumDecimalPlacesToDisplay(2);
            yPositionLabel.attachToComponent(&yPositionSlider, false);

            addAndMakeVisible(zPositionSlider);
            zPositionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, zPositionSlider.getTextBoxWidth(), zPositionSlider.getTextBoxHeight());
            zPositionSlider.setSliderStyle(juce::Slider::LinearBar);
            zPositionSlider.setRange(-50.0f, 50.0f);
            zPositionSlider.setNumDecimalPlacesToDisplay(2);
            zPositionLabel.attachToComponent(&zPositionSlider, false);

            addAndMakeVisible(rotationLabel);
            rotationLabel.setFont(juce::Font(16.0f, juce::Font::bold));

            addAndMakeVisible(xRotationSlider);
            xRotationSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, xRotationSlider.getTextBoxWidth(), xRotationSlider.getTextBoxHeight());
            xRotationSlider.setSliderStyle(juce::Slider::LinearBar);
            xRotationSlider.setRange(-180.0f, 180.0f);
            xRotationSlider.setNumDecimalPlacesToDisplay(2);
            xRotationLabel.attachToComponent(&xRotationSlider, false);

            addAndMakeVisible(yRotationSlider);
            yRotationSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, yRotationSlider.getTextBoxWidth(), yRotationSlider.getTextBoxHeight());
            yRotationSlider.setSliderStyle(juce::Slider::LinearBar);
            yRotationSlider.setRange(-180.0f, 180.0f);
            yRotationSlider.setNumDecimalPlacesToDisplay(2);
            yRotationLabel.attachToComponent(&yRotationSlider, false);

            addAndMakeVisible(zRotationSlider);
            zRotationSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, zRotationSlider.getTextBoxWidth(), zRotationSlider.getTextBoxHeight());
            zRotationSlider.setSliderStyle(juce::Slider::LinearBar);
            zRotationSlider.setRange(-180.0f, 180.0f);
            zRotationSlider.setNumDecimalPlacesToDisplay(2);
            zRotationLabel.attachToComponent(&zRotationSlider, false);

            addAndMakeVisible(cancelButton);
            cancelButton.onClick = [this] { parentWindow.closeButtonPressed(); };

            addAndMakeVisible(okButton);
            okButton.onClick = [this] { if (parentWindow.currentMode == ADD) {
                                                   addObject();
                                               }
                                               if (parentWindow.currentMode == EDIT) {
                                                   updateObjectProperties();
                                               }
                                               parentWindow.closeButtonPressed(); };
        }

        void paint(juce::Graphics& /*g*/) override
        {
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(5);

            {   // Objects
                auto objectArea = area.removeFromTop(25);
                deleteObjectButton.             setBounds(objectArea.removeFromRight(objectArea.getHeight()));
                objectMenu.                     setBounds(objectArea);
            }

            {   // General Object Properties
                auto generalArea = area.removeFromTop(125);
                generalLabel.               setBounds(generalArea.removeFromTop(25));

                auto nameArea = generalArea.removeFromTop(25);
                nameLabel.                  setBounds(nameArea.removeFromLeft(generalArea.getWidth()/3));
                nameEditor.                 setBounds(nameArea);

                auto activeArea = generalArea.removeFromTop(25);
                activeLabel.                setBounds(activeArea.removeFromLeft(generalArea.getWidth()/3));
                alreadyTakenLabel.          setBounds(activeArea.removeFromRight(200));
                activeToggle.               setBounds(activeArea);

                auto typeArea = generalArea.removeFromTop(25);
                typeLabel.                  setBounds(typeArea.removeFromLeft(generalArea.getWidth()/3));
                typeMenu.                   setBounds(typeArea);
            }

            {   // Object Position
                auto positionArea = area.removeFromTop(75);
                positionLabel.              setBounds(positionArea.removeFromTop(25));

                auto padding = positionArea.removeFromTop(25);
                auto xyzArea = positionArea.removeFromTop(25);
                xPositionSlider.            setBounds(xyzArea.removeFromLeft(positionArea.getWidth()/3));
                yPositionSlider.            setBounds(xyzArea.removeFromLeft(positionArea.getWidth()/3));
                zPositionSlider.            setBounds(xyzArea.removeFromLeft(positionArea.getWidth()/3));
            }

            {   // Object Rotation
                auto rotationArea = area.removeFromTop(75);
                rotationLabel.              setBounds(rotationArea.removeFromTop(25));

                auto padding = rotationArea.removeFromTop(25);
                auto xyzArea = rotationArea.removeFromTop(25);
                xRotationSlider.            setBounds(xyzArea.removeFromLeft(rotationArea.getWidth()/3));
                yRotationSlider.            setBounds(xyzArea.removeFromLeft(rotationArea.getWidth()/3));
                zRotationSlider.            setBounds(xyzArea.removeFromLeft(rotationArea.getWidth()/3));
            }

            {
                auto buttonsArea = area.removeFromBottom(75);

                auto padding = buttonsArea.removeFromTop(25);
                cancelButton.               setBounds(buttonsArea.reduced(5).removeFromLeft(100));
                okButton.                   setBounds(buttonsArea.reduced(5).removeFromRight(100));
            }
        }

        void populateObjectMenu()
        {
            objectMenu.clear();

            for (auto& object : parentWindow.raytracer.objects) {
                objectMenu.addItem(object.name, objectMenu.getNumItems() + 1);
            }

            if (parentWindow.currentMode == ADD) {
                objectMenu.setEnabled(false);
                deleteObjectButton.setEnabled(false);

                objectMenu.setSelectedId(0, sendNotificationAsync);
            }

            if (parentWindow.currentMode == EDIT) {
                objectMenu.setEnabled(true);
                deleteObjectButton.setEnabled(true);

                if (objectMenu.getNumItems() > 0) {
                    objectMenu.setSelectedId(1, sendNotificationAsync);
                }
            }
        }

        void populateObjectProperties()
        {
            for (const auto& object : parentWindow.raytracer.objects) {
                if (object.name == parentWindow.currentObjectName) {
                    nameEditor.setText(object.name);
                    activeToggle.setToggleState(object.active, dontSendNotification);
                    typeMenu.setSelectedId(object.type);

                    xPositionSlider.setValue(object.position.x, dontSendNotification);
                    yPositionSlider.setValue(object.position.y, dontSendNotification);
                    zPositionSlider.setValue(object.position.z, dontSendNotification);
                }
            }
        }

        void clearObjectProperties()
        {
            objectMenu.clear();
            for (const auto& object : parentWindow.raytracer.objects) {
                objectMenu.addItem(object.name, objectMenu.getNumItems());
            }

            nameEditor.clear();
            activeToggle.setToggleState(false, dontSendNotification);
            alreadyTakenLabel.setVisible(false);
            typeMenu.setSelectedId(0);

            xPositionSlider.setValue(0.0f, dontSendNotification);
            yPositionSlider.setValue(0.0f, dontSendNotification);
            zPositionSlider.setValue(0.0f, dontSendNotification);

            xRotationSlider.setValue(0.0f, dontSendNotification);
            yRotationSlider.setValue(0.0f, dontSendNotification);
            zRotationSlider.setValue(0.0f, dontSendNotification);

            okButton.setEnabled(false);
        }

    private:
        void addObject()
        {
             Raytracer::Object newObject;

             newObject.name = nameEditor.getText();
             newObject.active = activeToggle.getToggleState();
             newObject.type = static_cast<Raytracer::Object::Type>(typeMenu.getSelectedId());
             newObject.position = {xPositionSlider.getValue(), yPositionSlider.getValue(), zPositionSlider.getValue()};

             parentWindow.raytracer.objects.push_back(newObject);
             parentWindow.raytracer.saveObjects();
        }

        void deleteObject()
        {
             for (int objectNum = 0; objectNum < parentWindow.raytracer.objects.size(); objectNum++) {
                if (parentWindow.raytracer.objects[objectNum].name == objectMenu.getText()) {
                    parentWindow.raytracer.objects.erase(parentWindow.raytracer.objects.begin() + objectNum);
                }
             }

             populateObjectMenu();
             parentWindow.raytracer.saveObjects();
        }

        void updateObjectProperties()
        {
            for (auto& object : parentWindow.raytracer.objects) {
                if (object.name == nameEditor.getText()) {
                    object.name = nameEditor.getText();
                    object.active = activeToggle.getToggleState();
                    object.type = static_cast<Raytracer::Object::Type>(typeMenu.getSelectedId());

                    object.position.x = (float) xPositionSlider.getValue();
                    object.position.y = (float) yPositionSlider.getValue();
                    object.position.z = (float) zPositionSlider.getValue();
                }
            }

            parentWindow.raytracer.saveObjects();
        }

        bool checkNameAlreadyTaken(const String& name)
        {
            for (const auto& object : parentWindow.raytracer.objects) {
                if (object.name == name && name != parentWindow.currentObjectName) {
                    return true;
                }
            }

            return false;
        }

        ObjectWindow& parentWindow;

        Label           generalLabel{{}, "General"};

        ComboBox        objectMenu;
        DrawableButton  deleteObjectButton{"Delete object", juce::DrawableButton::ImageOnButtonBackground};
        Label           nameLabel{{}, "Name"};
        TextEditor      nameEditor;
        Label           alreadyTakenLabel{{}, "This name is already taken."};
        Label           activeLabel{{}, "Active"};
        ToggleButton    activeToggle;
        Label           typeLabel{{}, "Type"};
        ComboBox        typeMenu;

        Label           positionLabel{{}, "Position"};

        Label           xPositionLabel{{}, "X Position"};
        Slider          xPositionSlider;
        Label           yPositionLabel{{}, "Y Position"};
        Slider          yPositionSlider;
        Label           zPositionLabel{{}, "Z Position"};
        Slider          zPositionSlider;

        Label           rotationLabel{{}, "Rotation"};

        Label           xRotationLabel{{}, "X Rotation"};
        Slider          xRotationSlider;
        Label           yRotationLabel{{}, "Y Rotation"};
        Slider          yRotationSlider;
        Label           zRotationLabel{{}, "Z Rotation"};
        Slider          zRotationSlider;

        TextButton      cancelButton{"Cancel"};
        TextButton      okButton{"Ok"};
    };

    void closeButtonPressed() override;

    RaumsimulationAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& parameters;
    Raytracer& raytracer;
    ObjectComponent objectComponent{*this};

    Mode currentMode;
    String currentObjectName;
};
