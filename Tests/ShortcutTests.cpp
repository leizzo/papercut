#include "TestFixture.h"
#include "Commands/ApplicationCommandTable.h"

namespace resamper::test
{

/** The default shortcut map (PRD §17) and the Commands behind its rows. */
struct ShortcutTests : juce::UnitTest
{
    ShortcutTests() : juce::UnitTest ("Shortcuts", "Resamper") {}

    static juce::String describe (const KeyBinding& b)
    {
        return juce::KeyPress (b.keyCode, juce::ModifierKeys (b.modifiers), 0).getTextDescription();
    }

    void runTest() override
    {
        beginTest ("No two bindings share a key in the same context; a view's key never shadows a global one");
        {
            const auto bindings = getKeyBindings();

            for (size_t i = 0; i < bindings.size(); ++i)
                for (size_t j = i + 1; j < bindings.size(); ++j)
                {
                    const auto& a = bindings[i];
                    const auto& b = bindings[j];
                    const auto sameKey = a.keyCode == b.keyCode && a.modifiers == b.modifiers;
                    const auto overlap = a.contexts == anyView || b.contexts == anyView || (a.contexts & b.contexts) != 0;

                    expect (! (sameKey && overlap),
                            describe (a) + " is bound to both " + a.commandId + " and " + b.commandId);
                }
        }

        beginTest ("Mod is the command modifier (Cmd on macOS, Ctrl on Windows)");
        {
            const auto undo = findShortcut ("edit.undo");
            expect (undo.getModifiers().isCommandDown());
            expectEquals (undo.getKeyCode(), (int) 'Z');
        }

        beginTest ("A view's shortcut fires only in that view");
        {
            const auto q = juce::KeyPress ('Q', juce::ModifierKeys(), 0);
            expect (findBinding (q, pianoRollView) != nullptr);
            expect (findBinding (q, arrangeView) == nullptr);

            const auto s = juce::KeyPress ('S', juce::ModifierKeys(), 0);
            expectEquals (juce::String (findBinding (s, mixerView)->commandId), juce::String ("track.toggleSoloSelected"));
            expect (findBinding (s, pianoRollView) == nullptr);

            // Global shortcuts work everywhere.
            expect (findBinding (juce::KeyPress (juce::KeyPress::spaceKey), pianoRollView) != nullptr);
        }

        beginTest ("Every menu command's shortcut is in the bindings, and pending §17 rows name their ticket");
        {
            for (auto& p : getPendingShortcuts())
                expect (juce::String (p.waitingOn).isNotEmpty(), p.keys);

            expect (findShortcut ("view.toggleSessionArrange").isValid());
            expect (findShortcut ("transport.playFromSelection").isValid());
        }

        //==============================================================================
        beginTest ("F1-F8 toggle the mute of tracks 1-8");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.invoke (cmd::trackAdd);
            auto pressF = [&f] (int keyCode)
            {
                auto* binding = findBinding (juce::KeyPress (keyCode), anyView);
                return binding != nullptr && f.commands.invokeById (binding->commandId, bindingArgs (*binding));
            };

            expect (pressF (juce::KeyPress::F2Key));
            expect (! f.model.getTracks()[0].muted && f.model.getTracks()[1].muted);

            expect (pressF (juce::KeyPress::F8Key));   // no track 8: nothing happens
            expect (f.model.getTracks()[1].muted);
        }

        beginTest ("S solos the selected track");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.model.selectTrack (f.model.getTracks()[0].id);
            f.invoke (cmd::trackToggleSoloSelected);
            expect (f.model.getTracks()[0].solo);
        }

        beginTest ("Mod+L loops the selected clips; with none selected it toggles the loop");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 2.0);
            f.invoke (cmd::clipAdd);
            f.model.selectClip (f.model.getTracks()[0].clips[0].id);

            f.invoke (cmd::transportLoopSelection);
            expect (f.model.isLooping());
            expectWithinAbsoluteError (f.model.getLoopRange().start, 0.0, 1.0e-6);
            expectWithinAbsoluteError (f.model.getLoopRange().end, 2.0, 1.0e-3);

            f.model.selectClip ({});
            f.invoke (cmd::transportLoopSelection);
            expect (! f.model.isLooping());
        }

        beginTest ("Shift+Space plays from the selection");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke (cmd::clipAdd);
            f.invoke (cmd::clipCopy, { f.model.getTracks()[0].clips[0].id, 3.0 });   // selects the copy

            f.invoke (cmd::transportPlayFromSelection);
            expect (f.model.isPlaying());
            expectWithinAbsoluteError (f.model.getTransportPositionSeconds(), 3.0, 0.05);
            f.invoke (cmd::transportStop);
        }

        beginTest ("Up and Down transpose the selected notes; Mod+A selects all of a clip's notes");
        {
            Fixture f;
            f.invoke (cmd::trackAddMidi);
            f.model.selectTrack (f.model.getTracks()[0].id);
            f.invoke (cmd::clipAddMidi);
            const auto clipId = f.model.getTracks()[0].clips[0].id;
            f.invoke (cmd::noteAdd, { clipId, 0.0, 0.25, 60 });
            f.invoke (cmd::noteAdd, { clipId, 0.5, 0.25, 64 });
            f.model.selectNotes ({});

            f.invoke (cmd::noteSelectAll, { clipId });
            f.invoke (cmd::noteTransposeSelected, { clipId, 12 });

            const auto notes = f.model.getTracks()[0].clips[0].notes;
            expectEquals (notes[0].pitch, 72);
            expectEquals (notes[1].pitch, 76);
        }
    }
};

static ShortcutTests shortcutTests;

} // namespace resamper::test
