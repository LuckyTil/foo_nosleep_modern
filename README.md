# foo_nosleep_modern

[foo_nosleep_modern](https://github.com/LuckyTIL/foo_nosleep_modern/releases) is a [foobar2000](https://www.foobar2000.org/) component that disables automatic system sleep while foobar2000 playback is running.

## Requirements

* Tested on Microsoft Windows 7 and later.
* [foobar2000](https://www.foobar2000.org/download) v2.0 or later (32 or 64-bit). ![foobar2000](https://www.foobar2000.org/button-small.png)

## Getting started

### Installation

* Double-click `foo_nosleep_modern.fb2k-component`.

or

* Import `foo_nosleep_modern.fb2k-component` into foobar2000 using the "*File / Preferences / Components / Install...*" menu item.

## Links

* Home page: [https://github.com/LuckyTIL/foo_nosleep_modern](https://github.com/LuckyTIL/foo_nosleep_modern)
* Repository: [https://github.com/LuckyTIL/foo_nosleep_modern.git](https://github.com/LuckyTIL/foo_nosleep_modern.git)
* Issue tracker: [https://github.com/LuckyTIL/foo_nosleep_modern/issues](https://github.com/LuckyTIL/foo_nosleep_modern/issues)

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)

## Developing

To build the code you need:

* [Microsoft Visual Studio 2026 Community Edition](https://visualstudio.microsoft.com/downloads/) or later
* [foobar2000 SDK](https://www.foobar2000.org/SDK) 2025-03-07
* [Windows Template Library (WTL)](https://github.com/Win32-WTL/WTL) 10.0.10320
* Visual Studio 2022 Toolset (v143)

To create the deployment package you need:

* [PowerShell 7.2](https://github.com/PowerShell/PowerShell) or later

### Setup

Create the following directory structure:

    3rdParty
        WTL10_10320
    foo_nosleep_modern
    int
    out
    sdk

* `3rdParty/WTL10_10320` contains WTL 10.0.10320.
* `foo_nosleep_modern` contains the [Git](https://github.com/LuckyTIL/foo_nosleep_modern) repository.
* `int` contains intermediate stuff.
* `out` receives a deployable version of the component.
* `sdk` contains the foobar2000 SDK.

### Building

Open `foo_nosleep_modern.slnx` with Visual Studio and build the solution.

### Packaging

To create the component first build the x86 configuration and next the x64 configuration.

## Change Log

v1.1.2, 2026-03-17

* Add preferences page.
* Add "Prevent the display from turning off while playback is running" parameter.

v1.0.6, 2026-03-13

* Remove dependencies.

v1.0.5, 2026-03-13

* Use PowerRequestSystemRequired and PowerRequestExecutionRequired.

v1.0.3, 2026-03-12

* Initial release.
