#ifndef _examples_UiDocDemo_UiDocReviewPanel_h_
#define _examples_UiDocDemo_UiDocReviewPanel_h_

#include <Ui/Ui.h>
#include <Ui/UiIcons.h>

namespace Upp {

class UiDocReviewPanel : public UiGroupPanel {
public:
    typedef UiDocReviewPanel CLASSNAME;

    Event<> WhenClosePanel;
    Event<const String&> WhenStatus;

    UiDocReviewPanel()
    {
        Build();
        Wire();
    }

    void Bind(UiDoc& doc)
    {
        doc_ = &doc;
        RefreshPanel();
    }

    void RefreshPanel()
    {
        if(!doc_)
            return;
        refreshing_ = true;
        RefreshCommentList();
        RefreshMetadataList();
        RefreshCommentEditor();
        RefreshMetadataEditor();
        RefreshSummary();
        refreshing_ = false;
    }

    void ActivateAnnotation(const String& id)
    {
        if(!doc_ || id.IsEmpty())
            return;

        for(const UiDocAnnotation& metadata : doc_->GetMetadata()) {
            if(metadata.id != id)
                continue;
            active_metadata_id_ = id;
            active_comment_id_.Clear();
            tabs_.SetActiveTab(1);
            doc_->RevealAnnotation(id, false);
            RefreshPanel();
            metadata_list_.SetData(id);
            metadata_list_.ScrollToSelection();
            return;
        }

        for(const UiDocAnnotation& comment : doc_->GetComments()) {
            if(comment.id != id)
                continue;
            active_comment_id_ = id;
            active_metadata_id_.Clear();
            tabs_.SetActiveTab(0);
            doc_->RevealAnnotation(id, true);
            RefreshPanel();
            comment_list_.SetData(id);
            comment_list_.ScrollToSelection();
            return;
        }
    }

    void SelectMetadataType(const String& type)
    {
        String normalized = type;
        if(normalized.StartsWith("metadata."))
            normalized = normalized.Mid(9);
        if(normalized.IsEmpty())
            normalized = "note";
        EnsureMetadataTypeOption(normalized);
        metadata_type_.SetDataSilently(normalized);
    }

    void ShowCommentsTab()
    {
        tabs_.SetActiveTab(0);
        RefreshPanel();
    }

    void ShowMetadataTab()
    {
        tabs_.SetActiveTab(1);
        RefreshPanel();
    }

    void AddCommentAtCurrent()
    {
        if(!doc_)
            return;
        String id = doc_->AddComment("Review comment");
        if(id.IsEmpty()) {
            WhenStatus("Unable to add comment");
            return;
        }
        active_comment_id_ = id;
        active_metadata_id_.Clear();
        tabs_.SetActiveTab(0);
        doc_->RevealAnnotation(id, true);
        WhenStatus("Comment added - edit it in Review");
        RefreshPanel();
    }

    void AddMetadataAtCurrent(const String& requested_type)
    {
        if(!doc_)
            return;
        String type = requested_type;
        if(type.StartsWith("metadata."))
            type = type.Mid(9);
        if(type.IsEmpty())
            type = "note";
        SelectMetadataType(type);

        String label = ToUpper(type.Left(1)) + type.Mid(1);
        ValueMap payload;
        payload.Add("source", "UiDocDemo quick add");
        int at = CurrentParagraphStart();
        String id = doc_->AddMetadata(UiDocRange(at, at), type,
                                      label + " reference",
                                      "Non-printing reference metadata anchored to this paragraph.",
                                      payload);
        if(id.IsEmpty()) {
            WhenStatus("Unable to add metadata");
            return;
        }
        active_metadata_id_ = id;
        active_comment_id_.Clear();
        tabs_.SetActiveTab(1);
        doc_->ShowMetadata(true);
        doc_->SetMetadataExpanded(id, true);
        doc_->RevealAnnotation(id, false);
        WhenStatus("Metadata added - edit it in Review");
        RefreshPanel();
    }

    const String& GetActiveCommentId() const { return active_comment_id_; }
    const String& GetActiveMetadataId() const { return active_metadata_id_; }

private:
    UiDoc* doc_ = nullptr;
    bool refreshing_ = false;
    String active_comment_id_;
    String active_metadata_id_;

    UiBoxLayout root_;
    UiBoxLayout top_row_;
    UiLabel summary_;
    UiButton close_;
    UiTab tabs_;

