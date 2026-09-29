#include "Browser.h"
#include "Commands/CommandRegistry.h"
#include "Commands/PluginCommands.h"
#include "Engine/SamplePreview.h"

namespace resamper
{

namespace
{
    constexpr LibraryCategory categories[] = { LibraryCategory::sounds, LibraryCategory::drums, LibraryCategory::instruments,
                                               LibraryCategory::audioEffects, LibraryCategory::midiEffects,
                                               LibraryCategory::plugins, LibraryCategory::clips, LibraryCategory::samples };

    constexpr int categoryRowHeight = 30, itemRowHeight = 26, headerHeight = 26;

    const juce::String chevron (juce::CharPointer_UTF8 ("  \xe2\x80\xba  "));

    /** Hovering a sample this long starts its preview. */
    constexpr int hoverPreviewMs = 250;

    Icon iconFor (LibraryCategory c)
    {
        switch (c)
        {
            case LibraryCategory::sounds:        return Icon::audioLines;
            case LibraryCategory::drums:         return Icon::record;
            case LibraryCategory::instruments:   return Icon::music;
            case LibraryCategory::audioEffects:  return Icon::spline;
            case LibraryCategory::midiEffects:   return Icon::gripVertical;
            case LibraryCategory::plugins:       return Icon::layers;
            case LibraryCategory::clips:         return Icon::file;
            case LibraryCategory::samples:       return Icon::audioLines;
        }

        return Icon::file;
    }
}

Browser::Browser (CommandRegistry& c, PluginRack& r, ApplicationModel& m, ThemeManager& tm, SamplePreview& p, juce::File root)
    : commands (c), rack (r), model (m), themeManager (tm), preview (p),
      library ([&r] { return r.getCatalogue(); }, std::move (root)),
      scan (tm, "Scan", Button::Variant::ghost)
{
    setWantsKeyboardFocus (false);

    auto& theme = themeManager.getTheme();
    search.setTextToShowWhenEmpty ("Search Library", theme.textDim);
    search.setFont (themeManager.font (TypeStyle { 12.0f, false, 400 }));
    search.setIndents (28, 0);
    search.setJustification (juce::Justification::centredLeft);
    search.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    search.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    search.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    search.onTextChange = [this] { folder = juce::File(); refresh(); };
    search.onEscapeKey = [this] { search.clear(); refresh(); };

    scan.setTooltip ("Scan for VST3 / AU plug-ins");
    scan.onClick = [this] { commands.invoke ("plugin.scan"); };

    list.setRowHeight (itemRowHeight);
    list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    list.setOutlineThickness (0);
    list.addMouseListener (this, true);

    addAndMakeVisible (search);
    addAndMakeVisible (list);
    addChildComponent (scan);

    refresh();
    startTimerHz (4);   // picks up a finished plug-in scan
}

Browser::~Browser()
{
    preview.stop();
}

void Browser::refresh()
{
    items = library.list (category, folder, search.getText());
    catalogueSize = rack.getCatalogue().size();
    scan.setVisible (category == LibraryCategory::plugins);
    list.updateContent();
    list.deselectAllRows();
    repaint();
}

void Browser::selectCategory (LibraryCategory c)
{
    category = c;
    folder = juce::File();
    refresh();
}

void Browser::open (int row)
{
    if (! juce::isPositiveAndBelow (row, (int) items.size()))
        return;

    auto& item = items[(size_t) row];

    if (item.kind == LibraryItem::Kind::folder)
    {
        folder = item.file;
        search.clear();
        refresh();
    }
    else if (item.kind == LibraryItem::Kind::plugin)
    {
        if (auto trackId = model.getSelectedTrackId(); trackId.isNotEmpty())
            commands.invoke ("plugin.insert", pluginInsertArgs (trackId, item.pluginPath));
    }
    else
    {
        previewRow (row);
    }
}

void Browser::previewRow (int row)
{
    if (juce::isPositiveAndBelow (row, (int) items.size()) && items[(size_t) row].kind == LibraryItem::Kind::audioFile)
        preview.play (items[(size_t) row].file);
}

juce::String Browser::breadcrumb() const
{
    auto text = Library::nameOf (category);

    if (search.getText().trim().isNotEmpty())
        return text + chevron + "\"" + search.getText().trim() + "\"";

    if (folder.isDirectory())
    {
        const auto base = library.folderFor (category);
        juce::StringArray parts;

        for (auto f = folder; f != base && f.isAChildOf (base); f = f.getParentDirectory())
            parts.insert (0, f.getFileName());

        for (auto& part : parts)
            text << chevron << part;
    }

    return text;
}

juce::Rectangle<int> Browser::categoryBounds (int index) const
{
    return categoryArea.withHeight (categoryRowHeight).translated (0, index * categoryRowHeight);
}

int Browser::categoryAt (juce::Point<int> p) const
{
    for (int i = 0; i < (int) std::size (categories); ++i)
        if (categoryBounds (i).contains (p))
            return i;

    return -1;
}

void Browser::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    g.fillAll (theme.bgPanel);
    g.setColour (theme.borderSoft);
    g.fillRect (getWidth() - 1, 0, 1, getHeight());

