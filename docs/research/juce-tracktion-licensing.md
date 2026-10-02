# JUCE and Tracktion licences — what they require of Resamper

*Research note · 2026-10-02 · for the public License section. Resamper's own source is MIT; the app links pinned copies of JUCE, Tracktion Engine, and GIN.*

This is not legal advice. It records what the pinned licence files and the vendors' own pages say.

The copy that ships is the pinned tree. Where a live page uses different words, the pinned file governs this repository, and the live page is the vendor's current terms.

---

## What is pinned

| Piece | Where | Version in this tree |
|---|---|---|
| Resamper's own source | `LICENSE` at the repo root | MIT, Copyright (c) 2026 Ismail Bahtiyar [MIT] |
| Tracktion Engine | `external/tracktion_engine`, commit `964583ee940ee24d94bee7788d9e348d81e8d157` | 3.5.0, stated in `VERSION.md` and in the module header [TE-VER] |
| JUCE | `external/tracktion_engine/modules/juce`, commit `37c894f83d379179b2070d437ccd0f1cd9af9576` | Module header says 8.0.13; the commit is seven commits after that tag (`8.0.13-7-g37c894f83d`) [JUCE-VER] |
| GIN | `external/gin`, commit `ea79554126bf503be705d21f3998bdd7cdd59881` | Repo `LICENSE` is BSD-3-Clause; the `gin` module header says `license: BSD`. CMake links that module only [GIN][CMAKE] |

The app is one program. `resamper_engine` is a static library that links `tracktion::tracktion_engine` and `gin`, and the app links that library [CMAKE]. Tracktion's own module header lists JUCE modules as dependencies [TE-VER].

The public License sections in `README.md` and `README.tr.md` now say that MIT covers Resamper's own source, that a build also carries JUCE and Tracktion Engine, and that a closed binary needs the distributor's own commercial licences [README].

---

## 1. What the pinned JUCE and Tracktion licences offer

Both are dual licences: a copyleft grant, or a separate commercial contract with the vendor. They are not one licence, and neither vendor owns the other's code.

**JUCE**, in the pinned `LICENSE.md`, is dual-licensed under the AGPLv3 and the commercial JUCE licence [JUCE-LIC]. The same file says the commercial terms apply when you are *not* licensing the modules under the AGPLv3: downloading, installing, using, or combining JUCE with other code then means you agree to the JUCE 8 End User Licence Agreement [JUCE-LIC]. The JUCE 8 source headers in this pin say the same choice in shorter form: the EULA, "**Or:** You may also use this code under the terms of the AGPLv3" [JUCE-HDR]. The pin says AGPLv3, not "or any later version" [JUCE-LIC].

**Tracktion Engine**, in the pinned `LICENSE.md`, "is published under a dual GPL3 (or later)/Commercial license" [TE-LIC]. The pinned README repeats "a GPL/Commercial license" and says there are multiple commercial tiers, with prices on the Tracktion developers page [TE-README]. The same README says Tracktion "is not part of JUCE nor owned by the same company", that a JUCE licence does not include Tracktion Engine, and that a Tracktion licence does not include JUCE [TE-README].

GIN is not part of that dual-licence choice. Its repo licence is the BSD-3-Clause licence: redistribution in source or binary is allowed if the copyright notice, conditions, and disclaimer are kept, and if the copyright holder's name is not used to endorse a derived product [GIN].

The live JUCE 8 EULA (fetched 2026-10-02, changelog through 17 July 2025) is the commercial contract. Its tiers are Starter, Indie, Pro, and Educational [JUCE-EULA]:

| | Starter | Indie | Pro | Educational |
|---|---|---|---|---|
| Annual revenue or funding limit | Up to $20,000 | Up to $300,000 | No limit | No limit |
| Monthly price per user | — | $40 | $175 | — |
| Perpetual price per user | Free | $800 | $3,500 | Free |

Educational use is limited to teaching and academic research, and "may not be used for any commercial, professional, promotional, or any other for-profit activity" [JUCE-EULA §1.2.4]. The revenue limit for a company is the revenue of that entity and its affiliates from all sources, not only revenue from the JUCE product [JUCE-EULA §1.2.1].

The live Tracktion developers page no longer prints those tiers. Its "License Plans" block is a sign-up link to `https://engine.tracktion.com/` [TE-PAGE]. That portal, on the same day, lists per-seat plans and says usage "is governed by this document or the GNU General Public License" [TE-PORTAL][TE-EULA]:

