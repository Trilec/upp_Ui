#include "DesignerTrace.h"

namespace Upp {

static DesignerTraceContext designer_trace_;
static int designer_idle_preview_rebuild_count = 0;
static int designer_idle_preview_last_warn_msecs = 0;
static String designer_idle_preview_last_warn_message;
static VectorMap<String, int> designer_invalidate_source_counts;
static VectorMap<String, int> designer_preview_refresh_source_counts;
static VectorMap<String, int> designer_preview_layout_source_counts;
static String designer_trace_current_state = "Idle";
static String designer_trace_current_fsm_state = "Idle";
static bool designer_trace_refresh_posted = false;
static bool designer_trace_full_refresh_requested = false;
static bool designer_trace_inspector_live_editing = false;
static DesignerNodeId designer_trace_selection = Designer_NULL;
static bool designer_diagnostics_enabled = false;
static bool designer_console_trace_enabled = false;
static bool designer_preview_readback_trace_enabled = false;
static bool designer_refresh_loop_summary_enabled = false;

static void DesignerTraceAppendBlockLine(const String& line)
{
	if(!designer_trace_.block_text.IsEmpty())
		designer_trace_.block_text << "\n";
	designer_trace_.block_text << line;
}

int DesignerTraceSeq()
{
	static int seq = 0;
	return ++seq;
}

void DesignerBeginTrace(DesignerTraceMode mode, DesignerNodeId node_id, DesignerNodeId related_node_id,
                        const String& property_id, const String& reason)
{
	designer_trace_.active = true;
	designer_trace_.tx_id = DesignerTraceSeq();
	designer_trace_.mode = mode;
	designer_trace_.node_id = node_id;
	designer_trace_.related_node_id = related_node_id;
	designer_trace_.property_id = property_id;
	designer_trace_.reason = reason;
	designer_trace_.lines = 0;
	designer_trace_.preview_rebuild_lines = 0;
	designer_trace_.preview_rect_lines = 0;
	designer_trace_.repeated_preview_count = 0;
	designer_trace_.repeated_preview_rect_count = 0;
	designer_trace_.last_preview_rect_line.Clear();
	designer_trace_.last_preview_trace_line.Clear();
	designer_trace_.last_preview_trace_tag.Clear();
	designer_trace_.block_text.Clear();
	designer_trace_.summary_text.Clear();
	designer_trace_.capped = false;
	designer_trace_.begin_msecs = msecs();
	DesignerConsoleTrace("TRACE_BEGIN",
		Format("=== DESIGNER TRACE BEGIN tx=%03d reason=%s node=%d property=%s ===",
		       designer_trace_.tx_id, reason, (int)node_id, property_id),
		true);
}

void DesignerEndTrace(const String& result, const String& reason)
{
	if(!designer_trace_.active)
		return;
	String final_trace_state = designer_trace_current_state;
	String final_fsm_state = designer_trace_current_fsm_state;
	String final_result = result.IsEmpty() ? String(final_fsm_state == "Idle" ? "OK" : "FAILED") : result;
	String final_reason = reason;
	if(final_reason.IsEmpty() && final_result == "FAILED" && final_fsm_state != "Idle")
		final_reason = "fsm did not actually return to Idle";
	DesignerConsoleTrace("TRACE_END",
		Format("=== DESIGNER TRACE END tx=%03d result=%s final_trace_state=%s final_fsm_state=%s lines=%d%s%s ===",
		       designer_trace_.tx_id, final_result, final_trace_state, final_fsm_state, designer_trace_.lines,
		       final_reason.IsEmpty() ? "" : " reason=",
		       final_reason.IsEmpty() ? "" : final_reason),
		true);
	designer_trace_ = DesignerTraceContext();
}

bool DesignerTraceActive() { return designer_trace_.active; }
DesignerTraceMode DesignerGetTraceMode() { return designer_trace_.mode; }
DesignerNodeId DesignerTraceNodeId() { return designer_trace_.node_id; }
DesignerNodeId DesignerTraceRelatedNodeId() { return designer_trace_.related_node_id; }
String DesignerTraceCurrentState() { return designer_trace_current_state; }
bool DesignerTraceRefreshPosted() { return designer_trace_refresh_posted; }
bool DesignerTraceFullRefreshRequested() { return designer_trace_full_refresh_requested; }
bool DesignerTraceInspectorLiveEditing() { return designer_trace_inspector_live_editing; }
DesignerNodeId DesignerTraceSelection() { return designer_trace_selection; }

bool DesignerTraceWantsPreviewReadback(DesignerNodeId node_id, DesignerNodeId parent_id)
{
	if(!designer_preview_readback_trace_enabled)
		return false;
	if(!designer_trace_.active || designer_trace_.mode != TRACE_TRANSACTION)
		return false;
	return node_id == designer_trace_.node_id ||
	       node_id == designer_trace_.related_node_id ||
	       parent_id == designer_trace_.node_id ||
	       parent_id == designer_trace_.related_node_id;
}

void DesignerTraceNotifyIdlePreviewRebuild()
{
	if(!designer_refresh_loop_summary_enabled)
		return;
	designer_idle_preview_rebuild_count++;
	int now = msecs();
	if(designer_idle_preview_rebuild_count < 50)
		return;
	if(designer_idle_preview_last_warn_msecs && msecs(designer_idle_preview_last_warn_msecs) < 2000)
		return;
	designer_idle_preview_last_warn_msecs = now;
	String msg = Format("rebuilds=%d in 2000ms selected=%d current_state=%s refresh_posted=%d full_refresh=%d inspector_live_editing=%d",
	                    designer_idle_preview_rebuild_count, (int)designer_trace_selection,
	                    designer_trace_current_state, designer_trace_refresh_posted ? 1 : 0,
	                    designer_trace_full_refresh_requested ? 1 : 0,
	                    designer_trace_inspector_live_editing ? 1 : 0);
	if(msg != designer_idle_preview_last_warn_message) {
		designer_idle_preview_last_warn_message = msg;
		DesignerConsoleTrace("IDLE_PREVIEW_LOOP", msg, true);
	}
	designer_idle_preview_rebuild_count = 0;
}

static void DesignerTraceBumpCount(VectorMap<String, int>& counts, const String& key)
{
	int q = counts.Find(key);
	if(q < 0) counts.Add(key, 1);
	else counts[q] = counts[q] + 1;
}

void DesignerTraceRecordRefreshSource(const String& kind, const String& caller, const String& reason)
{
	if(!designer_refresh_loop_summary_enabled)
		return;
	String key = caller.IsEmpty() ? String("unknown") : caller;
	if(!reason.IsEmpty()) key << " [" << reason << "]";
	if(kind == "PreviewRefresh") DesignerTraceBumpCount(designer_preview_refresh_source_counts, key);
	else DesignerTraceBumpCount(designer_invalidate_source_counts, key);
}

void DesignerTraceRecordPreviewLayoutSource(const String& caller, const String& reason)
{
	if(!designer_refresh_loop_summary_enabled)
		return;
	String key = caller.IsEmpty() ? String("paint/layout") : caller;
	if(!reason.IsEmpty()) key << " [" << reason << "]";
	DesignerTraceBumpCount(designer_preview_layout_source_counts, key);
}

void DesignerTraceEmitRefreshLoopSummary()
{
	if(!designer_refresh_loop_summary_enabled)
		return;
	if(designer_invalidate_source_counts.IsEmpty() &&
	   designer_preview_refresh_source_counts.IsEmpty() &&
	   designer_preview_layout_source_counts.IsEmpty())
		return;
	String out;
	out << "REFRESH_LOOP_SUMMARY 2000ms\n";
	out << "InvalidateRealPreview:\n";
	if(designer_invalidate_source_counts.IsEmpty())
		out << "- none: 0\n";
	for(int i = 0; i < designer_invalidate_source_counts.GetCount(); i++)
		out << "- " << designer_invalidate_source_counts.GetKey(i) << ": " << designer_invalidate_source_counts[i] << "\n";
	out << "PreviewRefresh:\n";
	if(designer_preview_refresh_source_counts.IsEmpty())
		out << "- none: 0\n";
	for(int i = 0; i < designer_preview_refresh_source_counts.GetCount(); i++)
		out << "- " << designer_preview_refresh_source_counts.GetKey(i) << ": " << designer_preview_refresh_source_counts[i] << "\n";
	out << "PreviewLayout:\n";
	if(designer_preview_layout_source_counts.IsEmpty())
		out << "- none: 0\n";
	for(int i = 0; i < designer_preview_layout_source_counts.GetCount(); i++)
		out << "- " << designer_preview_layout_source_counts.GetKey(i) << ": " << designer_preview_layout_source_counts[i] << "\n";
	DesignerConsoleTrace("REFRESH_LOOP_SUMMARY", out, true);
	designer_invalidate_source_counts.Clear();
	designer_preview_refresh_source_counts.Clear();
	designer_preview_layout_source_counts.Clear();
}

void DesignerConsoleTrace(const String& tag, const String& msg, bool force)
{
	if(!force && !designer_console_trace_enabled)
		return;
	if(!force && !designer_trace_.active)
		return;
	String line = Format("#%05d %-14s %s", DesignerTraceSeq(), tag, msg);
	Cout() << line << "\n";
	RLOG(line);
	if(designer_trace_.active)
		designer_trace_.lines++;
}

void DesignerTraceSetCurrentState(const String& logical_state, const String& fsm_state)
{
	designer_trace_current_state = logical_state;
	designer_trace_current_fsm_state = fsm_state;
}

void DesignerTraceSetRefreshPosted(bool posted) { designer_trace_refresh_posted = posted; }
void DesignerTraceSetFullRefreshRequested(bool requested) { designer_trace_full_refresh_requested = requested; }
void DesignerTraceSetInspectorLiveEditing(bool active) { designer_trace_inspector_live_editing = active; }
void DesignerTraceSetSelection(DesignerNodeId id) { designer_trace_selection = id; }
void DesignerSetDiagnosticsEnabled(bool enabled) { designer_diagnostics_enabled = enabled; }
bool DesignerDiagnosticsEnabled() { return designer_diagnostics_enabled; }
void DesignerSetConsoleTraceEnabled(bool enabled) { designer_console_trace_enabled = enabled; }
bool DesignerConsoleTraceEnabled() { return designer_console_trace_enabled; }
void DesignerSetPreviewReadbackTraceEnabled(bool enabled) { designer_preview_readback_trace_enabled = enabled; }
bool DesignerPreviewReadbackTraceEnabled() { return designer_preview_readback_trace_enabled; }
void DesignerSetRefreshLoopSummaryEnabled(bool enabled) { designer_refresh_loop_summary_enabled = enabled; }
bool DesignerRefreshLoopSummaryEnabled() { return designer_refresh_loop_summary_enabled; }

}
