#include <Ui/UiDataModels.h>

namespace Upp {

bool UiListModel::Touch(int first, int count)
{
    if(first < 0 || count <= 0 || first >= items_.GetCount() || count > items_.GetCount() - first)
        return false;
    Notify(UI_MODEL_UPDATE, first, count);
    return true;
}

} // namespace Upp