| | Personal | Indie | Pro 1 | Pro 2 | Pro 3 | Enterprise | Education |
|---|---|---|---|---|---|---|---|
| Price | Free | $35/month | $50/month | $150/month | $300/month | Contact | Free |
| Revenue or funding limit | $50k | $200k | $400k | $2M | $10M | No limit | No limit |
| Minimum commitment | None | 12 months | 12 months | 12 months | 12 months | 12 months | None |
| Branding | Required | Required | Optional | Optional | Optional | Optional | Required |

Personal, Indie, and Education require "Powered by Tracktion Engine" branding [TE-EULA]. The portal's FAQ states the revenue limit is that of the whole organisation, not sales of the Tracktion-based product alone [TE-PORTAL].

---

## 2. MIT on Resamper's files, and the combined binary

The MIT text grants, for "this software", the rights to "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies", provided the copyright and permission notices travel with the copies [MIT]. "This software" is Resamper's own files. The grant does not mention JUCE or Tracktion, and it cannot: those copyrights belong to Raw Material Software Limited and Tracktion Software Corporation [JUCE-EULA][TE-EULA].

Taken alone, the MIT files may be copied, modified, and sold under MIT, notices included [MIT]. A binary built from this repository is not those files alone. It links Tracktion, which links JUCE [CMAKE][TE-VER].

The GNU FAQ states the consequence of linking a GPL library that is not under the LGPL: "the terms of the GPL apply to the entire combination. The software modules that link with the library may be under various GPL compatible licenses, but the work as a whole must be licensed under the GPL" [FAQ IfLibraryIsGPL]. GPLv3 says why, when the combination is conveyed:

> You must license the entire work, as a whole, under this License to anyone who comes into possession of a copy. This License will therefore apply … to the whole of the work, and all its parts, regardless of how they are packaged. This License gives no permission to license the work in any other way, but it does not invalidate such permission if you have separately received it. [GPL §5(c)]

AGPLv3 has the same paragraph [AGPL §5(c)]. The last sentence is what leaves the MIT permission in place on the MIT files: someone who receives those files under MIT still has that permission. It does not let the recipient convey the combined program under MIT only.

So a third party cannot ship a closed-source commercial product *built from this repository* on the strength of the MIT licence. They can sell and sublicense Resamper's own files. The combined binary is conveyable either under the copyleft terms below, or under commercial licences the third party obtains from each vendor (sections 4 and 6).

MIT is a GPL-compatible permissive licence. The FSF calls this text the Expat Licence and describes it as "a lax, permissive non-copyleft free software license, compatible with the GNU GPL" [FSF Expat]. The FAQ allows a module added to a GPL-covered program to stay under "a license which is more lax than the GPL but compatible with the GPL", while "the whole combined program has to be released under the GPL" [FAQ GPLModuleLicense]. That is the shape of this repo: MIT on Resamper's files, copyleft on the combination when it is conveyed.

---

## 3. Commercial use, and closed-source distribution

The copyleft forbids a closed conveyance. It does not forbid commercial use.

GPLv3, and AGPLv3 in the same words:

> This License explicitly affirms your unlimited permission to run the unmodified Program. … You may make, run and propagate covered works that you do not convey, without conditions so long as your license otherwise remains in force. [GPL §2][AGPL §2]

"Propagate" includes distribution. "Convey" means propagation that enables other parties to make or receive copies. Executing the program on a computer, and modifying a private copy, are outside propagation [GPL §0]. The FAQ says a company may run a modified GPL program without releasing the source when it never distributes that version [FAQ UnreleasedMods], and that copies made inside one organisation are not distribution [FAQ InternalDistribution]. Tracktion's own FAQ says the same thing in its words: in-house tools and internal development of pre-release software, before copies go to external testers, are "usually permitted under the GPLv3"; a Tracktion licence is required, if you are not on the GPLv3 path, "for at least the duration over which you are distributing closed-source binaries" [TE-PORTAL].

Charging money for a copy is allowed on the copyleft path. GPLv3: "You may charge any price or no price for each copy that you convey" [GPL §4]. The FAQ: "you are allowed to sell copies of the modified program commercially, but only under the terms of the GNU GPL", including making source available and allowing recipients to redistribute and modify [FAQ GPLCommercially][FAQ DoesTheGPLAllowMoney]. A licence that tried to forbid commercial use "is not a free software license" [FAQ NoMilitary].

Three situations follow:

