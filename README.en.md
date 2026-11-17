<p align="center"><img src="docs/assets/social-preview.jpg" alt="28 open-source native Photoshop filters inspired by After Effects" width="1280"></p>

# AE-to-PS-Plugins

**28 free, open-source native Photoshop filters for glow, chromatic dispersion, energy, glass, procedural textures and distortion.**

English · [简体中文](README.md)

![Release](https://img.shields.io/badge/release-0.38-blue) ![Platform](https://img.shields.io/badge/Windows-x64-0078D4) ![Code license](https://img.shields.io/badge/code-MIT-green)

Independent implementations inspired by familiar After Effects effects. Work inside Photoshop with editable Smart Filters, Chinese/English controls, individual parameter resets and fast previews that refine after dragging stops.

**Stable 0.38** adds Halftone, ASCII Dither, Pixel Dither, Lens Flare Designer and Etching, plus appearance-preserving Starglow/Shine performance improvements. Previous stable releases remain available.

**[Download Windows plugins](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/download/v0.38/AE-to-PS-Plugins-0.38-Windows-x64.zip)** · [Release notes](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/tag/v0.38) · **[All 28 parameter-window examples](docs/GALLERY.md)** · [Effect catalog](docs/EFFECTS.md)

## Parameter-window examples

### Etching · Green Engraving

![Etching effect and parameter window](docs/assets/effects/etching.png)

### Lens Flare Designer

![Lens Flare designer and parameter window](docs/assets/effects/lens-flare.png)

Actual production parameter windows show the effect preview together with its configured controls. The cover is unchanged; see [all 23 windows](docs/GALLERY.md) for inputs and settings. The standalone capture tool uses the same production dialog and preview code without changing personal Photoshop documents.

### Deep Glow

![Deep Glow parameter window and effect preview](docs/assets/effects/deep-glow.png)

### Saber energy

![Saber parameter window and effect preview](docs/assets/effects/saber.png)

## Install

1. Download **Windows-x64.zip** above. “Code → Download ZIP” contains source code and requires compilation.
2. Quit Photoshop, back up previous plugins, and copy the required `.8bf` files from `Plug-ins` into Photoshop’s `Plug-ins` folder. Avoid duplicate copies across plugin folders.
3. Start Photoshop and use **Filter → ZaYu-AE Recreated Effects**. The menu name follows the host language.
4. For editable results, convert the layer to a **Smart Object** before applying a filter, then save the PSD. Each instance stores its own parameters.

Both Windows and Photoshop must be 64-bit. End users do not need After Effects, original third-party effects, the SDK or Python.

## Included effects

| Category | Filters |
| --- | --- |
| Light | Glow, Deep Glow, Starglow, Shine, Saber, Light Sweep |
| Color and lens | Tint, Chromatic Dispersion, Optics Compensation, Lens, Lens Flare Designer |
| Print and stylization | Halftone, ASCII Dither, Pixel Dither, Etching |
| Textures | Fractal Noise, Cell Pattern, Grid, Checkerboard, JPEG Damage |
| Distortion and surfaces | Displacement Map, Vector Blur, Turbulent Displace, Blobbylize, Glass, Kaleida |
| Edges and masks | Roughen Edges, Venetian Blinds |

Chromatic Dispersion includes spectral warp and prism defocus modes. Saber supports endpoints, paths, Alpha inputs and 16 original presets. Most filters support RGB8/16/32; Tint currently supports RGB8 only.

## What changed in 0.38

- **[Halftone](docs/HALFTONE_LOCAL.md)**: round, square, diamond and line screens, with duotone or transparent output.
- **[ASCII Dither](docs/ASCII_LOCAL.md)**: four character sets, system fonts, color options and dithering.
- **[Pixel Dither](docs/DITHER_LOCAL.md)**: ordered patterns, Floyd–Steinberg, Atkinson and retro palettes.
- **[Lens Flare Designer](docs/FLARE_DESIGNER_LOCAL.md)**: twelve element types, visual insertion, drag ordering, guarded shortcuts, undo/redo, twelve original presets and portable preset files. Cached 640×400 previews support hover enlargement.
- **[Etching](docs/ETCHING_LOCAL.md)**: variable-width line sets, wave/luminance distortion, contours, ink/paper colors and six engraving looks.
- **Starglow/Shine performance**: parallel rotated-star rendering and cached ray stages for opacity/blending edits, retaining appearance. [Measurements](docs/LIGHTFX_PERFORMANCE_LOCAL.md)

[Full 0.38 release notes](docs/RELEASE_0.38.md). Install from Windows-x64.zip; source archives require compilation.

## Compatibility and limits

- **PS2020 21.2.9 on the development Windows 11 PC:** output, undo, persistence, new-process reopening and recalculation for the five additions; pixel comparisons for Starglow/Shine. The other 21 binaries are unchanged from 0.37.
- **PS2026 27.7.0:** this package is installed; no new full native regression claim for this round.
- **Windows 7 + CC2018:** the user confirmed the earlier local 0.7 bundle works. Compatibility paths are retained; version0.38 still needs external retesting.
- Other combinations are not comprehensively tested.

Lens Flare uses element snapshot format 2 starting with this release; earlier unpublished experimental PSDs/presets are not a compatibility target.

Effects use the **CPU**; the GPU draws the UI and displays previews. These are independent approximations, with no guarantee of pixel or random-seed equivalence to the original effects. Some effects cannot expand the canvas. Changes to external Glass/Blobbylize reference layers require reopening the filter. Previously unsaved parameters cannot be recovered retroactively.

[Validation scope](docs/VALIDATION.md) · [Parameter persistence](docs/PARAMETER_PERSISTENCE.md)

## Build and contribute

Development stays local until the maintainer explicitly requests a GitHub update. Publications are stable releases. [Release policy](docs/RELEASE_POLICY.md)


C++20, LLVM-MinGW, Win32, Dear ImGui and D3D11; 21 modules. CPU tests do not require Photoshop or its SDK:

```powershell
./build.ps1 -CoreOnly -Toolchain 'C:/Dev/llvm-mingw/bin'
```

A full build requires a separately obtained Photoshop C++ SDK. [Build guide](docs/BUILDING.md) · [Architecture](docs/ARCHITECTURE.md) · [Contributing](CONTRIBUTING.md)

[Report an issue or suggest an effect](https://github.com/Three-LEI/AE-to-PS-Plugins/issues/new/choose). If the plugins help your work, a Star makes updates easier to find.

## License

Project code is [MIT licensed](LICENSE). See [third-party notices](THIRD_PARTY.md) and the separate [demo asset notice](docs/assets/README.md). The photo and photo-containing demonstrations are excluded from the code’s MIT license. No Adobe SDK, proprietary effect binaries or Windows fonts are bundled. This project is not affiliated with or endorsed by Adobe or the original effect vendors.

by 淚泪泪 / Three-LEI
