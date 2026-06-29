#include "SymbolPickerGeneratedCatalog.h"

#include "OLD_CODE/icons_action.h"
#include "OLD_CODE/icons_activitie.h"
#include "OLD_CODE/icons_alert.h"
#include "OLD_CODE/icons_android.h"
#include "OLD_CODE/icons_audio_video.h"
#include "OLD_CODE/icons_busines.h"
#include "OLD_CODE/icons_communicate.h"
#include "OLD_CODE/icons_communication.h"
#include "OLD_CODE/icons_content.h"
#include "OLD_CODE/icons_device.h"
#include "OLD_CODE/icons_editor.h"
#include "OLD_CODE/icons_file.h"
#include "OLD_CODE/icons_hardware.h"
#include "OLD_CODE/icons_home.h"
#include "OLD_CODE/icons_household.h"
#include "OLD_CODE/icons_image.h"
#include "OLD_CODE/icons_map.h"
#include "OLD_CODE/icons_navigation.h"
#include "OLD_CODE/icons_notification.h"
#include "OLD_CODE/icons_place.h"
#include "OLD_CODE/icons_privacy.h"
#include "OLD_CODE/icons_search.h"
#include "OLD_CODE/icons_social.h"
#include "OLD_CODE/icons_text.h"
#include "OLD_CODE/icons_toggle.h"
#include "OLD_CODE/icons_transit.h"
#include "OLD_CODE/icons_travel.h"
#include "OLD_CODE/icons_ui_action.h"

namespace Upp {

static SymbolPickerIconStyle ToPickerStyleGenerated(int style)
{
	switch(style) {
	case 1: return SymbolPickerIconStyle::Rounded;
	case 2: return SymbolPickerIconStyle::Sharp;
	default: return SymbolPickerIconStyle::Outlined;
	}
}

static String MakeDisplayNameGenerated(const char* raw_name)
{
	String out;
	bool new_word = true;
	for(const char* s = raw_name; *s; ++s) {
		if(*s == '_' || *s == '-' || *s == '/') {
			out.Cat(' ');
			new_word = true;
			continue;
		}
		int c = (byte)*s;
		out.Cat(new_word ? ToUpper((wchar)c) : c);
		new_word = false;
	}
	return out;
}

template <class T>
static int AddGeneratedRows(SymbolPickerCatalog& catalog, const T* rows, int count)
{
	int added = 0;
	for(int i = 0; i < count; ++i) {
		const T& row = rows[i];
		SymbolPickerIconEntry e;
		e.category = row.category;
		e.display_name = MakeDisplayNameGenerated(row.name);
		e.source_id = String(row.category) + "/" + row.name;
		e.style = ToPickerStyleGenerated((int)row.style);
		switch(e.style) {
		case SymbolPickerIconStyle::Rounded:
			e.catalog_id = e.source_id + "/rounded";
			break;
		case SymbolPickerIconStyle::Sharp:
			e.catalog_id = e.source_id + "/sharp";
			break;
		default:
			e.catalog_id = e.source_id + "/outlined";
			break;
		}
		e.source_symbol = row.source ? String(row.source) : String();
		e.available = row.b64zIcon && *row.b64zIcon;
		catalog.Add(e);
		++added;
	}
	return added;
}

int LoadGeneratedSymbolPickerCatalog(SymbolPickerCatalog& catalog)
{
	catalog.Clear();

	int added = 0;
	added += AddGeneratedRows(catalog, action::kIcons, action::kIconCount);
	added += AddGeneratedRows(catalog, activitie::kIcons, activitie::kIconCount);
	added += AddGeneratedRows(catalog, alert::kIcons, alert::kIconCount);
	added += AddGeneratedRows(catalog, android::kIcons, android::kIconCount);
	added += AddGeneratedRows(catalog, audio_video::kIcons, audio_video::kIconCount);
	added += AddGeneratedRows(catalog, busines::kIcons, busines::kIconCount);
	added += AddGeneratedRows(catalog, communicate::kIcons, communicate::kIconCount);
	added += AddGeneratedRows(catalog, communication::kIcons, communication::kIconCount);
	added += AddGeneratedRows(catalog, content::kIcons, content::kIconCount);
	added += AddGeneratedRows(catalog, device::kIcons, device::kIconCount);
	added += AddGeneratedRows(catalog, editor::kIcons, editor::kIconCount);
	added += AddGeneratedRows(catalog, file::kIcons, file::kIconCount);
	added += AddGeneratedRows(catalog, hardware::kIcons, hardware::kIconCount);
	added += AddGeneratedRows(catalog, home::kIcons, home::kIconCount);
	added += AddGeneratedRows(catalog, household::kIcons, household::kIconCount);
	added += AddGeneratedRows(catalog, image::kIcons, image::kIconCount);
	added += AddGeneratedRows(catalog, map::kIcons, map::kIconCount);
	added += AddGeneratedRows(catalog, navigation::kIcons, navigation::kIconCount);
	added += AddGeneratedRows(catalog, notification::kIcons, notification::kIconCount);
	added += AddGeneratedRows(catalog, place::kIcons, place::kIconCount);
	added += AddGeneratedRows(catalog, privacy::kIcons, privacy::kIconCount);
	added += AddGeneratedRows(catalog, search::kIcons, search::kIconCount);
	added += AddGeneratedRows(catalog, social::kIcons, social::kIconCount);
	added += AddGeneratedRows(catalog, text::kIcons, text::kIconCount);
	added += AddGeneratedRows(catalog, toggle::kIcons, toggle::kIconCount);
	added += AddGeneratedRows(catalog, transit::kIcons, transit::kIconCount);
	added += AddGeneratedRows(catalog, travel::kIcons, travel::kIconCount);
	added += AddGeneratedRows(catalog, ui_action::kIcons, ui_action::kIconCount);

	return added;
}

}
