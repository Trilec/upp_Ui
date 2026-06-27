#ifndef _Utilities_DesignerTransaction_h_
#define _Utilities_DesignerTransaction_h_

#include "DesignerInspector.h"

namespace Upp {

struct DesignerProjectionRequest {
	bool preview = true;
	bool hierarchy = false;
	bool inspector = false;
	bool code = true;
	bool full = false;
	String reason;
};

struct PendingInspectorTransaction {
	bool active = false;
	bool preview = false;
	bool commit = false;
	bool final_commit = true;
	String node_type;
	String fsm_state_before;
	bool fsm_transition_accepted = false;
	String fsm_reject_reason;
	DesignerNodeId node_id = Designer_NULL;
	String property_id;
	Value value;
	String editor_kind;
	int row_generation = 0;
	int inspector_generation = 0;
	bool inspector_syncing = false;
	Value normalized;
	Value old_model_value;
	Value preview_old_value;
	bool had_old = false;
	bool has_preview_old = false;
	bool command_result = false;
	bool commit_succeeded = false;
	bool inspector_refresh_requested = false;
	bool validation_binding_found = false;
	bool validation_binding_visible = false;
	bool validation_binding_enabled = false;
	Value command_model_after;
	bool command_model_equals_intended = false;
	Value readback_model_value;
	Value readback_inspector_value;
	bool readback_equals_intended = false;
	int state_enter_msecs = 0;
	int state_enter_seq = 0;
	DesignerNodeId state_enter_node = Designer_NULL;
	String state_enter_property;
	String state_enter_state;
	bool state_enter_commit_succeeded = false;
	bool state_enter_warning_emitted = false;
	String failure_reason;
	DesignerProjectionRequest projection;
};

struct InspectorCommitReadback {
	bool active = false;
	DesignerNodeId node_id = Designer_NULL;
	String property_id;
	Value intended_value;
};

struct DeferredInspectorIntent {
	bool active = false;
	DesignerInspectorEditIntent intent;
};

}

#endif