    UiBoxLayout comments_page_;
    UiSplitter comments_splitter_;
    UiBoxLayout comment_editor_box_;
    UiLabel comment_context_;
    UiBaseEdit comment_edit_;
    UiBoxLayout comment_actions_;
    UiButton comment_add_;
    UiButton comment_update_;
    UiButton comment_delete_;
    UiButton comment_resolve_;
    UiList comment_list_;

    UiBoxLayout metadata_page_;
    UiSplitter metadata_splitter_;
    UiBoxLayout metadata_editor_box_;
    UiBoxLayout metadata_type_row_;
    UiLabel metadata_type_label_;
    UiDropdown metadata_type_;
    UiLabel metadata_title_label_;
    UiLineEdit metadata_title_;
    UiLabel metadata_body_label_;
    UiBaseEdit metadata_body_;
    UiLabel metadata_payload_label_;
    UiBaseEdit metadata_payload_;
    UiBoxLayout metadata_actions_;
    UiButton metadata_add_;
    UiButton metadata_update_;
    UiButton metadata_delete_;
    UiButton metadata_expand_;
    UiList metadata_list_;

    static void ConfigureTextButton(UiButton& button, const String& text)
    {
        button.SetText(text).SetContentInset(DPI(2)).SetContentGap(DPI(2));
    }

    static void ConfigureIconButton(UiButton& button, const Image& icon, const String& tip)
    {
        button.SetText("")
              .SetIcon(icon)
              .SetIconSide(UiAlign::CENTER)
              .SetIconSize(DPI(14), DPI(14))
              .SetIconRenderMode(UiIconRenderMode::MonoTint)
              .SetContentInset(DPI(2));
        button.Tip(tip);
    }

    void Build()
    {
        SetTitle("Review")
            .SetSubTitle("Comments & metadata")
            .SetHeaderMode(UiGroupPanel::Inside)
            .SetInset(Rect(DPI(8), DPI(6), DPI(8), DPI(8)))
            .SetHeaderInset(Rect(DPI(9), DPI(5), DPI(9), DPI(5)))
            .SetLine(true);

        root_.SetDirection(UiDirection::V).SetGap(DPI(6)).SetInset(0);
        top_row_.SetDirection(UiDirection::H).SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        summary_.SetText("No review items");
        ConfigureIconButton(close_, ICON_NAVIGATION_CLOSE_SMALL_48(), "Collapse Review panel");
        top_row_.Add(summary_).Expand(1);
        top_row_.Add(close_).Fixed(DPI(28));

        BuildCommentsPage();
        BuildMetadataPage();

        tabs_.SetVisual(UITAB_UNDERLINE)
             .SetExpandTabs(true)
             .SetTabFont(SansSerifZ(DPI(9)));
        tabs_.Add(comments_page_, "Comments", ICON_DESIGN_COMMENT_48());
        tabs_.Add(metadata_page_, "Metadata", ICON_EDITOR_NOTES_48());

        root_.Add(top_row_).Fit();
        root_.Add(tabs_).Expand(1);
        SetContent(root_);
    }

    void BuildCommentsPage()
    {
        comments_page_.SetDirection(UiDirection::V).SetGap(0).SetInset(Rect(DPI(2), DPI(5), DPI(2), DPI(2)));
        comment_editor_box_.SetDirection(UiDirection::V).SetGap(DPI(5)).SetInset(0);
        comment_context_.SetText("Select text to add a comment, or choose an item below.");
        comment_edit_.SetPlaceholder("Write or edit the selected comment...")
                     .SetAcceptsNewlines(true)
                     .SetAcceptsTabs(false);

        comment_actions_.SetDirection(UiDirection::H).SetGap(DPI(3)).SetInset(0);
        ConfigureTextButton(comment_add_, "Add");
        ConfigureTextButton(comment_update_, "Update");
        ConfigureTextButton(comment_delete_, "Delete");
        ConfigureTextButton(comment_resolve_, "Resolve");
        comment_actions_.Add(comment_add_).Fixed(DPI(46));
        comment_actions_.Add(comment_update_).Fixed(DPI(58));
        comment_actions_.Add(comment_delete_).Fixed(DPI(54));
        comment_actions_.Add(comment_resolve_).Fixed(DPI(60));

        comment_editor_box_.Add(comment_context_).Fit();
        comment_editor_box_.Add(comment_edit_).Expand(1);
        comment_editor_box_.Add(comment_actions_).Fit();

        UiList::Style list_style = comment_list_.GetStyle();
        list_style.row_height = DPI(34);
        list_style.show_checks = false;
        list_style.show_drag_handle = false;
        list_style.right_text_as_badge = true;
        comment_list_.SetCustomStyle(list_style);

        comments_splitter_.Vert(comment_editor_box_, comment_list_)
                          .SetSplitPercent(45)
                          .SetMinPixels(0, DPI(110))
                          .SetMinPixels(1, DPI(120));
        comments_page_.Add(comments_splitter_).Expand(1);
    }

