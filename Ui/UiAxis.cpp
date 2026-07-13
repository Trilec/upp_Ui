#include <Ui/UiAxis.h>

namespace Upp {

UiResolvedAxis UiResolveAxis(const UiAxisConstraints& constraints)
{
	UiResolvedAxis out;
	out.minimum = max(0, constraints.minimum);
	out.maximum = max(0, constraints.maximum);
	if(out.maximum > 0 && out.maximum < out.minimum)
		out.maximum = out.minimum;
	out.preferred = max(out.minimum, constraints.preferred);
	if(out.maximum > 0)
		out.preferred = min(out.preferred, out.maximum);

	int value = constraints.mode == UiAxisMode::Fixed ? constraints.fixed
	          : constraints.mode == UiAxisMode::Expand ? constraints.available
	                                                  : constraints.preferred;
	value = max(out.minimum, value);
	if(out.maximum > 0)
		value = min(value, out.maximum);
	out.final_size = max(0, value);
	return out;
}

}
