#include "UiDesignerOverlay.h"

namespace Upp {

void UiDesignerTransientOverlay::Set(UiDesignerNodeId node,
                                     UiDesignerTransientValueKind kind,
                                     const String& property,
                                     const Value& value)
{
    for(UiDesignerTransientOverride& item : values_)
        if(item.node == node && item.kind == kind && item.property == property) {
            item.value = value;
            return;
        }
    UiDesignerTransientOverride& item = values_.Add();
    item.node = node;
    item.kind = kind;
    item.property = property;
    item.value = value;
}

void UiDesignerTransientOverlay::Remove(UiDesignerNodeId node,
                                        UiDesignerTransientValueKind kind,
                                        const String& property)
{
    for(int i = values_.GetCount() - 1; i >= 0; i--)
        if(values_[i].node == node && values_[i].kind == kind &&
           values_[i].property == property)
            values_.Remove(i);
}

void UiDesignerTransientOverlay::Clear()
{
    values_.Clear();
}

bool UiDesignerTransientOverlay::Has(UiDesignerNodeId node,
                                     UiDesignerTransientValueKind kind,
                                     const String& property) const
{
    for(const UiDesignerTransientOverride& item : values_)
        if(item.node == node && item.kind == kind && item.property == property)
            return true;
    return false;
}

Value UiDesignerTransientOverlay::Resolve(UiDesignerNodeId node,
                                          UiDesignerTransientValueKind kind,
                                          const String& property,
                                          const Value& canonical) const
{
    for(int i = values_.GetCount() - 1; i >= 0; i--)
        if(values_[i].node == node && values_[i].kind == kind &&
           values_[i].property == property)
            return values_[i].value;
    return canonical;
}

}