1. **Run the app, including in paid studio work, without giving anyone a copy.** That is running, which §2 permits. No JUCE or Tracktion commercial licence is required for that.
2. **Distribute the binary and comply with the AGPL and the GPL**, including the corresponding-source offer in section 6. Selling that build is allowed. The commercial licences are still not required, because this is the copyleft side of each dual licence [JUCE-LIC][TE-LIC][TE-PORTAL].
3. **Distribute a closed binary**, with no AGPL/GPL source obligations. That is the commercial side. The distributor needs a JUCE commercial licence *and* a Tracktion commercial licence. Some of those tiers are free within a revenue cap (JUCE Starter, Tracktion Personal); they are still vendor licences, and MIT does not grant them [JUCE-EULA §1.2][TE-PORTAL].

"Anyone cannot use this commercially unless those commercial licences are purchased first" covers situation 3 and overreaches situations 1 and 2. Commercial *use* and commercial *sale of a copyleft-compliant build* are both permitted. What the commercial licences buy is the right to distribute without the copyleft.

AGPLv3 adds one duty the GPL does not, and it is about remote interaction, not about payment. If you modify the program, a version that supports users "interacting with it remotely through a computer network" must offer those users the corresponding source [AGPL §13]. The FAQ applies that to a modified AGPL program run as a network service [FAQ UnreleasedModsAGPL]. A desktop binary that people run locally is still a conveyance when you ship the DMG (section 6). The network offer is an additional duty when remote interaction exists.

---

## 4. Who can take the commercial licences, and when

They can be taken before any sale. Nothing in either commercial agreement requires a completed sale first. A JUCE subscription fee is payable "from the first day of purchase" [JUCE-EULA §3.1]. A Tracktion subscription fee is payable the same way [TE-EULA §3.1]. The right those agreements then give is a right to distribute and sell *if you already adhere to the agreement* [JUCE-EULA §1.9][TE-EULA §1.4]. Tracktion's FAQ ties the commercial licence to the period in which closed binaries are distributed, which can be arranged before the first copy goes out [TE-PORTAL].

They are bought, or in the free tiers accepted, from the vendor. MIT cannot sublicense them.

- JUCE calls a product any combination of the framework with other code. The owner, "the entity or person legally responsible for the development or maintenance of the Product", "is responsible for the licensing of the Framework in relation to that Product" [JUCE-EULA §1.4–1.5]. Each person who modifies code that depends on JUCE needs a seat, provided by that owner [JUCE-EULA §1.6–1.7]. "You may not sell, sub-license, or otherwise Distribute the Framework, or any subset of the Framework, on its own" [JUCE-EULA §1.17]. On the commercial path the licensee also undertakes not to place the framework under an open-source licence that requires source disclosure [JUCE-EULA §2.3].
- Tracktion: "You may sell or distribute Applications using the Code … You may not sell, sublicense, or otherwise distribute the Code or Software on their own" [TE-EULA §1.4]. One user per seat [TE-EULA §1.2]. The portal FAQ: each developer needs their own seat; by default every developer who contributes source to software containing Tracktion Engine needs a licence [TE-PORTAL]. A seller who wrote none of the code does not, on that FAQ, need a personal Tracktion seat, but "must ensure that the software is appropriately licensed for the whole duration over which you are distributing it", and if contributors are third parties those licences must be perpetual rather than subscription [TE-PORTAL].

Someone who clones this repo and ships their own closed build is developing and distributing that build. They need their own JUCE licence (as product owner, with a seat per person modifying JUCE-dependent code) and their own Tracktion licence (a seat per developer contributing source, held for as long as the closed binaries are distributed). The MIT copyright holder does not have those seats to pass on. A later pure reseller of an already-licensed closed product is a different case: JUCE confers redistribution of *that* product on the recipient when the owner distributed it under the right number of seats [JUCE-EULA §1.11], and Tracktion's FAQ says the seller must still make sure the contributors' licences cover the distribution period [TE-PORTAL]. Neither of those is a sublicense hiding inside the MIT text.

A JUCE licence is not a Tracktion licence. The pinned Tracktion README says you need an appropriate JUCE licence from juce.com when distributing Tracktion Engine-based products, and that "Tracktion Engine is not included in a JUCE licence" [TE-README]. The two commercial agreements are with different companies [JUCE-EULA][TE-EULA].

---

## 5. AGPLv3, GPLv3, and BSD-3-Clause in one binary

