#pragma once

#include <JuceHeader.h>

class CustomLookAndFeel: public LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        ColourScheme colourScheme{ 0xff101010,      // windowBackground
                                   0xff202020,      // widgetBackground
                                   0xff101010,      // menuBackground
                                   0xff303030,      // outline
                                   0xffa0a0a0,      // defaultText
                                   0xff303030,      // defaultFill
                                   0xfff0f0f0,      // highlightedText
                                   0xff808080,      // highlightedFill
                                   0xffa0a0a0 };    // menuText
        
        setColourScheme(colourScheme);
        
        {
            const uint32 transparent = 0x00000000;
            
            const uint32 coloursToUse[] =
                    {
                            TextButton::buttonColourId,                 colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            TextButton::buttonOnColourId,               colourScheme.getUIColour (ColourScheme::UIColour::highlightedFill).getARGB(),
                            TextButton::textColourOnId,                 colourScheme.getUIColour (ColourScheme::UIColour::highlightedText).getARGB(),
                            TextButton::textColourOffId,                colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),

                            ToggleButton::textColourId,                 colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            ToggleButton::tickColourId,                 colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            ToggleButton::tickDisabledColourId,         colourScheme.getUIColour (ColourScheme::UIColour::defaultText).withAlpha (0.5f).getARGB(),

                            TextEditor::backgroundColourId,             colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            TextEditor::textColourId,                   colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            TextEditor::highlightColourId,              colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).withAlpha (0.4f).getARGB(),
                            TextEditor::highlightedTextColourId,        colourScheme.getUIColour (ColourScheme::UIColour::highlightedText).getARGB(),
                            TextEditor::outlineColourId,                colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),
                            TextEditor::focusedOutlineColourId,         colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),
                            TextEditor::shadowColourId,                 transparent,

                            CaretComponent::caretColourId,              colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).getARGB(),

                            Label::backgroundColourId,                  transparent,
                            Label::textColourId,                        colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            Label::outlineColourId,                     transparent,
                            Label::textWhenEditingColourId,             colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),

                            ScrollBar::backgroundColourId,              transparent,
                            ScrollBar::thumbColourId,                   colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).getARGB(),
                            ScrollBar::trackColourId,                   transparent,

                            TreeView::linesColourId,                    transparent,
                            TreeView::backgroundColourId,               transparent,
                            TreeView::dragAndDropIndicatorColourId,     colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),
                            TreeView::selectedItemBackgroundColourId,   transparent,
                            TreeView::oddItemsColourId,                 transparent,
                            TreeView::evenItemsColourId,                transparent,

                            PopupMenu::backgroundColourId,              colourScheme.getUIColour (ColourScheme::UIColour::menuBackground).getARGB(),
                            PopupMenu::textColourId,                    colourScheme.getUIColour (ColourScheme::UIColour::menuText).getARGB(),
                            PopupMenu::headerTextColourId,              colourScheme.getUIColour (ColourScheme::UIColour::menuText).getARGB(),
                            PopupMenu::highlightedTextColourId,         colourScheme.getUIColour (ColourScheme::UIColour::highlightedText).getARGB(),
                            PopupMenu::highlightedBackgroundColourId,   colourScheme.getUIColour (ColourScheme::UIColour::highlightedFill).getARGB(),

                            ComboBox::buttonColourId,                   colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),
                            ComboBox::outlineColourId,                  colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),
                            ComboBox::textColourId,                     colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            ComboBox::backgroundColourId,               colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            ComboBox::arrowColourId,                    colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            ComboBox::focusedOutlineColourId,           colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),

                            PropertyComponent::backgroundColourId,      colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            PropertyComponent::labelTextColourId,       colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),

                            TextPropertyComponent::backgroundColourId,  colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            TextPropertyComponent::textColourId,        colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            TextPropertyComponent::outlineColourId,     colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),

                            BooleanPropertyComponent::backgroundColourId, colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            BooleanPropertyComponent::outlineColourId,    colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),

                            ListBox::backgroundColourId,                colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            ListBox::outlineColourId,                   colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),
                            ListBox::textColourId,                      colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),

                            Slider::backgroundColourId,                 colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            Slider::thumbColourId,                      colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).getARGB(),
                            Slider::trackColourId,                      colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).getARGB(),
                            Slider::rotarySliderFillColourId,           colourScheme.getUIColour (ColourScheme::UIColour::highlightedFill).getARGB(),
                            Slider::rotarySliderOutlineColourId,        colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            Slider::textBoxTextColourId,                colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            Slider::textBoxBackgroundColourId,          colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).withAlpha (0.0f).getARGB(),
                            Slider::textBoxHighlightColourId,           colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).withAlpha (0.4f).getARGB(),
                            Slider::textBoxOutlineColourId,             colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),

                            ResizableWindow::backgroundColourId,        colourScheme.getUIColour (ColourScheme::UIColour::windowBackground).getARGB(),

                            DocumentWindow::textColourId,               colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),

                            AlertWindow::backgroundColourId,            colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            AlertWindow::textColourId,                  colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            AlertWindow::outlineColourId,               colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),

                            ProgressBar::backgroundColourId,            colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            ProgressBar::foregroundColourId,            colourScheme.getUIColour (ColourScheme::UIColour::highlightedFill).getARGB(),

                            TooltipWindow::backgroundColourId,          colourScheme.getUIColour (ColourScheme::UIColour::highlightedFill).getARGB(),
                            TooltipWindow::textColourId,                colourScheme.getUIColour (ColourScheme::UIColour::highlightedText).getARGB(),
                            TooltipWindow::outlineColourId,             transparent,

                            TabbedComponent::backgroundColourId,        transparent,
                            TabbedComponent::outlineColourId,           colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),
                            TabbedButtonBar::tabOutlineColourId,        colourScheme.getUIColour (ColourScheme::UIColour::outline).withAlpha (0.5f).getARGB(),
                            TabbedButtonBar::frontOutlineColourId,      colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),

                            Toolbar::backgroundColourId,                colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).withAlpha (0.4f).getARGB(),
                            Toolbar::separatorColourId,                 colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),
                            Toolbar::buttonMouseOverBackgroundColourId, colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).contrasting (0.2f).getARGB(),
                            Toolbar::buttonMouseDownBackgroundColourId, colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).contrasting (0.5f).getARGB(),
                            Toolbar::labelTextColourId,                 colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            Toolbar::editingModeOutlineColourId,        colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),

                            DrawableButton::textColourId,               colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            DrawableButton::textColourOnId,             colourScheme.getUIColour (ColourScheme::UIColour::highlightedText).getARGB(),
                            DrawableButton::backgroundColourId,         transparent,
                            DrawableButton::backgroundOnColourId,       colourScheme.getUIColour (ColourScheme::UIColour::highlightedFill).getARGB(),

                            HyperlinkButton::textColourId,              colourScheme.getUIColour (ColourScheme::UIColour::defaultText).interpolatedWith (Colours::blue, 0.4f).getARGB(),

                            GroupComponent::outlineColourId,            colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),
                            GroupComponent::textColourId,               colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),

                            BubbleComponent::backgroundColourId,        colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            BubbleComponent::outlineColourId,           colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),

                            DirectoryContentsDisplayComponent::highlightColourId,          colourScheme.getUIColour (ColourScheme::UIColour::highlightedFill).getARGB(),
                            DirectoryContentsDisplayComponent::textColourId,               colourScheme.getUIColour (ColourScheme::UIColour::menuText).getARGB(),
                            DirectoryContentsDisplayComponent::highlightedTextColourId,    colourScheme.getUIColour (ColourScheme::UIColour::highlightedText).getARGB(),

                            0x1000440, /*LassoComponent::lassoFillColourId*/        colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).getARGB(),
                            0x1000441, /*LassoComponent::lassoOutlineColourId*/     colourScheme.getUIColour (ColourScheme::UIColour::outline).getARGB(),

                            0x1004000, /*KeyboardComponentBase::upDownButtonBackgroundColourId*/  0xffd3d3d3,
                            0x1004001, /*KeyboardComponentBase::upDownButtonArrowColourId*/       0xff000000,

                            0x1005000, /*MidiKeyboardComponent::whiteNoteColourId*/               0xffffffff,
                            0x1005001, /*MidiKeyboardComponent::blackNoteColourId*/               0xff000000,
                            0x1005002, /*MidiKeyboardComponent::keySeparatorLineColourId*/        0x66000000,
                            0x1005003, /*MidiKeyboardComponent::mouseOverKeyOverlayColourId*/     0x80ffff00,
                            0x1005004, /*MidiKeyboardComponent::keyDownOverlayColourId*/          0xffb6b600,
                            0x1005005, /*MidiKeyboardComponent::textLabelColourId*/               0xff000000,
                            0x1005006, /*MidiKeyboardComponent::shadowColourId*/                  0x4c000000,

                            0x1006000, /*MPEKeyboardComponent::whiteNoteColourId*/                0xff1a1c27,
                            0x1006001, /*MPEKeyboardComponent::blackNoteColourId*/                0x99f1f1f1,
                            0x1006002, /*MPEKeyboardComponent::textLabelColourId*/                0xfff1f1f1,
                            0x1006003, /*MPEKeyboardComponent::noteCircleFillColourId*/           0x99ba00ff,
                            0x1006004, /*MPEKeyboardComponent::noteCircleOutlineColourId*/        0xfff1f1f1,

                            0x1004500, /*CodeEditorComponent::backgroundColourId*/                colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            0x1004502, /*CodeEditorComponent::highlightColourId*/                 colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).withAlpha (0.4f).getARGB(),
                            0x1004503, /*CodeEditorComponent::defaultTextColourId*/               colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            0x1004504, /*CodeEditorComponent::lineNumberBackgroundId*/            colourScheme.getUIColour (ColourScheme::UIColour::highlightedFill).withAlpha (0.5f).getARGB(),
                            0x1004505, /*CodeEditorComponent::lineNumberTextId*/                  colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).getARGB(),

                            0x1007000, /*ColourSelector::backgroundColourId*/                     colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            0x1007001, /*ColourSelector::labelTextColourId*/                      colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),

                            0x100ad00, /*KeyMappingEditorComponent::backgroundColourId*/          colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            0x100ad01, /*KeyMappingEditorComponent::textColourId*/                colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),

                            FileSearchPathListComponent::backgroundColourId,        colourScheme.getUIColour (ColourScheme::UIColour::menuBackground).getARGB(),

                            FileChooserDialogBox::titleTextColourId,                colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),

                            SidePanel::backgroundColour,                            colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).getARGB(),
                            SidePanel::titleTextColour,                             colourScheme.getUIColour (ColourScheme::UIColour::defaultText).getARGB(),
                            SidePanel::shadowBaseColour,                            colourScheme.getUIColour (ColourScheme::UIColour::widgetBackground).darker().getARGB(),
                            SidePanel::dismissButtonNormalColour,                   colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).getARGB(),
                            SidePanel::dismissButtonOverColour,                     colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).darker().getARGB(),
                            SidePanel::dismissButtonDownColour,                     colourScheme.getUIColour (ColourScheme::UIColour::defaultFill).brighter().getARGB(),

                            FileBrowserComponent::currentPathBoxBackgroundColourId,    colourScheme.getUIColour (ColourScheme::UIColour::menuBackground).getARGB(),
                            FileBrowserComponent::currentPathBoxTextColourId,          colourScheme.getUIColour (ColourScheme::UIColour::menuText).getARGB(),
                            FileBrowserComponent::currentPathBoxArrowColourId,         colourScheme.getUIColour (ColourScheme::UIColour::menuText).getARGB(),
                            FileBrowserComponent::filenameBoxBackgroundColourId,       colourScheme.getUIColour (ColourScheme::UIColour::menuBackground).getARGB(),
                            FileBrowserComponent::filenameBoxTextColourId,             colourScheme.getUIColour (ColourScheme::UIColour::menuText).getARGB(),
                    };

            for (int i = 0; i < numElementsInArray(coloursToUse); i += 2)
                setColour((int) coloursToUse[i], Colour((uint32) coloursToUse[i + 1]));
        }
    }
};
