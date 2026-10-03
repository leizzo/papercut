# PR evidence

Every pull request shows the behavior the change makes visible. Pick from what that change draws.

## What to attach

- A still, when one frame is the result: a window, a menu, a dialog, a layout.
- A video, when the result is motion or a sequence: a drag, playback, transport, one window following another.
- One sentence in the PR, and no file, when the change never draws. Name the check that stood in for a picture.

Done when the PR body has that still, that clip, or that sentence.

## Capture

The tests open real windows. The suite filter is the `UnitTest` name (`ResamperTests "Plug-in Sandbox"`). The run activates the app and can move the pointer.

Hold the state with `runDispatchLoopUntil` in the test, long enough to capture. Take that pause back out before committing. The test in the PR is the real one.

Another app covers the plug-in window, and a screen rectangle then shows that app. Capture each Resamper window with `screencapture -l <id> -o` and stack them back to front: the plug-in window, its sandbox panel, then the popup. Window ids come from `CGWindowListCopyWindowInfo`; keep those whose `kCGWindowOwnerName` contains `Resamper`.

A transparent window (a tooltip) comes back black from `-l`. Bring the plug-in window to the front, clear of other apps, and capture the screen region.

For video, run `screencapture -v` while that test runs, with the plug-in window in front and uncovered. Stop once the interaction has happened. The clip is that interaction.

## On the PR

Files live in `docs/screenshots/<issue>/`. Link them from the PR body. `gh pr edit` fails on this repo (Projects classic). Set the body with:

```sh
gh api --method PATCH repos/<owner>/<repo>/pulls/<n> -f body=...
```

Done when the still or the clip renders in the description.
