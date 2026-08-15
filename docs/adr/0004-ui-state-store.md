# UI State lives in an app-owned store, not in components

Hot Reload destroys and recreates component subtrees, and the brief §16 requires scroll/zoom/focus to survive. Rather than scraping state out of dying components (capture/restore), UI State lives in an app-owned ValueTree store keyed by component ID; components bind to it on construction. The structural invariant: **components are disposable, state is not.**

**Considered options:** §16's literal capture/restore snapshots at reload time — rejected because it only works at reload boundaries and can't express state shared between components (TimelineHeader, TrackLanes, and Playhead all need the same zoom/scroll to implement `timeToX`/`xToTime`).

**Consequences:** Selection is *not* UI State — it lives in the engine's SelectionManager and survives reload for free, so it drops off brief §16's preservation list. Arrangement coordinate conversion (`timeToX`/`xToTime`) has a single owner: the view-state object holding zoom and scroll, not free functions.
