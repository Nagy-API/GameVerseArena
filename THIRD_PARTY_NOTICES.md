# Third-party notices

## SQLite

GameVerseArena's graphical profile system statically embeds the unmodified `sqlite3.c` and `sqlite3.h` files from the official SQLite 3.53.4 amalgamation archive `sqlite-amalgamation-3530400.zip`. The verified archive SHA3-256 is `628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e`. SQLite is dedicated to the public domain. Provenance details are recorded in `third_party/sqlite/README.md`.

## SFML

GameVerseArena's graphical target fetches and links SFML 3.1.0 (Graphics, Window, System, and Audio modules) from the [official SFML repository](https://github.com/SFML/SFML) through CMake FetchContent. SFML is licensed under the zlib/libpng license; its source distribution contains the complete license text.

SFML's own build fetches and statically links the following libraries into `GameVerseArenaGUI`. Each is used unmodified apart from SFML's build-configuration patches, and each source tree contains its complete license text:

| Library | Version | Used by | License |
| --- | --- | --- | --- |
| FreeType | 2.14.3 | text rendering | FreeType License (FTL) |
| HarfBuzz | 14.1.0 | text shaping | "Old MIT" license |
| SheenBidi | 3.0.0 | bidirectional text | Apache License 2.0 |
| miniaudio | bundled with SFML 3.1.0 | audio output | public domain (Unlicense) or MIT No Attribution, at the user's choice |
| libogg | 1.3.6 | SFML Audio (Ogg container support) | BSD-style, Copyright (c) 2002 Xiph.org Foundation |
| libvorbis | 1.3.7 | SFML Audio (Vorbis decoding) | BSD-style, Copyright (c) 2002-2020 Xiph.org Foundation |
| libFLAC | 1.5.0 | SFML Audio (FLAC decoding) | BSD-style (Xiph), Copyright (C) 2000-2009 Josh Coalson, 2011-2025 Xiph.Org Foundation |

GameVerseArena ships no audio files: every sound effect is generated procedurally in memory, so the Ogg, Vorbis, and FLAC decoders are linked because SFML's Audio module requires them, not to play bundled media.

## Inter

The graphical shell redistributes `Inter-Regular.ttf` and `Inter-SemiBold.ttf` from the official [Inter 4.1 release](https://github.com/rsms/inter/releases/tag/v4.1).

Copyright (c) 2016 The Inter Project Authors (https://github.com/rsms/inter)

Inter is licensed under the SIL Open Font License, Version 1.1. The complete license is included at `assets/fonts/LICENSE.txt`.
