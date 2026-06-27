#ifndef _Utilities_DesignerTrace_h_
#define _Utilities_DesignerTrace_h_

#include "DesignerModel.h"

namespace Upp {

int DesignerTraceSeq();

enum DesignerTraceMode {
	TRACE_OFF,
	TRACE_TRANSACTION,
	TRACE_SELECTION,
	TRACE_DUMP,
	TRACE_LOAD
};

struct DesignerTraceContext {
	bool active = false;
	int tx_id = 0;
	int lines = 0;
	int max_lines = 120;
	DesignerTraceMode mode = TRACE_OFF;
	DesignerNodeId node_id = Designer_NULL;
	DesignerNodeId related_node_id = Designer_NULL;
	String property_id;
	String reason;
	int preview_rebuild_lines = 0;
	int preview_rect_lines = 0;
	int repeated_preview_count = 0;
	int repeated_preview_rect_count = 0;
	String last_preview_rect_line;
	String last_preview_trace_line;
	String last_preview_trace_tag;
	String block_text;
	String summary_text;
	bool capped = false;
	int begin_msecs = 0;
};

void DesignerBeginTrace(DesignerTraceMode mode, DesignerNodeId node_id = Designer_NULL,
                        DesignerNodeId related_node_id = Designer_NULL, const String& property_id = String(),
                        const String& reason = String());
void DesignerEndTrace(const String& result = String(), const String& reason = String());
bool DesignerTraceActive();
DesignerTraceMode DesignerGetTraceMode();
DesignerNodeId DesignerTraceNodeId();
DesignerNodeId DesignerTraceRelatedNodeId();
String DesignerTraceCurrentState();
bool DesignerTraceRefreshPosted();
bool DesignerTraceFullRefreshRequested();
bool DesignerTraceInspectorLiveEditing();
DesignerNodeId DesignerTraceSelection();
bool DesignerTraceWantsPreviewReadback(DesignerNodeId node_id, DesignerNodeId parent_id = Designer_NULL);
void DesignerTraceNotifyIdlePreviewRebuild();
void DesignerTraceRecordRefreshSource(const String& kind, const String& caller, const String& reason = String());
void DesignerTraceRecordPreviewLayoutSource(const String& caller, const String& reason = String());
void DesignerTraceEmitRefreshLoopSummary();
void DesignerConsoleTrace(const String& tag, const String& msg, bool force = false);
void DesignerTraceSetCurrentState(const String& logical_state, const String& fsm_state);
void DesignerTraceSetRefreshPosted(bool posted);
void DesignerTraceSetFullRefreshRequested(bool requested);
void DesignerTraceSetInspectorLiveEditing(bool active);
void DesignerTraceSetSelection(DesignerNodeId id);
void DesignerSetDiagnosticsEnabled(bool enabled);
bool DesignerDiagnosticsEnabled();
void DesignerSetConsoleTraceEnabled(bool enabled);
bool DesignerConsoleTraceEnabled();
void DesignerSetPreviewReadbackTraceEnabled(bool enabled);
bool DesignerPreviewReadbackTraceEnabled();
void DesignerSetRefreshLoopSummaryEnabled(bool enabled);
bool DesignerRefreshLoopSummaryEnabled();

}

#endif
