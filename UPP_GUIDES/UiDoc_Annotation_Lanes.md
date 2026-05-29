# UiDoc Annotation Lanes

`UiDoc` keeps annotations semantic and keeps gutter markers as view
configuration. This avoids tying the document model to one product domain such
as comments, treatments, budget notes, or image references.

## Model

- `UiDocAnnotation` owns durable data: range, type, payload, expanded,
  printable, and resolved state.
- Annotation ranges are remapped through document edits.
- `UiDoc::AnnotationLane` maps one or more annotation types to a visible marker
  lane.
- Lanes own marker color, icon, shape, visibility, side, and order.

## Lane Side

Each lane can choose where it appears:

- `LANE_AUTO`: follows `UiDoc::SetGutterSide`.
- `LANE_LEFT`: always appears in the left gutter.
- `LANE_RIGHT`: always appears in the right gutter.
- `LANE_BOTH`: appears on both sides.

This allows script-review tools to keep structural metadata on the left and
producer/review comments on the right without changing the annotation model.

## Defaults

The default registry contains:

- `metadata`: blue square, generic non-comment annotations.
- `table`: blue square, table/embed markers.
- `comments`: orange circle, `note`, `comment`, and `review.comment`.

The legacy marker color/icon setters update these default lanes, so existing
callers keep working.

## Example

```cpp
UiDoc::AnnotationLane treatment;
treatment.id = "treatment";
treatment.label = "Treatment";
treatment.annotation_types.Add("script.treatment");
treatment.color = Color(32, 116, 226);
treatment.shape = UiDoc::MARKER_SQUARE;
treatment.side = UiDoc::LANE_LEFT;
doc.AddAnnotationLane(treatment);

UiDoc::AnnotationLane budget;
budget.id = "budget";
budget.label = "Budget";
budget.annotation_types.Add("script.budget");
budget.color = Color(64, 160, 96);
budget.shape = UiDoc::MARKER_TRIANGLE;
budget.side = UiDoc::LANE_RIGHT;
doc.AddAnnotationLane(budget);
```

Marker clicks select the matching annotation range and emit
`WhenMetadataMarker(annotation_id, lane_id, line)`.