    // Search well.
    const auto field = search.getBounds().toFloat();
    g.setColour (theme.bgSlot);
    g.fillRoundedRectangle (field, 7.0f);
    g.setColour (search.hasKeyboardFocus (true) ? theme.focusRing : theme.border);
    g.drawRoundedRectangle (field.reduced (0.5f), 7.0f, 1.0f);
    drawIcon (g, Icon::search, field.withWidth (28.0f).withSizeKeepingCentre (14.0f, 14.0f).translated (3.0f, 0.0f),
              theme.textDim);

    // Categories.
    for (int i = 0; i < (int) std::size (categories); ++i)
    {
        auto row = categoryBounds (i);
        const auto selected = categories[i] == category;

        if (selected || i == hoveredCategory)
        {
            g.setColour (selected ? theme.bgElevated : theme.bgHover);
            g.fillRoundedRectangle (row.toFloat(), theme.radiusLg);
        }

        auto content = row.reduced (10, 0);
        drawIcon (g, iconFor (categories[i]), content.removeFromLeft (15).toFloat().withSizeKeepingCentre (15.0f, 15.0f),
                  selected ? theme.accent : theme.textSecondary);
        content.removeFromLeft (10);
        g.setColour (selected ? theme.textPrimary : theme.textSecondary);
        g.setFont (themeManager.font (TypeStyle { 12.5f, false, selected ? 600 : 400 }));
        g.drawText (Library::nameOf (categories[i]), content, juce::Justification::centredLeft, true);
    }

    // Divider and the list header (a breadcrumb; click to go up a folder).
    g.setColour (theme.borderSoft);
    g.fillRect (0, headerArea.getY() - 1, getWidth(), 1);
    drawStyledText (g, themeManager, breadcrumb(), TypeStyle { 10.0f, false, 600, true, 0.0f },
                    headerArea.reduced (16, 0).withTrimmedRight (scan.isVisible() ? scan.getWidth() : 0),
                    juce::Justification::centredLeft, theme.textDim);

    if (items.empty())
        drawStyledText (g, themeManager,
                        Library::isFileCategory (category) ? "Drop audio files into " + library.folderFor (category).getFullPathName()
                                                           : juce::String ("Nothing here"),
                        theme.bodySm, list.getBounds().reduced (16, 8).withHeight (40), juce::Justification::topLeft,
                        theme.textDim);
}

void Browser::resized()
{
    auto r = getLocalBounds().withTrimmedRight (1);
    auto top = r.removeFromTop (12 + 32 + 12).reduced (14, 12);
    search.setBounds (top);

    categoryArea = r.removeFromTop (4 + (int) std::size (categories) * categoryRowHeight + 4).reduced (8, 4);
    r.removeFromTop (1);
    r.reduce (0, 8);
    headerArea = r.removeFromTop (headerHeight);
    scan.setBounds (headerArea.removeFromRight (scan.getIdealWidth() + 8).reduced (0, 2).translated (-8, 0));
    list.setBounds (r.reduced (8, 0));
}

