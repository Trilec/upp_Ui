#ifndef _Utilities_Designer_DesignerAssets_h_
#define _Utilities_Designer_DesignerAssets_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerAssets
    ==============

    Purpose
    - Public header for the DesignerAssets component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.
    - Keep application branding close to the utility without promoting it into
      the shared Ui icon catalog used by controls and demos.

    V1 icon note
    - The public icon pipeline stays source-neutral.
    - Designer stores stable icon ids/names and codegen resolves through the
      shared icon catalog/cache path.
    - Keep controls agnostic to whether the backing source is .iml, SVG, or a
      future vector backend.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-05: added local Designer logo image class for the window/header icon.
    - 2026-06: normalized the top-level header documentation.
*/


#include <Draw/Draw.h>

namespace Upp {

#define IMAGECLASS DesignerAssetsImg
#define IMAGEFILE <Utilities/Designer/Designer.iml>
#include <Draw/iml_header.h>

}

#endif
