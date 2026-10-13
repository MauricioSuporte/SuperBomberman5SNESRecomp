# Third-party notices

The game-side project was scaffolded from SNESRecomp. Existing Matthew Stanley copyright notices are preserved. Original contributions by this repository's contributors are covered by the root PolyForm Noncommercial 1.0.0 license. This license does not grant rights to Super Bomberman 5 or its data.

| Component | Source | License location |
|---|---|---|
| SNESRecomp | https://github.com/RetroPortingToolKit/snesrecomp | `snesrecomp/LICENSE` and component notices in that submodule |
| recomp-ui | https://github.com/RetroPortingToolKit/recomp-ui | `recomp-ui/LICENSE` and bundled third-party notices |
| Native analyzer | SNESRecomp `recompiler-rs` | `snesrecomp/LICENSE`; actual Cargo dependency license texts are bundled under `licenses/cargo/` in the release |
| Rust standard library linked into the analyzer | https://github.com/rust-lang/rust | `licenses/rust/COPYRIGHT-library.html` and its referenced license texts in the release |
| LLVM libunwind DLL, if bundled | RetroPorting Toolchains / llvm-mingw | `licenses/LLVM-LICENSE.txt` in the Windows release; source: https://github.com/llvm/llvm-project |
| Portable build tools, downloaded separately | https://github.com/RetroPortingToolKit/RetroPorting-Toolchains | Notices and licenses supplied with the official toolchain archive |

The lean Windows setup release does not bundle the portable toolchain. The framework downloads the official tools when needed. The setup executable, analyzer and source are built without using the game ROM. The player supplies their own ROM and generates game code locally.

No ROM, ROM-derived C, ROM-derived game executable, cartridge audio dump, extracted game graphics, save state or gameplay capture is included in this repository's release package. Dependency game screenshots and test ROM fixtures are excluded from the packaged source tree. Licensed launcher fonts, flags and interface assets retain their upstream notices.

Super Bomberman 5, Bomberman names and game content belong to their respective rights holders. This is an unofficial, noncommercial community project, without affiliation or endorsement.