void Browser::mouseDown (const juce::MouseEvent& e)
{
    if (e.eventComponent != this)
        return;

    if (auto index = categoryAt (e.getPosition()); index >= 0)
    {
        selectCategory (categories[index]);
        return;
    }

    // The header goes up one folder.
    if (headerArea.contains (e.getPosition()) && folder.isDirectory())
    {
        const auto parent = folder.getParentDirectory();
        folder = parent == library.folderFor (category) ? juce::File() : parent;
        refresh();
    }
}

void Browser::mouseMove (const juce::MouseEvent& e)
{
    if (e.eventComponent == this)
    {
        if (auto index = categoryAt (e.getPosition()); index != hoveredCategory)
        {
            hoveredCategory = index;
            repaint (categoryArea);
        }

        return;
    }

    const auto row = list.getRowContainingPosition (e.getEventRelativeTo (&list).x, e.getEventRelativeTo (&list).y);

    if (row != hoveredRow)
    {
        hoveredRow = row;
        list.repaint();
        preview.stop();
        pendingPreview = juce::isPositiveAndBelow (row, (int) items.size()) && items[(size_t) row].kind == LibraryItem::Kind::audioFile
                           ? items[(size_t) row].file : juce::File();
        startTimer (hoverPreviewMs);
    }
}

void Browser::mouseExit (const juce::MouseEvent&)
{
    hoveredCategory = hoveredRow = -1;
    pendingPreview = juce::File();
    preview.stop();
    repaint();
}

bool Browser::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::rightKey)
    {
        previewRow (list.getSelectedRow());
        return true;
    }

    return false;
}

void Browser::timerCallback()
{
    if (pendingPreview != juce::File())
    {
        preview.play (pendingPreview);
        pendingPreview = juce::File();
    }

    if (! rack.isScanning() && rack.getCatalogue().size() != catalogueSize && ! Library::isFileCategory (category))
        refresh();

    startTimerHz (4);
}

void Browser::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (! juce::isPositiveAndBelow (row, (int) items.size()))
        return;

    auto& theme = themeManager.getTheme();
    auto& item = items[(size_t) row];
    auto area = juce::Rectangle<int> (width, height);

    if (selected || row == hoveredRow)
    {
        g.setColour (selected ? theme.bgElevated : theme.bgHover);
        g.fillRoundedRectangle (area.toFloat(), 5.0f);
    }

    auto content = area.reduced (8, 0);
    const auto icon = item.kind == LibraryItem::Kind::folder ? Icon::folder
                    : item.kind == LibraryItem::Kind::plugin ? (item.instrument ? Icon::music : Icon::layers)
                                                             : Icon::audioLines;
    const auto iconColour = item.kind == LibraryItem::Kind::plugin ? theme.accentDim
                          : item.kind == LibraryItem::Kind::folder ? theme.textSecondary : theme.textDim;
    drawIcon (g, icon, content.removeFromLeft (14).toFloat().withSizeKeepingCentre (14.0f, 14.0f), iconColour);
    content.removeFromLeft (9);

    g.setColour (item.kind == LibraryItem::Kind::audioFile ? theme.textSecondary : theme.textPrimary);
    g.setFont (themeManager.font (TypeStyle { 12.0f, false, 400 }));
    g.drawText (item.name, content, juce::Justification::centredLeft, true);
}

void Browser::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    if (juce::isPositiveAndBelow (row, (int) items.size()) && items[(size_t) row].kind == LibraryItem::Kind::folder)
        open (row);
}

void Browser::listBoxItemDoubleClicked (int row, const juce::MouseEvent&)
{
    open (row);
}

juce::var Browser::getDragSourceDescription (const juce::SparseSet<int>& rows)
{
    if (rows.isEmpty() || ! juce::isPositiveAndBelow (rows[0], (int) items.size()))
        return {};

    auto& item = items[(size_t) rows[0]];
    return item.kind == LibraryItem::Kind::folder ? juce::var() : dragDescription (item);
}

} // namespace resamper
