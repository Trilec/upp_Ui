#ifndef _Utilities_Designer_DesignerAssets_h_
#define _Utilities_Designer_DesignerAssets_h_

/*
    DesignerAssets
    ==============

    Purpose
    - Local image assets for the Designer utility shell.

    Intent
    - Keep application branding close to the utility without promoting it into
      the shared Ui icon catalog used by controls and demos.

    Changelog
    - 2026-05: added local Designer logo image class for the window/header icon.
*/

#include <Draw/Draw.h>

namespace Upp {

#define IMAGECLASS DesignerAssetsImg
#define IMAGEFILE <Utilities/Designer/Designer.iml>
#include <Draw/iml_header.h>

}

#endif
