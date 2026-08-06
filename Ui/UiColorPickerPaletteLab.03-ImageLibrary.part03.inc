            bin.weight = pow((double)bin.count, 0.65) * (0.25 + min(1.0, edge_distance / 35.0));
            break;
        }
        case UiColorPicker::COVERAGE_DISTINCTIVE:
            bin.weight = pow((double)bin.count, 0.45) * (0.20 + min(2.0, chroma / 28.0));
            break;
        case UiColorPicker::COVERAGE_BALANCED:
        default:
            bin.weight = pow((double)bin.count, 0.65);
            break;
        }
        if(settings.analysis == UiColorPicker::IMAGE_INTERFACE) {
            double extreme = max(fabs(bin.lab.l - 50.0) / 50.0, chroma / 70.0);
            bin.weight *= 0.75 + extreme;
        }
        else if(settings.analysis == UiColorPicker::IMAGE_ACCENT_FINDER)
            bin.weight *= 0.35 + min(2.5, chroma / 22.0);
        else if(settings.analysis == UiColorPicker::IMAGE_NATURE_SCENE)
            bin.weight *= 0.75 + min(1.25, chroma / 45.0);
        else if(settings.analysis == UiColorPicker::IMAGE_PAINT_MATERIAL)
            bin.weight *= bin.count < valid_pixels / 300 ? 0.15 : 1.0;
        bin.weight = max(0.0001, bin.weight);
    }
    if(bins.IsEmpty()) {
        result.diagnostic = "The image produced no stable colour bins.";
        return result;
    }

    int cluster_count = min(settings.requested_count, bins.GetCount());
    Vector<LabValue> centroid;
    centroid.SetCount(cluster_count);
    Vector<int> seed_index;
    seed_index.SetCount(cluster_count, -1);
    int first = 0;
    for(int i = 1; i < bins.GetCount(); i++)
        if(bins[i].weight > bins[first].weight + 1e-12 ||
           (fabs(bins[i].weight - bins[first].weight) <= 1e-12 && bins[i].key < bins[first].key))
            first = i;
    seed_index[0] = first;
    centroid[0] = bins[first].lab;
    for(int c = 1; c < cluster_count; c++) {
        int best = -1;
        double best_score = -1.0;
        for(int i = 0; i < bins.GetCount(); i++) {
            bool already = false;
            for(int j = 0; j < c; j++)
                already = already || seed_index[j] == i;
            if(already)
                continue;
            double distance = DBL_MAX;
            for(int j = 0; j < c; j++)
                distance = min(distance, LabDistanceSquared(bins[i].lab, centroid[j]));
            double score = distance * sqrt(bins[i].weight);
            if(score > best_score + 1e-12 ||
               (fabs(score - best_score) <= 1e-12 && (best < 0 || bins[i].key < bins[best].key))) {
                best = i; best_score = score;
            }
        }
        if(best < 0) best = c % bins.GetCount();
        seed_index[c] = best;
        centroid[c] = bins[best].lab;
    }

    Vector<int> assignment;
    assignment.SetCount(bins.GetCount(), 0);
    Vector<double> cluster_weight;
    cluster_weight.SetCount(cluster_count, 0.0);
    for(int iteration = 0; iteration < 10; iteration++) {
        Vector<double> sl, sa, sb;
        sl.SetCount(cluster_count, 0.0); sa.SetCount(cluster_count, 0.0); sb.SetCount(cluster_count, 0.0);
        cluster_weight.SetCount(cluster_count, 0.0);
        for(int i = 0; i < bins.GetCount(); i++) {
            int best = 0;
            double best_distance = LabDistanceSquared(bins[i].lab, centroid[0]);
            for(int c = 1; c < cluster_count; c++) {
                double distance = LabDistanceSquared(bins[i].lab, centroid[c]);
                if(distance < best_distance - 1e-12) { best = c; best_distance = distance; }
            }
            assignment[i] = best;
            double weight = bins[i].weight;
            sl[best] += bins[i].lab.l * weight;
            sa[best] += bins[i].lab.a * weight;
            sb[best] += bins[i].lab.b * weight;
            cluster_weight[best] += weight;
        }
        for(int c = 0; c < cluster_count; c++)
            if(cluster_weight[c] > 0.0) {
                centroid[c].l = sl[c] / cluster_weight[c];
                centroid[c].a = sa[c] / cluster_weight[c];
                centroid[c].b = sb[c] / cluster_weight[c];
            }
    }

    Vector<int> representative;
    representative.SetCount(cluster_count, -1);
    for(int i = 0; i < bins.GetCount(); i++) {
        int c = assignment[i];
        if(representative[c] < 0 ||
           LabDistanceSquared(bins[i].lab, centroid[c]) < LabDistanceSquared(bins[representative[c]].lab, centroid[c]) - 1e-12 ||
           (fabs(LabDistanceSquared(bins[i].lab, centroid[c]) - LabDistanceSquared(bins[representative[c]].lab, centroid[c])) <= 1e-12 &&
            bins[i].key < bins[representative[c]].key))
            representative[c] = i;
    }
    Vector<int> order;
    for(int c = 0; c < cluster_count; c++)
        order.Add(c);
    for(int i = 0; i < order.GetCount(); i++) {
        int best = i;
        for(int j = i + 1; j < order.GetCount(); j++) {
            int a = order[best], b = order[j];
            if(cluster_weight[b] > cluster_weight[a] + 1e-12 ||
               (fabs(cluster_weight[b] - cluster_weight[a]) <= 1e-12 && b < a))
                best = j;
        }
        Swap(order[i], order[best]);
    }

    for(int rank = 0; rank < order.GetCount(); rank++) {
        int c = order[rank];
        int ri = representative[c] >= 0 ? representative[c] : seed_index[c];
        const ImageBin& bin = bins[ri];
        GeneratedSwatch& swatch = result.swatches.Add();
        swatch.value = Slot(bin.color, 255, Format("Extracted %d", rank + 1));
        swatch.source_index = ri;
        swatch.hero = false;
        Pointf position((bin.sum_x / double(max(1, bin.count))) / max(1, size.cx - 1),
                        (bin.sum_y / double(max(1, bin.count))) / max(1, size.cy - 1));
        result.representative_positions.Add(position);
    }
    result.diagnostic = Format("Analyzed %d pixels through a %dx%d proxy; %.1f%% excluded.",
                               valid_pixels, size.cx, size.cy, result.ignored_fraction * 100.0);
    return result;
}

