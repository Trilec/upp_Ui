#ifndef _Utilities_PropertyEditor_PropertySemanticEditorsInternal_h_
#define _Utilities_PropertyEditor_PropertySemanticEditorsInternal_h_

#include "PropertyValueEditors.h"

namespace Upp {

void RegisterPropertyEditorSemanticScalarEditors(PropertyEditorFactory& factory);
void RegisterPropertyEditorSemanticCollectionEditors(PropertyEditorFactory& factory);
void RegisterPropertyEditorSemanticGradientEditors(PropertyEditorFactory& factory);

}

#endif
