#include "PropertySemanticEditorsInternal.h"

namespace Upp {

const char *UiPropertyInspectorDateTimeId()   { return "property.datetime"; }
const char *UiPropertyInspectorDurationId()   { return "property.duration"; }
const char *UiPropertyInspectorGeometryId()   { return "property.geometry"; }
const char *UiPropertyInspectorFlagsId()      { return "property.flags"; }
const char *UiPropertyInspectorStringListId() { return "property.string-list"; }
const char *UiPropertyInspectorGradientId()   { return "property.gradient"; }
const char *UiPropertyInspectorKeyChordId()   { return "property.key-chord"; }
const char *UiPropertyInspectorReferenceId()  { return "property.reference"; }
const char *UiPropertyInspectorOptionalId()   { return "property.optional"; }

void RegisterUiPropertyInspectorSemanticEditors(UiPropertyInspectorFactory& factory)
{
    RegisterUiPropertyInspectorSemanticScalarEditors(factory);
    RegisterUiPropertyInspectorSemanticCollectionEditors(factory);
    RegisterUiPropertyInspectorSemanticGradientEditors(factory);
}

void RegisterUiPropertyInspectorEditors(UiPropertyInspectorFactory& factory)
{
    RegisterUiPropertyInspectorBuiltinEditors(factory);
    RegisterUiPropertyInspectorSemanticEditors(factory);
}

} // namespace Upp
