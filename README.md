# 3 KNOB — by Leo Gallagher

**AU effect for Logic Pro (buildable macOS source, version 1.0.0).**

**Important:** The package does *not* contain a precompiled `.component`. An Audio Unit is a native macOS binary and cannot be compiled or host-validated in the Linux environment where the project was created. The included macOS build script compiles and installs the actual AU on your Mac. Do not rename the source ZIP to `.component`.

## What you get

A compact effect named **3 KNOB**, published under **Leo Gallagher** in Logic's Audio Units menu.

- **SUN** — smooth tonal contour: fuller low body, a little less lower-mid clutter, musical presence, rounded top end.
- **GOLD** — frequency-conscious low/mid/high harmonic saturation. No convolution, IRs, or artificial hiss.
- **GLOW** — attack-responsive, decaying harmonic bloom on upper-mid material. No reverb or delay.
- **BYPASS** — audible click-resistant wet/dry crossfade; unaffected signal after fade finishes.

Three knob values are 0–100. Starting values: SUN 55, GOLD 35, GLOW 25. The plugin stores knob and bypass settings in Logic projects and allows host automation.

It is an AU **effect**, not a software instrument or MIDI effect. Mono or stereo, in/out channel counts must match. Float processing only, no lookahead and no intentional latency. CPU-conscious: four biquads per channel, two inexpensive one-pole crossovers, a small algebraic soft saturator, and envelope followers. There is no oversampling or analysis visualization, to keep CPU demand low. Extreme drive can produce some aliasing; use restraint with bright high-frequency sources.

## Build and install on your Mac

You need **macOS**, **Apple Command Line Tools** (not the full Xcode app), **CMake 3.22 or later**, plus internet access once to download JUCE 8.0.15 (unless you have put a local JUCE source checkout in this folder). **Alternatively, use the cloud build below with NO Mac development tools installed.**

1. Unzip the source archive to your Mac.
2. In Terminal, run `xcode-select --install` to install Apple Command Line Tools, if they are not already installed. **Do not install the full Xcode app.**
3. Install CMake (download its macOS `.dmg` from cmake.org and enable its command-line tools, or use `brew install cmake`). Check with `cmake --version`.
4. Double-click **`build-mac.command`**. If your Mac blocks the script, right-click it and choose **Open**, or run `bash build-mac.command` from Terminal in this directory.
5. The script tests the audio engine, downloads the JUCE framework on first build, compiles the AU using Apple clang + Makefiles (without full Xcode), places it in `~/Library/Audio/Plug-Ins/Components/3 KNOB.component`, locally ad-hoc signs it, and attempts an `auval` validation.
6. Relaunch Logic Pro. Insert it on an audio/instrument track: **Audio FX → Audio Units → Leo Gallagher → 3 KNOB**.

If the plugin does not appear: in Logic Pro open **Settings → Plug-in Manager**, find **3 KNOB**, then try **Reset & Rescan Selection**. Check that the `.component` exists at the install path and that `auval -v aufx Knob Lega` succeeds in Terminal. AU registration sometimes requires quitting and reopening Logic.

**Do not run `sudo` on the build script.** It installs for the current user, not the entire machine.

## Cloud build — no Xcode, compiler, or CMake on your Mac

**Best when disk space is scarce (especially on an Intel Mac running Monterey 12.7.4).** GitHub Actions compiles the Audio Unit on a hosted Intel Mac and produces the actual `3 KNOB.component` file for you. A free GitHub account is needed; a public repository uses free standard hosted runners, while private repositories have usage limits.

1. Create a new repository at https://github.com/new . A *public* repository is simplest, but it makes this project's source visible to everyone; choose private if you prefer. A README in the initial repository is optional.
2. Open the repository's **Add file > Upload files** page. Drag the **contents of this extracted project folder** (not the ZIP) into the browser. Make sure `.github/workflows/build-3-knob.yml` is included. On a Mac, hidden files are displayed in Finder with `Command + Shift + .` . Commit directly to the default branch, usually `main`.
3. Click **Actions > Build 3 KNOB for macOS Intel**. It should run automatically after upload; if not, choose **Run workflow**.
4. If the build succeeds, open the completed run and download the artifact named `3-KNOB-by-Leo-Gallagher-macOS-Intel`. Extract its outer download ZIP and then its inner `3-KNOB-macOS-Intel.zip` to obtain **`3 KNOB.component`**.
5. On your Mac, Finder > Go > Go to Folder > `~/Library/Audio/Plug-Ins/Components/` . Drag **`3 KNOB.component`** there, then restart Logic. The plugin should show under **Audio FX > Audio Units > Leo Gallagher > 3 KNOB**.
6. If macOS quarantines the unsigned personal plugin, only if you trust this source and build, you may remove the quarantine attribute from this one installed component using Terminal: `xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/Components/3 KNOB.component"`. Restart Logic and try Plug-in Manager > Reset & Rescan Selection.

**Caveat:** The workflow is provided but has not been executed on GitHub from this environment. Likewise, actual Monterey/Logic validation still must happen on the user's Mac. The cloud result is compiled for x86_64 (Intel), with macOS 10.15 as the minimum target; targeting an older version does not itself guarantee it will load on every older Mac.

## Technical details

- Audio Unit type: `aufx` (effect)
- Manufacturer (four-character AU ID): `Lega` (Leo Gallagher)
- Plugin (four-character AU ID): `Knob`
- Display name: `3 KNOB`
- Company: `Leo Gallagher`
- CMake target: `ThreeKnob_AU`
- Bundle identifier: `com.leogallagher.threeknob`
- macOS deployment target: 10.15; native Mac architecture (x86_64 or arm64)

### Developer manual build

```bash
cmake -S . -B build-clt -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
cmake --build build-clt --config Release --target ThreeKnob_AU --parallel 2
```

The script takes care of locating and installing the compiled bundle. A JUCE checkout at `./JUCE/` is used automatically if supplied; otherwise CMake FetchContent downloads the pinned version.

### Tests performed when authoring

On Linux (not in Logic), the C++ DSP engine was compiled using a standard optimizing compiler and again with **AddressSanitizer + UndefinedBehaviorSanitizer**. Eleven assertions passed in each run, including silent input, exact passthrough at zero settings and in bypass, independent action of the three controls, invalid input containment, varying buffer lengths, mono/stereo operation, automation changes, and 8–384 kHz sample-rate preparation. A processing benchmark was also run.

**Not yet tested:** macOS compilation, JUCE host/UI integration, AU validation, or Logic Pro playback. The build script runs an additional local DSP test and `auval` on your Mac. If macOS compilation or `auval` fails, the output from the script will show which step needs fixing. The source package is a real implementation, not a fabricated AU file or a promise of a pretested Mac binary.

## License / framework

The DSP and plugin source code were written for this project. To compile, JUCE 8.0.15 is fetched from its upstream repository; JUCE is available under AGPLv3 or a commercial license. If distributing a plugin built with JUCE, review and comply with the applicable JUCE license terms. This source ZIP does not redistribute JUCE.
