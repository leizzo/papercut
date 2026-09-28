# Papercut

**English** · [Türkçe](#türkçe)

A dark, dense, keyboard-friendly desktop DAW for electronic producers and mix engineers.

> **Status: alpha.** Papercut is in early development. The current release is
> [v0.1.0 — M1 Core](https://github.com/leizzo/papercut/releases/tag/v0.1.0), published as an alpha
> pre-release. Expect missing features, rough edges and project-format changes — don't trust it with
> your only copy of a song yet.

## What is Papercut?

Papercut takes you from idea to structure to mix in a single window. Its core principle:
**sound design lives in the device chain, mixing lives in the mixer** — two separate chains, with a
signal path you can always see.

## What you can do today (v0.1.0)

- **Arrange** — audio and MIDI tracks, clips you can move, resize, split, duplicate, loop-extend and
  consolidate; zoom, lane height and Follow.
- **Record** — audio recording with input selection, live waveform and takes; MIDI recording from a
  MIDI input; count-in (Shift-click Rec to skip it).
- **Edit MIDI** — piano roll note editing and quantize.
- **Shape sound** — a per-track device chain with plug-ins, shown in the detail view.
- **Mix** — volume, pan, mute, solo, stereo meters, 8 insert slots per channel with bypass and reorder.
- **Browse** — a library browser with search, categories, sample preview and drag-to-track.
- **Stay safe** — autosave and crash recovery.

See [CHANGELOG.md](CHANGELOG.md) for the full list.

## Roadmap

| Milestone | What it brings |
|---|---|
| **M1 — Core** ✅ | Shell, transport, Arrangement, device chain with plug-ins, basic Mixer, design system (v0.1.0) |
| **M1.1 — Devices & Plug-ins** | Native devices (EQ Eight, Compressor), VST3 / AU / CLAP hosting with crash isolation, plug-in window on insert |
| **M2 — Mix** | Pre-FX / Pre / Post sends, returns, master loudness, folders & buses, sidechain inputs |
| **M3 — Automation** | Arrangement lanes, clip overlays, Read / Touch / Latch / Write |
| **M4 — Editors** | Scale-aware piano roll with chords and velocity, audio editor with warp and fades, clip envelopes |
| **M5 — Racks & Session** | Instrument / Drum / Audio Effect racks, macros, Session view with scenes, crossfader |

Target platforms: macOS 13+ (Apple Silicon) and Windows 11 x64. The alpha currently builds on macOS.

## Getting started

There are no prebuilt downloads yet — build from source (see [For developers](#for-developers)),
then open `Papercut.app`.

### Keyboard shortcuts

`Mod` = Cmd on macOS, Ctrl on Windows.

| Action | Shortcut |
|---|---|
| Play / Stop · Play from selection | `Space` · `Shift+Space` |
| Record | `F9` (count-in; Shift-click Rec to skip) |
| Loop selection · Return to start | `Mod+L` · `Home` |
| Metronome · Tap tempo | `C` · `T` |
| Session ↔ Arrange · Mixer | `Tab` · `Mod+Alt+M` |
| Toggle detail view · browser | `Mod+Alt+L` · `Mod+Alt+B` |
| Undo · Redo | `Mod+Z` · `Mod+Shift+Z` |
| Duplicate · Split · Consolidate | `Mod+D` · `Mod+E` · `Mod+J` |
| New audio · MIDI track · return | `Mod+T` · `Mod+Shift+T` · `Mod+Alt+T` |
| Mute track 1–8 · Solo selected | `F1`–`F8` · `S` |
| Zoom in / out · to selection · to song | `+` / `−` · `Z` · `Shift+Z` |
| Piano roll: quantize · transpose | `Q` · `↑↓` (semitone), `Shift+↑↓` (octave) |
| New · Open · Save project | `Mod+N` · `Mod+O` · `Mod+S` |
| Export mix | `Mod+Shift+E` |

## Feedback

Found a bug or have an idea? [Open an issue](https://github.com/leizzo/papercut/issues).

## License

Papercut's source code is released under the [MIT License](LICENSE).

---

<a id="türkçe"></a>

# Papercut (Türkçe)

[English](#papercut) · **Türkçe**

Elektronik müzik prodüktörleri ve miks mühendisleri için koyu temalı, yoğun ve klavye dostu bir
masaüstü DAW.

> **Durum: alfa.** Papercut erken geliştirme aşamasında. Güncel sürüm
> [v0.1.0 — M1 Core](https://github.com/leizzo/papercut/releases/tag/v0.1.0), alfa ön sürümü olarak
> yayımlandı. Eksik özellikler, pürüzler ve proje formatında değişiklikler olabilir — şarkınızın tek
> kopyasını henüz ona emanet etmeyin.

## Papercut nedir?

Papercut, fikirden yapıya, yapıdan mikse tek bir pencerede ilerlemenizi sağlar. Temel ilkesi:
**ses tasarımı cihaz zincirinde, miks mikserde yapılır** — iki ayrı zincir ve her zaman görebildiğiniz
bir sinyal yolu.

## Bugün neler yapabilirsiniz (v0.1.0)

- **Aranje** — ses ve MIDI kanalları; taşıyabildiğiniz, boyutlandırabildiğiniz, bölebildiğiniz,
  çoğaltabildiğiniz, döngüyle uzatabildiğiniz ve birleştirebildiğiniz klipler; zoom, kanal yüksekliği
  ve Follow.
- **Kayıt** — giriş seçimi, canlı dalga formu ve take'lerle ses kaydı; MIDI girişinden MIDI kaydı;
  count-in (atlamak için Rec'e Shift ile tıklayın).
- **MIDI düzenleme** — piano roll'da nota düzenleme ve quantize.
- **Ses tasarımı** — detay görünümünde, her kanal için eklentili cihaz zinciri.
- **Miks** — ses seviyesi, pan, mute, solo, stereo metreler; kanal başına bypass ve sıralama destekli
  8 insert slotu.
- **Kütüphane** — arama, kategoriler, sample önizleme ve kanala sürükle-bırak içeren kütüphane tarayıcısı.
- **Güvenlik** — otomatik kayıt ve çökme sonrası kurtarma.

Tam liste için [CHANGELOG.md](CHANGELOG.md) dosyasına bakın.

## Yol haritası

| Kilometre taşı | Getirdikleri |
|---|---|
| **M1 — Core** ✅ | Kabuk, transport, Arrangement, eklentili cihaz zinciri, temel Mixer, tasarım sistemi (v0.1.0) |
| **M1.1 — Cihazlar ve Eklentiler** | Yerleşik cihazlar (EQ Eight, Compressor), çökme izolasyonlu VST3 / AU / CLAP desteği, eklenince açılan eklenti penceresi |
| **M2 — Miks** | Pre-FX / Pre / Post send'ler, return kanalları, master loudness, klasörler ve bus'lar, sidechain girişleri |
| **M3 — Otomasyon** | Arrangement otomasyon şeritleri, klip üzeri zarflar, Read / Touch / Latch / Write |
| **M4 — Editörler** | Gam ve akor destekli, velocity şeritli piano roll; warp ve fade destekli ses editörü; klip zarfları |
| **M5 — Rack'ler ve Session** | Instrument / Drum / Audio Effect rack'leri, makrolar, sahneli Session görünümü, crossfader |

Hedef platformlar: macOS 13+ (Apple Silicon) ve Windows 11 x64. Alfa sürümü şu an macOS'ta derleniyor.

## Başlarken

Henüz hazır indirilebilir sürüm yok — kaynaktan derleyin ([Geliştiriciler için](#for-developers)
bölümüne bakın), ardından `Papercut.app`'i açın.

### Klavye kısayolları

`Mod` = macOS'ta Cmd, Windows'ta Ctrl.

| İşlem | Kısayol |
|---|---|
| Çal / Durdur · Seçimden çal | `Space` · `Shift+Space` |
| Kayıt | `F9` (count-in ile; atlamak için Rec'e Shift ile tıklayın) |
| Seçimi döngüye al · Başa dön | `Mod+L` · `Home` |
| Metronom · Tap tempo | `C` · `T` |
| Session ↔ Arrange · Mixer | `Tab` · `Mod+Alt+M` |
| Detay görünümü · tarayıcıyı aç/kapa | `Mod+Alt+L` · `Mod+Alt+B` |
| Geri al · Yinele | `Mod+Z` · `Mod+Shift+Z` |
| Çoğalt · Böl · Birleştir | `Mod+D` · `Mod+E` · `Mod+J` |
| Yeni ses · MIDI kanalı · return | `Mod+T` · `Mod+Shift+T` · `Mod+Alt+T` |
| 1–8. kanalı sustur · Seçiliyi solo | `F1`–`F8` · `S` |
| Yakınlaş / uzaklaş · seçime · şarkıya | `+` / `−` · `Z` · `Shift+Z` |
| Piano roll: quantize · transpoze | `Q` · `↑↓` (yarım ses), `Shift+↑↓` (oktav) |
| Yeni · Aç · Projeyi kaydet | `Mod+N` · `Mod+O` · `Mod+S` |
| Miksi dışa aktar | `Mod+Shift+E` |

## Geri bildirim

Bir hata mı buldunuz ya da bir fikriniz mi var? [Issue açın](https://github.com/leizzo/papercut/issues).

## Lisans

Papercut'ın kaynak kodu [MIT Lisansı](LICENSE) ile yayımlanmıştır.

---

<a id="for-developers"></a>

# For developers · Geliştiriciler için

*Teknik bölüm, komut ve kod terimleri ortak olduğu için İngilizce tutulmuştur.*

Papercut is built on [JUCE](https://juce.com) + [Tracktion Engine](https://github.com/Tracktion/tracktion_engine)
+ [GIN](https://github.com/FigBug/Gin). [PRD.md](PRD.md) is the source of truth for product behaviour;
[CONTEXT.md](CONTEXT.md) defines the domain language.

## Dependencies (pinned git submodules)

| Path | Pin |
|---|---|
| `external/tracktion_engine` | Tracktion Engine 3.5.0, commit `964583ee` (3.5.0 plus an upstream fix for a null ProjectItem crash when saving an Edit outside a Tracktion project) |
| `external/tracktion_engine/modules/juce` | JUCE 8.0.13 (`8.0.13-7-g37c894f8`), pinned by Tracktion |
| `external/gin` | GIN, commit `ea795541`; modules `gin`, `gin_gui`, `gin_svg`, `gin_plugin`, plus `gin_dsp`, `gin_graphics`, and `gin_simd` (required by `gin_plugin`) |

## Build (macOS)

Requires CMake ≥ 3.22, Ninja and Xcode command-line tools (C++20).

```sh
git submodule update --init external/gin external/tracktion_engine
# Tracktion's .gitmodules points JUCE at an SSH URL; use HTTPS unless you have GitHub SSH keys:
git -C external/tracktion_engine config submodule.modules/juce.url https://github.com/juce-framework/JUCE.git
git -C external/tracktion_engine submodule update --init modules/juce

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

- App: `build/Papercut_artefacts/Debug/Papercut.app`
- Tests (headless, no audio device): `build/PapercutTests_artefacts/Debug/PapercutTests [suite-name-filter]`, or `ctest --test-dir build`

## Layout

```
Source/Engine/    EngineManager, ProjectManager, ApplicationModel facade — the only code that sees Tracktion
Source/Commands/  Command registry, model Commands, ApplicationCommand ↔ Command ID table
Source/UI/        Theme, JSON layouts (ComponentFactory/LayoutManager), UI State, Arrangement, MainWindow
Source/App/       Application entry point
UI/layouts, UI/themes   Declarative UI files (embedded in Release; read from the source tree in Debug)
Tests/            Headless tests over the Command registry / Application Model seam
```

`Source/UI`, `Source/Commands` and `Source/App` are compiled without Tracktion on the include path, so
"UI never includes Tracktion headers" is a compile error rather than a convention.

## Developer Mode

In Debug builds, layouts and theme are read from `UI/` in the source tree. Edit a file, then:

- **Reload Layout** — Cmd+Alt+Shift+L: rebuilds only the regions whose layout file changed
- **Reload Theme** — Cmd+Alt+Shift+T: re-styles in place
- **Developer Overlay** — Cmd+Alt+Shift+D: shows the status bar and component inspector (hidden by default; the design has neither)

## Contributing

Issues are tracked on [GitHub](https://github.com/leizzo/papercut/issues). Releases follow
[Semantic Versioning](https://semver.org); each PRD milestone ships as a minor release until 1.0.0 —
see [CHANGELOG.md](CHANGELOG.md).

## License

Papercut's own code is [MIT](LICENSE). The pinned dependencies keep their own licenses: JUCE (AGPLv3 or
commercial), Tracktion Engine (GPLv3 or commercial) and GIN (BSD-3-Clause). A distributed Papercut
binary must also satisfy those terms.
