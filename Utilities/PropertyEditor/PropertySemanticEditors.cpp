#include "PropertySemanticEditorsInternal.h"

namespace Upp {

const char *PropertyEditorDateTimeId()   { return "property.datetime"; }
const char *PropertyEditorDurationId()   { return "property.duration"; }
const char *PropertyEditorGeometryId()   { return "property.geometry"; }
const char *PropertyEditorFlagsId()      { return "property.flags"; }
const char *PropertyEditorStringListId() { return "property.string-list"; }
const char *PropertyEditorGradientId()   { return "property.gradient"; }
const char *PropertyEditorKeyChordId()   { return "property.key-chord"; }
const char *PropertyEditorReferenceId()  { return "property.reference"; }
const char *PropertyEditorOptionalId()   { return "property.optional"; }

void RegisterPropertyEditorSemanticEditors(PropertyEditorFactory& factory)
{
    RegisterPropertyEditorSemanticScalarEditors(factory);
    RegisterPropertyEditorSemanticCollectionEditors(factory);
    RegisterPropertyEditorSemanticGradientEditors(factory);
}

void RegisterPropertyEditorEditors(PropertyEditorFactory& factory)
{
    RegisterPropertyEditorV1Editors(factory);
    RegisterPropertyEditorSemanticEditors(factory);
}

} // namespace Upp