JUCE arrives as AGPLv3 [JUCE-LIC]. Tracktion arrives as GPL-3.0-or-later [TE-LIC]. The "or later" lets a distributor of this tree use Tracktion under GPLv3 specifically, which is the version that speaks to the AGPL.

Each licence permits the combination. GPLv3:

> Notwithstanding any other provision of this License, you have permission to link or combine any covered work with a work licensed under version 3 of the GNU Affero General Public License into a single combined work, and to convey the resulting work. The terms of this License will continue to apply to the part which is the covered work, but the special requirements of the GNU Affero General Public License, section 13, concerning interaction through a network will apply to the combination as such. [GPL §13]

AGPLv3:

> Notwithstanding any other provision of this License, you have permission to link or combine any covered work with a work licensed under version 3 of the GNU General Public License into a single combined work, and to convey the resulting work. The terms of this License will continue to apply to the part which is the covered work, but the work with which it is combined will remain governed by version 3 of the GNU General Public License. [AGPL §13]

The AGPL also says that when the network source offer applies, that corresponding source includes the GPLv3 work brought in by the paragraph above [AGPL §13]. The FAQ states the result directly: "You can always link GPLv3-covered modules with AGPLv3-covered modules, and vice versa. That is true regardless of whether some of the modules are libraries" [FAQ AGPLGPL]. The "notwithstanding" wording exists so the AGPL's extra requirements are not treated as forbidden further restrictions under GPLv3 §7 [FAQ v3Notwithstanding]. The compatibility matrix says that wherever it states a result for GPLv3, "the same statement about compatibility is true for AGPLv3 as well" [FAQ AllCompatibility].

In this binary, the JUCE parts stay under the AGPL, the Tracktion parts stay under GPLv3, and the AGPL's network condition in §13 applies to the combination. Conveying the whole program has to satisfy both licences at once [FAQ WhatIsCompatible].

GIN's BSD-3-Clause text has no advertising clause [GIN]. The FSF calls that text the Modified BSD licence, "sometimes referred to as the 3-clause BSD license", and "a lax, permissive non-copyleft free software license, compatible with the GNU GPL" [FSF ModifiedBSD]. The original 4-clause BSD is the one the FAQ treats as GPL-incompatible, because of the advertising clause; "the revised BSD license does not have the advertising clause, which eliminates the problem" [FAQ OrigBSD]. GIN can sit inside the AGPL/GPL combination. Its own conditions still apply to the GIN code: keep the notice in source redistributions, reproduce it with binaries, and do not use the copyright holder's name to endorse the product [GIN]. BSD does not impose a copyleft of its own, and it does not replace the AGPL or the GPL on the rest of the binary [FAQ WhatDoesCompatMean][FAQ GPLModuleLicense].

---

## 6. The open DMG, and a later closed build

**Publishing the open-source DMG** is conveying object code. On the copyleft side of both dual licences, the maintainer does not need a paid JUCE or Tracktion seat to do that [JUCE-LIC][TE-PORTAL]. The conveyance still has conditions.

GPLv3 and AGPLv3 allow object code only if you also convey the machine-readable corresponding source, in one of the ways §6 lists [GPL §6][AGPL §6]. Corresponding source is "all the source code needed to generate, install, and (for an executable work) run the object code and to modify the work", including shared libraries and dynamically linked subprograms the work is designed to require, and excluding system libraries and general-purpose tools that are not part of the work [GPL §1]. For a DMG offered as a download, §6(d) is the matching option: offer the object code from a designated place, gratis or for a charge, and offer equivalent access to the corresponding source at no further charge, with clear directions next to the object code if the source is on another server. The source has to stay available for as long as needed to satisfy that section [GPL §6(d)].

For this app that source is the Resamper tree at the release, including the pinned Tracktion, JUCE, and GIN sources that the binary is built from. The MIT files by themselves are not the corresponding source of the binary.

The combined work has to be licensed as a whole under those terms to anyone who receives a copy [GPL §5(c)][AGPL §5(c)]. Notices stay: the MIT notice on Resamper's files [MIT], the AGPL and GPL texts, and the BSD notice with the binary as GIN requires [GIN][GPL §4]. Section 5(d) requires appropriate legal notices in interactive interfaces only when the program you started from already displays them [GPL §5(d)].

The AGPL network paragraph in section 3 is additional to this. Shipping the DMG does not, by itself, depend on it.

