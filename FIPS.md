# HarfBuzz fips module

This fork exports `harfbuzz` from `fips-files/harfbuzz` and imports
`kochol/fips-freetype`. The upstream root CMake and Meson builds are unchanged.

## Import

```yaml
imports:
    fips-harfbuzz:
        git: https://github.com/kochol/fips-harfbuzz.git
    fips-icu:
        git: https://github.com/kochol/fips-icu.git
```

The application calls `fips_setup()` once in its root CMakeLists.txt after
`project(...)`. Then use the [standard fips macros](https://floooh.github.io/fips/docs/cmakeguide/):

```cmake
fips_begin_app(example cmdline)
    fips_files(main.cpp)
    fips_deps(harfbuzz freetype icu)
fips_end_app()
```

If the application's fips.yml uses `no_auto_import: true`, call these before
declaring the application:

```cmake
fips_import_fips_freetype_freetype()
fips_import_fips_harfbuzz_harfbuzz()
fips_import_fips_icu_icu()
```

Header directories and static dependencies propagate through targets. The
`harfbuzz::harfbuzz` alias is also available. The adapter reuses upstream's CMake
target to preserve generated headers and platform checks instead of maintaining
a second source list. It does not call fips_setup again inside an import.

## Profile

Static core shaping library, including hb-ft FreeType interop. Utilities,
subsetting, experimental raster/vector/GPU libraries, platform shapers and
Cairo/GLib/Graphite integrations are disabled. HarfBuzz's built-in Unicode
properties remain enabled; ICU supplies paragraph bidi, boundaries and locale
services independently. No harfbuzz-icu bridge is required for this arrangement.

These modules prepare the dependencies; they do not add text layout or a renderer
to CGE. Import them only in the client's build branch.

## Cross-library verification

Clone all three fips forks as siblings, then run from fips-harfbuzz:

```sh
cmake -S fips-files/smoke -B ../fips-build/text-smoke
cmake --build ../fips-build/text-smoke --config Release --parallel
ctest --test-dir ../fips-build/text-smoke -C Release --output-on-failure
```

To exercise fips itself, clone fips as another sibling and add
`-DFIPS_TEXT_USE_FIPS=ON -DFIPS_CONFIG=win64-vstudio-release` (or
`linux-ninja-release`) to configure. This uses `fips_setup`, `fips_begin_app`,
`fips_files`, `fips_deps`, and `fips_end_app` directly.

For a single-configuration generator, add `-DCMAKE_BUILD_TYPE=Release` to configure.
The test uses this repository's Amiri test font, not an installed system font.
It checks Arabic/Persian contextual shaping and rasterization, mixed RTL/LTR
runs, grapheme boundaries, line breaking, and Persian number formatting.
The test links to embedded ICU data; no external ICU installation is required.

Builds use the local checkout. Pin integration commits in consumers after these
forks are committed/pushed. Tested base: `873dbc1e3`.
