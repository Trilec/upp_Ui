# Getting Started

The shortest path is to build UiLabelDemo, then read its small application shell
and generated control code. Reading Graph workspace architecture is not a prerequisite
for displaying a label or button.

## 1. Use a declared U++ environment

The maintainer setup uses Windows, U++18468 and CLANGx64. Keep your established
compiler/framework version while validating a checkpoint; do not upgrade silently.
Ui depends on Core, Draw, Painter, CtrlCore, CtrlLib and the external Animation
package. The checked-in assembly also includes the external upp_statemachine nest.
Demos using PropertyEditor find its packages under this repository's Utilities path.

GitHubOut.var contains the maintainer's actual nest and output configuration:

```text
UPP = "E:/apps/github/upp_Ui/examples;E:/apps/github/upp_Ui;E:/apps/github/upp_statemachine;E:/apps/github/upp_animation;E:/upp-18468/uppsrc";
OUTPUT = "E:/apps/github/upp_Ui/build";
```

At those paths use that file unchanged. On another machine create a local assembly
.var with equivalent existing nests and your actual output folder; do not change
source paths or depend on somebody else's E: drive. Keep build output outside source
or in a git-ignored build directory. TheIDE and UMK must resolve the same packages.

## 2. Build a runnable demo, not the library

From the maintainer checkout in PowerShell:

```powershell
Set-Location E:\apps\github\upp_Ui
& 'E:\upp-18468\umk.exe' '.\GitHubOut.var' 'examples/UiLabelDemo' 'CLANGx64' -b +GUI '.\build\UiLabelDemo.exe'
if ($LASTEXITCODE) { throw 'UiLabelDemo build failed' }
& '.\build\UiLabelDemo.exe'
```

UMK accepts an assembly name, a full .var path or an explicit nest list. Use the
full .var path when assembly discovery is ambiguous. Debug is the default; `-r`
selects Release and `-b` BLITZ. `-a` is rebuild-all, not a library-link workaround.
TheIDE users open an assembly with the same nests, select UiLabelDemo and run it.

Ui/Ui.upp and the PropertyEditor packages are libraries with no main entry point.
A missing main when asking for Ui.exe is a wrong build target, not a missing library
feature. Real test/demo packages supply GUI_APP_MAIN or CONSOLE_APP_MAIN.

## 3. Explore one control

Use Inspector for normal public behavior, Theme Overrides for explicitly authored
style, and Code for the corresponding C++. UiLabelDemo is the shell reference;
UiButtonDemo is the next action/state example. UiEditDemo covers the text-edit
family; UiIntFloatDemo covers numeric input. See the Controls Guide for other types.

Generated examples deliberately omit the demo shell. Copy them into an ordinary
U++ application with the stated headers/resources and lifetimes. Where the generator
requires host resource/provider/callback code, supply it explicitly. Compile the
actual output unchanged before treating the example as accepted.

## 4. Run the surgical implementation gate

Update clean main first; do not discard local work:

```powershell
git status --short
git pull --ff-only
if ($LASTEXITCODE) { throw 'Update failed; preserve local work and inspect' }
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ValidateUiRelease.ps1
if ($LASTEXITCODE) { throw 'Read the reported validation evidence directory' }
```

The runner checks a clean main checkout and the freshly fetched origin/main,
reads the supplied assembly, derives UMK from its uppsrc nest unless -Umk is given,
and records tested HEAD, version, toolchain, logs and summaries. It never adds a
library main, commits code, deletes user files or kills an unrelated demo instance.
-AssemblyFile and -Method select an already installed equivalent environment.
-RequiredAncestor verifies a published checkpoint without requiring exact HEAD equality.

Profiles: Surgical (default), Headers (isolated public headers), Demos (build each
retained example), Full (retained test/demo builds and test execution). Select
-Configuration Debug/Release/Both and -Blitz explicitly; Full defaults to Both.
Broader profiles may reveal unfinished release work: failure is not permission to
weaken tests or remove a target. -SelfTest exercises the evidence parser only.

A surgical PASS is not full visual, generated-code, all-controls or cross-platform
acceptance. The release inventory and ACTIVE_WORK keep those boundaries explicit.

## Where to go next

Read the [Controls Guide](docs/01_UI_CONTROLS_GUIDE.md), then the guide for the
subsystem you are changing. Read [Coding](docs/00_UPP_CODING_GUIDE.md) before edits.
Graph users start with its usage guide; graph authors/developers use the separate
development guide and the existing workspace runner. Do not resurrect DesignMatrix
or historical checkpoint tasks as the current graph workflow.