**Selling a closed build later** means leaving the copyleft path. The party who develops and distributes that build takes the commercial side of each dual licence, from each vendor, before or at the time closed distribution starts, and keeps it for as long as closed distribution continues [JUCE-EULA §1.9–1.10][TE-PORTAL][TE-EULA §1.4]. JUCE and Tracktion are separate purchases or acceptances [TE-README]. Free tiers exist, inside their revenue caps, and they are still those vendors' licences [JUCE-EULA §1.2][TE-PORTAL]. Exceeding a cap means moving to a higher tier, or stopping closed distribution; Tracktion's agreement also allows relicensing the applications under the GPL instead [JUCE-EULA §1.14][TE-EULA]. The MIT licence on Resamper's files can remain the licence of those files inside the closed product. It does not supply the JUCE or Tracktion grant.

Both pins also carry other notices the distributor keeps. JUCE's `LICENSE.md` lists them, including the VST3 SDK (MIT) and the AudioUnit SDK (Apache 2.0) [JUCE-LIC]. This tree turns on VST3 hosting, and AU hosting on macOS; it does not turn on AAX or ASIO [CMAKE]. Tracktion's README lists further libraries and says to adhere to their terms where necessary [TE-README].

---

## 7. Wording for the public License section

The public section should say that Resamper's own source is MIT, that the linked libraries keep their own licences, and that a closed binary needs the distributor's own commercial licences. It should also say that running the app, and distributing a copyleft-compliant build, remain allowed. `README.md` and `README.tr.md` now say that. The draft below is the wording this note recommended; the published paragraphs are the same points in the README's own sentences.

English:

```markdown
## License

Resamper's own source is released under the [MIT License](LICENSE)
(Copyright © 2026 Ismail Bahtiyar). The application also links JUCE
(AGPLv3, or a commercial JUCE licence) and Tracktion Engine (GPLv3 or
later, or a commercial Tracktion licence). GIN is BSD-3-Clause.

You may run the combined program, including for paid studio work. You
may also distribute it, if that distribution complies with the AGPL and
the GPL and offers the corresponding source. A closed-source binary
requires the distributor's own commercial JUCE licence and own
commercial Tracktion Engine licence. The MIT grant does not include
either.
```

Turkish, for the public `## Lisans` section:

```markdown
## Lisans

Resamper'ın kendi kaynak kodu [MIT Lisansı](LICENSE) ile yayımlanmıştır
(Copyright © 2026 Ismail Bahtiyar). Uygulama ayrıca JUCE'u (AGPLv3 veya
ticari bir JUCE lisansı) ve Tracktion Engine'i (GPLv3 veya sonraki bir
sürümü, ya da ticari bir Tracktion lisansı) bağlar. GIN, BSD-3-Clause
lisanslıdır.

Birleşik programı çalıştırmak serbesttir; ücretli stüdyo kullanımı buna
dahildir. Programı dağıtmak da serbesttir, yeter ki dağıtım AGPL ve GPL
koşullarına uysun ve karşılık gelen kaynak sunulsun. Kapalı kaynak bir
ikili dağıtmak için dağıtıcının kendi ticari JUCE lisansı ve kendi
ticari Tracktion Engine lisansı gerekir. MIT metni bu iki lisansı
kapsamaz.
```

---

## Where the live pages and the pins differ

The pinned files govern the code in this repository. The live pages are the vendors' current terms.

- **JUCE.** The pin offers AGPLv3 or the commercial EULA, and says the EULA applies if you are not licensing under the AGPLv3 [JUCE-LIC][JUCE-HDR]. The live EULA's opening says that downloading, installing, or using the framework agrees you to that agreement, and the EULA text itself does not restate the AGPL alternative [JUCE-EULA]. The AGPL choice for this tree is the one written in the pin.
- **Tracktion.** The pin says "GPL3 (or later)" or commercial [TE-LIC]. The developers page no longer states that; it points at the pricing portal [TE-PAGE]. The portal FAQ says Tracktion "is dual licensed under both the Tracktion Engine licence and the GPLv3", without the words "or later" [TE-PORTAL]. The commercial agreement's notice says usage "is governed by this document or the GNU General Public License" [TE-EULA].
- **Prices and tier names** in section 1 are from the live JUCE EULA and the live Tracktion portal on 2026-10-02. They are not copied into the pinned `LICENSE.md` files, which only point at the vendors for commercial terms [JUCE-LIC][TE-LIC].

---

## Sources

Pinned in this repository:

