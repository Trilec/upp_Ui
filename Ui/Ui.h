#ifndef _Ui_Ui_h_
#define _Ui_Ui_h_

/*
    Ui.h
    ====

    Purpose
    - Public umbrella include for the Ui package.

    Intent
    - Provide one stable include point for application code that wants the full
      Ui control family plus shared styling, drawing, theme, and model support.

    Thread context
    - GUI thread for control use; value/model types follow their own semantics.

    Usage
    - Include this header in applications that want the complete Ui package.
    - Include narrower headers directly when compile isolation matters.

    Changelog
    - 2026-03: documented as the public umbrella surface for release cleanup.
*/

#include <CtrlCore/CtrlCore.h>   // Core widgets + TopWindow
#include <CtrlLib/CtrlLib.h>     // Chameleon, SColor*, DrawFocus, etc.

#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiIndicatorBase.h>
#include <Ui/UiTheme.h>
#include <Ui/UiDataModels.h>
#include <Ui/UiBoxLayout.h>
#include <Ui/UiGridLayout.h>
#include <Ui/UiIcons.h>
#include <Ui/UiLabel.h>
#include <Ui/UiCheckBox.h>
#include <Ui/UiRadioButton.h>
#include <Ui/UiToggle.h>
#include <Ui/UiPanel.h>
#include <Ui/UiAccordion.h>
#include <Ui/UiScrollPanel.h>
#include <Ui/UiTab.h>
#include <Ui/UiTitleCard.h>
#include <Ui/UiDropdown.h>
#include <Ui/UiMenu.h>
#include <Ui/UiButton.h>
#include <Ui/UiToolButton.h>
#include <Ui/UiBaseEdit.h>
#include <Ui/UiLineEdit.h>
#include <Ui/UiIntEdit.h>
#include <Ui/UiFloatEdit.h>
#include <Ui/UiPasswordEdit.h>
#include <Ui/UiMultiEdit.h>
#include <Ui/UiMaskEdit.h>
#include <Ui/UiSlider.h>
#include <Ui/UiSliderEdit.h>
#include <Ui/UiScrollBar.h>
#include <Ui/UiTable.h>
#include <Ui/UiDoc.h>
#include <Ui/UiTree.h>
#include <Ui/UiList.h>

#endif