    void BuildMetadataPage()
    {
        metadata_page_.SetDirection(UiDirection::V).SetGap(0).SetInset(Rect(DPI(2), DPI(5), DPI(2), DPI(2)));
        metadata_editor_box_.SetDirection(UiDirection::V).SetGap(DPI(4)).SetInset(0);

        metadata_type_row_.SetDirection(UiDirection::H).SetGap(DPI(5)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        metadata_type_label_.SetText("Type");
        metadata_type_.UseInternalModel();
        metadata_type_.SetPlaceholderText("Metadata type").SetPopupMaxItems(6);
        metadata_type_.Add("Note", String("note"));
        metadata_type_.Add("Guidance", String("guidance"));
        metadata_type_.Add("Structure", String("structure"));
        metadata_type_.SetDataSilently(String("note"));
        metadata_type_row_.Add(metadata_type_label_).Fixed(DPI(36));
        metadata_type_row_.Add(metadata_type_).Expand(1);

        metadata_title_label_.SetText("Title");
        metadata_title_.SetPlaceholder("Metadata title");
        metadata_body_label_.SetText("Reference text");
        metadata_body_.SetPlaceholder("Non-printing reference text...")
                      .SetAcceptsNewlines(true)
                      .SetAcceptsTabs(false);
        metadata_payload_label_.SetText("Payload JSON");
        metadata_payload_.SetPlaceholder("{ }")
                         .SetAcceptsNewlines(true)
                         .SetAcceptsTabs(false);

        metadata_actions_.SetDirection(UiDirection::H).SetGap(DPI(3)).SetInset(0);
        ConfigureTextButton(metadata_add_, "Add");
        ConfigureTextButton(metadata_update_, "Update");
        ConfigureTextButton(metadata_delete_, "Delete");
        ConfigureTextButton(metadata_expand_, "Show card");
        metadata_actions_.Add(metadata_add_).Fixed(DPI(46));
        metadata_actions_.Add(metadata_update_).Fixed(DPI(58));
        metadata_actions_.Add(metadata_delete_).Fixed(DPI(54));
        metadata_actions_.Add(metadata_expand_).Fixed(DPI(72));

        metadata_editor_box_.Add(metadata_type_row_).Fit();
        metadata_editor_box_.Add(metadata_title_label_).Fit();
        metadata_editor_box_.Add(metadata_title_).Fixed(DPI(28));
        metadata_editor_box_.Add(metadata_body_label_).Fit();
        metadata_editor_box_.Add(metadata_body_).Expand(1);
        metadata_editor_box_.Add(metadata_payload_label_).Fit();
        metadata_editor_box_.Add(metadata_payload_).Fixed(DPI(72));
        metadata_editor_box_.Add(metadata_actions_).Fit();

        UiList::Style list_style = metadata_list_.GetStyle();
        list_style.row_height = DPI(36);
        list_style.show_checks = false;
        list_style.show_drag_handle = false;
        list_style.right_text_as_badge = true;
        metadata_list_.SetCustomStyle(list_style);

        metadata_splitter_.Vert(metadata_editor_box_, metadata_list_)
                          .SetSplitPercent(62)
                          .SetMinPixels(0, DPI(245))
                          .SetMinPixels(1, DPI(100));
        metadata_page_.Add(metadata_splitter_).Expand(1);
    }

    void Wire()
    {
        close_.WhenAction = [=] { WhenClosePanel(); };
        tabs_.WhenAction = [=] { if(!refreshing_) RefreshPanel(); };

        comment_add_.WhenAction = [=] { AddComment(); };
        comment_update_.WhenAction = [=] { UpdateComment(); };
        comment_delete_.WhenAction = [=] { DeleteComment(); };
        comment_resolve_.WhenAction = [=] { ResolveComment(); };
        comment_list_.WhenSelection = [=] { if(!refreshing_) SelectCommentFromList(); };
        comment_list_.WhenAction = [=] { if(!refreshing_) SelectCommentFromList(); };

        metadata_add_.WhenAction = [=] { AddMetadata(); };
        metadata_update_.WhenAction = [=] { UpdateMetadata(); };
        metadata_delete_.WhenAction = [=] { DeleteMetadata(); };
        metadata_expand_.WhenAction = [=] { ToggleMetadataCard(); };
        metadata_list_.WhenSelection = [=] { if(!refreshing_) SelectMetadataFromList(); };
        metadata_list_.WhenAction = [=] { if(!refreshing_) SelectMetadataFromList(); };
    }

    void RefreshSummary()
    {
        if(!doc_)
            return;
        int comments = doc_->GetComments().GetCount();
        int metadata = doc_->GetMetadata().GetCount();
        summary_.SetText(Format("%d comment%s  |  %d metadata",
                                comments, comments == 1 ? "" : "s", metadata));
    }

    void RefreshCommentList()
    {
        UiListModel& model = comment_list_.Model();
        model.Clear();
        if(!doc_)
            return;
        for(const UiDocAnnotation& comment : doc_->GetComments()) {
            String text = comment.payload.Find("text") >= 0 ? AsString(comment.payload["text"]) : String("Comment");
            UiModelItem item(text.IsEmpty() ? String("Comment") : text, comment.id);
            item.description = Format("characters %d-%d", comment.range.from, comment.range.to);
            item.right_text = comment.resolved ? "Resolved" : "Open";
            item.icon = ICON_DESIGN_COMMENT_48();
            item.icon_render_mode = UiIconRenderMode::MonoTint;
            item.has_metadata = true;
            item.metadata_color = comment.resolved ? SColorDisabled() : Color(216, 112, 35);
            model.Add(item);
        }
        if(!active_comment_id_.IsEmpty())
            comment_list_.SetData(active_comment_id_);
    }

    void ResolveMetadataVisual(const UiDocAnnotation& metadata, Image& icon, Color& tint) const
    {
        icon = ICON_EDITOR_NOTES_48();
        tint = SColorHighlight();
        if(!doc_)
            return;
        for(const UiDoc::AnnotationLane& lane : doc_->GetAnnotationLanes())
            for(const String& type : lane.annotation_types)
                if(type == metadata.type) {
                    if(!lane.icon.IsEmpty())
                        icon = lane.icon;
                    tint = lane.color;
                    return;
                }
    }

    void RefreshMetadataList()
    {
        UiListModel& model = metadata_list_.Model();
        model.Clear();
        if(!doc_)
            return;
        for(const UiDocAnnotation& metadata : doc_->GetMetadata()) {
            String title = MetadataTitle(metadata);
            String type = MetadataTypeName(metadata);
            UiModelItem item(title, metadata.id);
            item.description = MetadataBody(metadata);
            item.right_text = type;
            ResolveMetadataVisual(metadata, item.icon, item.metadata_color);
            item.icon_render_mode = UiIconRenderMode::MonoTint;
            item.has_metadata = true;
            model.Add(item);
        }
        if(!active_metadata_id_.IsEmpty())
            metadata_list_.SetData(active_metadata_id_);
    }

    void RefreshCommentEditor()
    {
        if(!doc_)
            return;
        const UiDocAnnotation* active = FindComment(active_comment_id_);
        if(!active) {
            active_comment_id_.Clear();
            if(!comment_edit_.HasFocus())
                comment_edit_.Clear();
            comment_context_.SetText("Select text to add a comment, or choose an item below.");
            comment_update_.Enable(false);
            comment_delete_.Enable(false);
            comment_resolve_.Enable(false);
            return;
        }
        comment_edit_.SetTextUtf8(active->payload.Find("text") >= 0 ? AsString(active->payload["text"]) : String());
        comment_edit_.ClearDirty();
        comment_context_.SetText(Format("%s | characters %d-%d",
                                        active->resolved ? "Resolved" : "Open",
                                        active->range.from, active->range.to));
        comment_update_.Enable(true);
        comment_delete_.Enable(true);
        comment_resolve_.Enable(!active->resolved);
    }

    void RefreshMetadataEditor()
    {
        if(!doc_)
            return;
        const UiDocAnnotation* active = FindMetadata(active_metadata_id_);
        if(!active) {
            active_metadata_id_.Clear();
            metadata_update_.Enable(false);
            metadata_delete_.Enable(false);
            metadata_expand_.Enable(false);
            metadata_expand_.SetText("Show card");
            return;
        }

        String type = MetadataTypeName(*active);
        EnsureMetadataTypeOption(type);
        metadata_type_.SetDataSilently(type);
        metadata_title_.SetTextUtf8(MetadataTitle(*active));
        metadata_body_.SetTextUtf8(MetadataBody(*active));
        metadata_payload_.SetTextUtf8(PayloadJson(*active));
        metadata_body_.ClearDirty();
        metadata_payload_.ClearDirty();
        metadata_update_.Enable(true);
        metadata_delete_.Enable(true);
        metadata_expand_.Enable(true);
        metadata_expand_.SetText(active->expanded ? "Collapse" : "Show card");
    }

    const UiDocAnnotation* FindComment(const String& id) const
    {
        if(!doc_ || id.IsEmpty())
            return nullptr;
        for(const UiDocAnnotation& item : doc_->Model().GetAnnotations())
            if(item.id == id && (item.type == "comment" || item.type == "review.comment"))
                return &item;
        return nullptr;
    }

    const UiDocAnnotation* FindMetadata(const String& id) const
    {
        if(!doc_ || id.IsEmpty())
            return nullptr;
        for(const UiDocAnnotation& item : doc_->Model().GetAnnotations())
            if(item.id == id && item.type.StartsWith("metadata."))
                return &item;
        return nullptr;
    }

    static String MetadataTitle(const UiDocAnnotation& metadata)
    {
        int q = metadata.payload.Find("title");
        return q >= 0 ? AsString(metadata.payload[q]) : String("Metadata");
    }

    static String MetadataBody(const UiDocAnnotation& metadata)
    {
        int q = metadata.payload.Find("text");
        return q >= 0 ? AsString(metadata.payload[q]) : String();
    }

    static String MetadataTypeName(const UiDocAnnotation& metadata)
    {
        String type = metadata.type;
        if(type.StartsWith("metadata."))
            type = type.Mid(9);
        return type.IsEmpty() ? String("note") : type;
    }

    void EnsureMetadataTypeOption(const String& type)
    {
        for(int i = 0; i < metadata_type_.GetCount(); i++)
            if(AsString(metadata_type_.GetItemData(i)) == type)
                return;
        String label = ToUpper(type.Left(1)) + type.Mid(1);
        metadata_type_.Add(label, type);
    }

    static ValueMap UserPayload(const UiDocAnnotation& metadata)
    {
        ValueMap payload = clone(metadata.payload);
        int q = payload.Find("title");
        if(q >= 0)
            payload.Remove(q);
        q = payload.Find("text");
        if(q >= 0)
            payload.Remove(q);
        return payload;
    }

    static String PayloadJson(const UiDocAnnotation& metadata)
    {
        return AsJSON(UserPayload(metadata), true);
    }

    bool ParsePayload(ValueMap& out)
    {
        out.Clear();
        String text = TrimBoth(metadata_payload_.GetTextUtf8());
        if(text.IsEmpty())
            return true;
        Value parsed = ParseJSON(text);
        if(parsed.IsError() || !IsValueMap(parsed)) {
            WhenStatus("Payload must be a JSON object");
            return false;
        }
        ValueMap parsed_map = parsed;
        out = clone(parsed_map);
        return true;
    }

    int CurrentParagraphStart() const
    {
        if(!doc_)
            return 0;
        UiDocSelection selection = doc_->GetSelection();
        int at = clamp(selection.caret, 0, doc_->GetLength());
        const WString& text = doc_->GetTextW();
        while(at > 0 && text[at - 1] != '\n')
            --at;
        return at;
    }

    void SelectCommentFromList()
    {
        if(!doc_)
            return;
        String id = AsString(comment_list_.GetData());
        if(id.IsEmpty())
            return;
        active_comment_id_ = id;
        active_metadata_id_.Clear();
        doc_->RevealAnnotation(id, true);
        RefreshPanel();
    }

    void SelectMetadataFromList()
    {
        if(!doc_)
            return;
        String id = AsString(metadata_list_.GetData());
        if(id.IsEmpty())
            return;
        active_metadata_id_ = id;
        active_comment_id_.Clear();
        doc_->RevealAnnotation(id, false);
        RefreshPanel();
    }

    void AddComment()
    {
        if(!doc_)
            return;
        String text = TrimBoth(comment_edit_.GetTextUtf8());
        if(text.IsEmpty())
            text = "Review comment";
        String id = doc_->AddComment(text);
        if(id.IsEmpty()) {
            WhenStatus("Unable to add comment");
            return;
        }
        active_comment_id_ = id;
        active_metadata_id_.Clear();
        doc_->RevealAnnotation(id, true);
        WhenStatus("Comment added");
        RefreshPanel();
    }

    void UpdateComment()
    {
        if(!doc_ || active_comment_id_.IsEmpty())
            return;
        String text = TrimBoth(comment_edit_.GetTextUtf8());
        if(text.IsEmpty()) {
            WhenStatus("Comment text is empty");
            return;
        }
        if(!doc_->UpdateComment(active_comment_id_, text)) {
            WhenStatus("Unable to update comment");
            return;
        }
        WhenStatus("Comment updated");
        RefreshPanel();
    }

    void DeleteComment()
    {
        if(!doc_ || active_comment_id_.IsEmpty() || !doc_->RemoveComment(active_comment_id_)) {
            WhenStatus("No active comment to delete");
            return;
        }
        active_comment_id_.Clear();
        doc_->SetActiveAnnotation(String());
        WhenStatus("Comment deleted");
        RefreshPanel();
    }

    void ResolveComment()
    {
        if(!doc_ || active_comment_id_.IsEmpty() || !doc_->ResolveComment(active_comment_id_, true)) {
            WhenStatus("No active comment to resolve");
            return;
        }
        doc_->SetActiveAnnotation(String());
        WhenStatus("Comment resolved");
        RefreshPanel();
    }

    void AddMetadata()
    {
        if(!doc_)
            return;
        ValueMap payload;
        if(!ParsePayload(payload))
            return;
        String type = metadata_type_.HasSelection() ? AsString(metadata_type_.GetSelectedData()) : String("note");
        String title = TrimBoth(metadata_title_.GetTextUtf8());
        if(title.IsEmpty())
            title = ToUpper(type.Left(1)) + type.Mid(1) + " reference";
        String body = TrimBoth(metadata_body_.GetTextUtf8());
        if(body.IsEmpty())
            body = "Non-printing reference metadata anchored to this paragraph.";
        int at = CurrentParagraphStart();
        String id = doc_->AddMetadata(UiDocRange(at, at), type, title, body, payload);
        if(id.IsEmpty()) {
            WhenStatus("Unable to add metadata");
            return;
        }
        active_metadata_id_ = id;
        active_comment_id_.Clear();
        doc_->ShowMetadata(true);
        doc_->SetMetadataExpanded(id, true);
        doc_->RevealAnnotation(id, false);
        WhenStatus("Metadata added");
        RefreshPanel();
    }

    void UpdateMetadata()
    {
        if(!doc_ || active_metadata_id_.IsEmpty())
            return;
        ValueMap payload;
        if(!ParsePayload(payload))
            return;
        String type = metadata_type_.HasSelection() ? AsString(metadata_type_.GetSelectedData()) : MetadataTypeName(*FindMetadata(active_metadata_id_));
        String title = TrimBoth(metadata_title_.GetTextUtf8());
        String body = metadata_body_.GetTextUtf8();
        if(!doc_->UpdateMetadata(active_metadata_id_, type, title, body, payload)) {
            WhenStatus("Unable to update metadata");
            return;
        }
        doc_->RevealAnnotation(active_metadata_id_, false);
        WhenStatus("Metadata updated");
        RefreshPanel();
    }

    void DeleteMetadata()
    {
        if(!doc_ || active_metadata_id_.IsEmpty() || !doc_->RemoveMetadata(active_metadata_id_)) {
            WhenStatus("No active metadata to delete");
            return;
        }
        active_metadata_id_.Clear();
        doc_->SetActiveAnnotation(String());
        WhenStatus("Metadata deleted");
        RefreshPanel();
    }

    void ToggleMetadataCard()
    {
        if(!doc_ || active_metadata_id_.IsEmpty())
            return;
        const UiDocAnnotation* active = FindMetadata(active_metadata_id_);
        if(!active)
            return;
        bool next = !active->expanded;
        if(!doc_->SetMetadataExpanded(active_metadata_id_, next))
            return;
        doc_->RevealAnnotation(active_metadata_id_, false);
        WhenStatus(next ? "Metadata card shown" : "Metadata card collapsed");
        RefreshPanel();
    }
};

}

#endif
