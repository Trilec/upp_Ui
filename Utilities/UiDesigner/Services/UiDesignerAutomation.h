#ifndef _Utilities_UiDesigner_Services_UiDesignerAutomation_h_
#define _Utilities_UiDesigner_Services_UiDesignerAutomation_h_

#include "UiDesignerSession.h"
#include "UiDesignerExport.h"

namespace Upp {

class UiDesignerAutomationService {
public:
    explicit UiDesignerAutomationService(UiDesignerSession& session)
        : session_(session) {}

    Value Handle(const ValueMap& request);

    Value ListControls(const ValueMap& params = ValueMap()) const;
    Value GetControlSpec(const ValueMap& params) const;
    Value ListMcpTools() const;
    Value GetDocument() const;
    Value GetSelection() const;
    Value SetSelection(const ValueMap& params);
    Value GetProperties() const;
    Value GetBehaviors(const ValueMap& params = ValueMap()) const;
    Value SetBehavior(const ValueMap& params);
    Value RemoveBehavior(const ValueMap& params);
    Value PreviewProperty(const ValueMap& params);
    Value CommitProperty(const ValueMap& params);
    Value CancelPreview();
    Value ValidateDocument() const;
    Value GetTheme() const;
    Value PreviewThemeProperty(const ValueMap& params);
    Value CommitThemeProperty(const ValueMap& params);
    Value CancelThemePreview();
    Value ThemeUndo();
    Value ThemeRedo();
    Value NewDocument(const ValueMap& params);
    Value PlanAdd(const ValueMap& params) const;
    Value PlanMove(const ValueMap& params) const;
    Value ApplyDrop(const ValueMap& params);
    Value AddNode(const ValueMap& params);
    Value RemoveNode(const ValueMap& params);
    Value MoveNode(const ValueMap& params);
    Value SetVirtualSize(const ValueMap& params);
    Value Undo();
    Value Redo();
    Value GenerateCode(const ValueMap& params) const;
    Value Export(const ValueMap& params);
    Value Save(const ValueMap& params);
    Value Load(const ValueMap& params);

private:
    Value Ok(const Value& result = Value()) const;
    Value Error(const String& message) const;
    bool CheckRevision(const ValueMap& params, String& error) const;
    Value DropPlanValue(const UiDesignerDropPlan& plan) const;
    UiDesignerExportRequest ExportRequest(const ValueMap& params,
                                           String& error) const;

    UiDesignerSession& session_;
};

class UiDesignerMcpEndpoint {
public:
    explicit UiDesignerMcpEndpoint(UiDesignerAutomationService& service)
        : service_(service) {}

    String HandleJsonLine(const String& line);

private:
    UiDesignerAutomationService& service_;
};

}

#endif
