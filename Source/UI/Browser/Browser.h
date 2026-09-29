#pragma once

#include "Library.h"
#include "UI/Controls/Controls.h"

namespace resamper
{

class CommandRegistry;
class SamplePreview;

/** The left Browser, 236 wide (PRD §6.2): search, categories, a divider, and
    the list of the chosen category. Items drag onto tracks (a device onto the
    device chain, a sample onto an audio track as a clip) and, later, onto
    empty mixer insert slots. Hovering a sample previews it; `→` previews the
    selected item. Double-clicking a device adds it to the selected track's
    device chain. Replaces the old plug-in list. */
class Browser : public juce::Component,
                private juce::ListBoxModel,
                private juce::Timer
{
public:
    Browser (CommandRegistry&, PluginRack&, ApplicationModel&, ThemeManager&, SamplePreview&, juce::File libraryRoot);
    ~Browser() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    CommandRegistry& commands;
    PluginRack& rack;
    ApplicationModel& model;
    ThemeManager& themeManager;
    SamplePreview& preview;
    Library library;

    juce::TextEditor search;
    Button scan;
    juce::ListBox list { "Library", this };

    LibraryCategory category = LibraryCategory::audioEffects;
    juce::File folder;
    std::vector<LibraryItem> items;
    int hoveredCategory = -1, hoveredRow = -1;
    juce::File pendingPreview;
    int catalogueSize = 0;

    juce::Rectangle<int> categoryArea, headerArea;

    void refresh();
    void selectCategory (LibraryCategory);
    void open (int row);
    juce::Rectangle<int> categoryBounds (int index) const;
    int categoryAt (juce::Point<int>) const;
    juce::String breadcrumb() const;
    void previewRow (int row);

    // ListBoxModel
    int getNumRows() override   { return (int) items.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;
    void returnKeyPressed (int row) override   { open (row); }
    juce::var getDragSourceDescription (const juce::SparseSet<int>& rows) override;

    void timerCallback() override;
};

} // namespace resamper
