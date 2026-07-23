$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

function Fail([string]$message) { throw "UiDesigner architecture guard: $message" }
function Require-Path([string]$relative) {
    $path = Join-Path $root $relative
    if(-not (Test-Path -LiteralPath $path)) { Fail "missing $relative" }
}
function Require-Text([string]$relative, [string]$pattern) {
    $path = Join-Path $root $relative
    if(-not (Select-String -LiteralPath $path -Pattern $pattern -Quiet)) {
        Fail "$relative does not contain required contract: $pattern"
    }
}
function Forbid-Text([string]$relative, [string]$pattern) {
    $path = Join-Path $root $relative
    if(Select-String -LiteralPath $path -Pattern $pattern -Quiet) {
        Fail "$relative contains forbidden dependency/mutation: $pattern"
    }
}
function Forbid-InTree([string]$relative, [string]$pattern) {
    $path = Join-Path $root $relative
    $match = Get-ChildItem -LiteralPath $path -Recurse -File -Include *.h,*.cpp,*.upp |
        Select-String -Pattern $pattern | Select-Object -First 1
    if($match) { Fail "$relative contains '$pattern' at $($match.Path):$($match.LineNumber)" }
}

$requiredPackages = @(
    'Utilities\PropertyEditorCore',
    'Utilities\PropertyEditor',
    'Utilities\UiDesigner\Core',
    'Utilities\UiDesigner\Commands',
    'Utilities\UiDesigner\Catalog',
    'Utilities\UiDesigner\Preview',
    'Utilities\UiDesigner\CodeGen',
    'Utilities\UiDesigner\ThemeCore',
    'Utilities\UiDesigner\Theme',
    'Utilities\UiDesigner\Services',
    'Utilities\UiDesigner\CLI',
    'Utilities\UiDesigner\MCP',
    'Utilities\UiDesigner\UiDesigner',
    'Utilities\UiDesigner\Tests',
    'Utilities\UiDesigner\FoundationTests'
)
foreach($package in $requiredPackages) { Require-Path $package }

$requiredDocs = @(
    'Utilities\UiDesigner\DRAG_DROP_DESIGN.md',
    'Utilities\UiDesigner\BEHAVIOR_BINDING_DESIGN.md',
    'Utilities\UiDesigner\EXPORT_CONTRACT.md',
    'Utilities\UiDesigner\IMPLEMENTATION_STATUS.md'
)
foreach($doc in $requiredDocs) { Require-Path $doc }

$mainPath = Join-Path $root 'Utilities\UiDesigner\UiDesigner\main.cpp'
$mainLines = @(Get-Content -LiteralPath $mainPath).Count
if($mainLines -gt 40) { Fail "application main.cpp is $mainLines lines; shell composition belongs outside main" }

foreach($headless in @(
    'Utilities\UiDesigner\Core',
    'Utilities\UiDesigner\Commands',
    'Utilities\UiDesigner\Catalog',
    'Utilities\UiDesigner\CodeGen',
    'Utilities\UiDesigner\ThemeCore',
    'Utilities\UiDesigner\Services',
    'Utilities\UiDesigner\CLI',
    'Utilities\UiDesigner\MCP'
)) {
    Forbid-InTree $headless '^\s*#include\s+<Ui/'
    Forbid-InTree $headless '^\s*#include\s+<CtrlLib/'
    Forbid-InTree $headless 'Utilities/Designer/'
}

Forbid-InTree 'Utilities\UiDesigner' 'Utilities/Designer/main\.cpp'
Forbid-Text 'Utilities\UiDesigner\UiDesigner\UiDesignerWindow.cpp' 'Document\(\)\.(Set|Add|Remove|Move|Replace)'
Forbid-Text 'Utilities\UiDesigner\UiDesigner\UiDesignerWindow.cpp' 'GetNodes\(\)\.(Add|Remove|Set)'

Require-Text 'Utilities\UiDesigner\Catalog\UiDesignerBuiltins.cpp' 'type_id\s*=\s*"Spacer"'
Require-Text 'Utilities\UiDesigner\Catalog\UiDesignerBuiltins.cpp' 'UiDesignerNodeSemanticItem'
Require-Text 'Utilities\UiDesigner\Theme\UiDesignerThemeAdapter.h' 'class UiDesignerThemeAdapter'
Require-Text 'Utilities\UiDesigner\Theme\UiDesignerThemeAdapter.cpp' 'ButtonThemeAdapter'
Require-Text 'Utilities\UiDesigner\Theme\UiDesignerThemeAdapter.cpp' 'TreeThemeAdapter'
Require-Text 'Utilities\UiDesigner\Theme\UiDesignerThemeAdapter.cpp' 'ListThemeAdapter'
Require-Text 'Utilities\UiDesigner\Theme\UiDesignerThemeAdapter.cpp' 'MenuThemeAdapter'
Require-Text 'Utilities\UiDesigner\Core\UiDesignerTypes.h' 'UiDesignerActionBinding'
Require-Text 'Utilities\UiDesigner\Services\UiDesignerDrop.cpp' 'UiDesignerDropService::PlanAdd'
Require-Text 'Utilities\UiDesigner\Services\UiDesignerDrop.cpp' 'UiDesignerDropService::PlanMove'
Require-Text 'Utilities\UiDesigner\Services\UiDesignerExport.cpp' 'UiDesignerExportProfile::ComponentOnly'
Require-Text 'Utilities\UiDesigner\CodeGen\UiDesignerCodeGen.cpp' 'UiDesignerChildAdapterEntry'
Require-Text 'Utilities\UiDesigner\CodeGen\UiDesignerCodeGen.cpp' 'RestorePublished'
Require-Text 'Utilities\UiDesigner\FoundationTests\main.cpp' 'legacy root Spacer imports'
Require-Text 'Utilities\UiDesigner\FoundationTests\main.cpp' 'package filename is independent from class name'
Require-Text 'Utilities\UiDesigner\CLI\main.cpp' 'behavior-set'
Require-Text 'Utilities\UiDesigner\Services\UiDesignerAutomation.cpp' 'uidesigner_apply_drop'
Require-Text 'Utilities\UiDesigner\Services\UiDesignerAutomation.cpp' 'uidesigner_set_behavior'
Require-Text 'Utilities\UiDesigner\Services\UiDesignerAutomation.cpp' 'uidesigner_export'
Require-Text 'Utilities\UiDesigner\Catalog\UiDesignerCatalog.h' 'theme_adapter_id'
Forbid-Text 'Utilities\UiDesigner\Catalog\UiDesignerCatalog.cpp' 'button_style_field'
Forbid-Text 'Utilities\UiDesigner\Preview\UiDesignerPreview.cpp' 'button_style_field'
Forbid-Text 'Utilities\UiDesigner\CodeGen\UiDesignerCodeGen.cpp' 'button_style_field'
Forbid-Text 'Utilities\UiDesigner\Services\UiDesignerSession.cpp' 'button_style_field'

foreach($obsolete in @(
    'Utilities\UiDesigner\.integrated',
    'Utilities\UiDesigner\.bundle',
    'Utilities\UiDesigner\InstallIntegratedSource.ps1',
    'Utilities\UiDesigner\InstallSource.ps1'
)) {
    if(Test-Path -LiteralPath (Join-Path $root $obsolete)) {
        Fail "obsolete source-delivery transport remains: $obsolete"
    }
}

Write-Host 'UiDesigner architecture guard: PASS'
