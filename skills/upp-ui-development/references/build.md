# Build discovery and integration

Find the application's .upp, the installed U++ source/toolchain, assembly .var,
build method, Ui checkout and external nests. Use existing build scripts first.
The assembly's source nests resolve packages; its output folder does not.
Do not install or upgrade a compiler to bypass an unexplained local failure.

At the review date this workstation uses E:/upp-18468/umk.exe, CLANGx64 and
E:/apps/github/upp_Ui. These are examples to verify, never universal paths.
The Ui repository's GitHubOut.var records Ui, examples, Animation, statemachine
and uppsrc nests. Designer has its own assembly/package; it is not a Ui dependency.

UMK shape: `umk <assembly-or-var-path> <main-package> <method> <flags> <output>`.
For example, from the Ui checkout:

```powershell
& 'E:/upp-18468/umk.exe' './GitHubOut.var' 'examples/UiLabelDemo' 'CLANGx64' -br +GUI './build/UiLabelDemo.exe'
if ($LASTEXITCODE) { throw 'Build failed' }
```

Debug is the default, -r selects Release, -b enables BLITZ and -a rebuilds all.
Check the installed tool's help before relying on other switches. Preserve both
ordinary and BLITZ compilation: missing direct includes can be masked by BLITZ.
If a shared header changes object layout and an incremental executable crashes,
rebuild affected dependencies cleanly before attributing it to application logic.

A library has no application entrypoint. A runnable GUI package supplies
GUI_APP_MAIN and GUI mainconfig. Declare direct dependencies in uses and keep
file membership complete. Image decoders need their plugin packages. Diagnose
missing packages via nests and missing WinMain via the selected main package.

For .lay resources, verify LAYOUTFILE against the actual include search roots;
do not impose an absolute path or the old guide's contradictory path rules.
Use the real project's resource macros and .iml pipeline for icons.

Generated native apps need their .upp, sources and assets together. Put behavior
outside overwritten generated regions. Wire OK/Cancel actions, not just labels;
set startup theme/mode, size and resize policy explicitly. Keep executables in
the project's bin/output convention, scratch evidence under build, and preserve
unsaved work before replacing a running application.

The full getting-started snapshot is available at
[upstream/GETTING_STARTED.md](upstream/GETTING_STARTED.md). Its release runner
has a clean-checkout workflow; do not discard local work to satisfy that gate.