Vector<GeneratedSwatch> ApplyImagePostProcessing(const Vector<GeneratedSwatch>& source,
                                                  UiColorPicker::MediumMode medium,
                                                  int global_gain,
                                                  int global_saturation,
                                                  int hero_index,
                                                  int hero_gain)
{
    Vector<GeneratedSwatch> output = clone(source);
    MediumProfile profile = Profile(medium);
    global_gain = minmax(global_gain, -50, 50);
    global_saturation = minmax(global_saturation, 0, 150);
    hero_gain = minmax(hero_gain, -50, 50);
    for(int i = 0; i < output.GetCount(); i++) {
        int h = 0, s = 0, v = 0;
        ColorToHsv(output[i].value.color, h, s, v);
        double sat = s * global_saturation / 100.0 * profile.saturation_character;
        sat = min(sat, profile.saturation_limit * 100.0);
        int value = v + global_gain + (i == hero_index ? hero_gain : 0);
        value = minmax(value, profile.minimum_value, profile.maximum_value);
        output[i].value.color = HsvToColor(h, sat, value);
        output[i].hero = i == hero_index;
    }
    return output;
}

bool AddUniqueTransactional(Vector<UiColorPicker::SlotValue>& destination,
                            const Vector<UiColorPicker::SlotValue>& source,
                            int capacity, bool allow_duplicates, int *rejected)
{
    capacity = max(0, capacity);
    Vector<UiColorPicker::SlotValue> append;
    for(const UiColorPicker::SlotValue& value : source) {
        if(IsNull(value.color))
            continue;