- **[MIT]** `LICENSE` — MIT License, Copyright (c) 2026 Ismail Bahtiyar.
- **[README]** `README.md` — public License section, and the developer License section.
- **[TE-LIC]** `external/tracktion_engine/LICENSE.md` — GPL-3.0-or-later or commercial.
- **[TE-README]** `external/tracktion_engine/README.md` — License section, including the note that JUCE and Tracktion are licensed separately. This is the README at pin `964583ee`.
- **[TE-VER]** `external/tracktion_engine/VERSION.md` and `modules/tracktion_engine/tracktion_engine.h` — version 3.5.0.
- **[JUCE-LIC]** `external/tracktion_engine/modules/juce/LICENSE.md` — AGPLv3 or the JUCE 8 commercial licence, and the dependency list.
- **[JUCE-HDR]** `external/tracktion_engine/modules/juce/CMakeLists.txt` — the "Or: AGPLv3" header in the pin.
- **[JUCE-VER]** `external/tracktion_engine/modules/juce/modules/juce_core/juce_core.h` — version 8.0.13. Git pin `37c894f83d` (`8.0.13-7-g37c894f83d`).
- **[GIN]** `external/gin/LICENSE` — BSD-3-Clause, Copyright (c) 2018 Roland Rabien. Module header: `external/gin/modules/gin/gin.h` (`license: BSD`).
- **[CMAKE]** `CMakeLists.txt` — `resamper_engine` links Tracktion and `gin`; `JUCE_PLUGINHOST_VST3` and `JUCE_PLUGINHOST_AU`.

Live vendor pages, fetched 2026-10-02:

- **[JUCE-EULA]** JUCE 8 End User Licence Agreement. https://juce.com/legal/juce-8-licence/
- **[TE-PAGE]** Tracktion, *Develop with Tracktion Engine*. https://www.tracktion.com/develop/tracktion-engine
- **[TE-PORTAL]** Tracktion Engine pricing and licensing FAQ. https://engine.tracktion.com/
- **[TE-EULA]** Tracktion Engine End User License Agreement. https://engine.tracktion.com/agreement

GNU, fetched 2026-10-02:

- **[GPL]** GNU General Public License, version 3, §§0–2, 4–6, 13. https://www.gnu.org/licenses/gpl-3.0.html
- **[AGPL]** GNU Affero General Public License, version 3, §§2, 5, 6, 13. https://www.gnu.org/licenses/agpl-3.0.html
- **[FAQ WhatDoesCompatMean]** https://www.gnu.org/licenses/gpl-faq.html#WhatDoesCompatMean
- **[FAQ WhatIsCompatible]** https://www.gnu.org/licenses/gpl-faq.html#WhatIsCompatible
- **[FAQ IfLibraryIsGPL]** https://www.gnu.org/licenses/gpl-faq.html#IfLibraryIsGPL
- **[FAQ GPLModuleLicense]** https://www.gnu.org/licenses/gpl-faq.html#GPLModuleLicense
- **[FAQ LinkingWithGPL]** https://www.gnu.org/licenses/gpl-faq.html#LinkingWithGPL
- **[FAQ GPLCommercially]** https://www.gnu.org/licenses/gpl-faq.html#GPLCommercially
- **[FAQ DoesTheGPLAllowMoney]** https://www.gnu.org/licenses/gpl-faq.html#DoesTheGPLAllowMoney
- **[FAQ NoMilitary]** https://www.gnu.org/licenses/gpl-faq.html#NoMilitary
- **[FAQ UnreleasedMods]** https://www.gnu.org/licenses/gpl-faq.html#UnreleasedMods
- **[FAQ UnreleasedModsAGPL]** https://www.gnu.org/licenses/gpl-faq.html#UnreleasedModsAGPL
- **[FAQ InternalDistribution]** https://www.gnu.org/licenses/gpl-faq.html#InternalDistribution
- **[FAQ AGPLGPL]** https://www.gnu.org/licenses/gpl-faq.html#AGPLGPL
- **[FAQ v3Notwithstanding]** https://www.gnu.org/licenses/gpl-faq.html#v3Notwithstanding
- **[FAQ AllCompatibility]** https://www.gnu.org/licenses/gpl-faq.html#AllCompatibility
- **[FAQ OrigBSD]** https://www.gnu.org/licenses/gpl-faq.html#OrigBSD
- **[FSF Expat]** GNU license list, *Expat License*. https://www.gnu.org/licenses/license-list.html#Expat
- **[FSF ModifiedBSD]** GNU license list, *Modified BSD license*. https://www.gnu.org/licenses/license-list.html#ModifiedBSD
