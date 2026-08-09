
class UiColorPicker::Impl {
public:
    Impl(UiColorPicker& owner);
    ~Impl();

    void Layout();
    void SyncTheme();

    UiColorPicker& SetPageMode(PageMode mode);
    UiColorPicker& SetChannelMode(ChannelMode mode);
    UiColorPicker& SetSlotCount(int count);
    UiColorPicker& SetActiveSlot(int index);
    UiColorPicker& SetSlot(int index, Color color, int alpha, bool fire);
    UiColorPicker& SetSlotColor(int index, Color color, bool fire);
    UiColorPicker& SetSlotAlpha(int index, int alpha, bool fire);
    UiColorPicker& SetSlotLabel(int index, const String& label);
    UiColorPicker& SetAlphaEnabled(bool enabled);
    UiColorPicker& SetSpectrumMode(SpectrumMode mode);
    UiColorPicker& SetHarmonyMode(HarmonyMode mode);
    UiColorPicker& SetDistributionMode(DistributionMode mode);
    UiColorPicker& SetMediumMode(MediumMode mode);
    UiColorPicker& SetGeneratorCount(int count);
    UiColorPicker& SetGeneratorImage(const Image& image);
    UiColorPicker& ExtractGeneratorPalette(int count);
    UiColorPicker& AddUserSwatches(const Vector<SlotValue>& values, bool transactional);
    UiColorPicker& ClearUserSwatches();
    UiColorPicker& ClearRecentSwatches();
    UiColorPicker& EnableSessionPersistence(bool enabled);
    UiColorPicker& BeginEyedropper();

    PageMode GetPageMode() const { return page_mode_; }
    ChannelMode GetChannelMode() const { return channel_mode_; }
    int GetSlotCount() const { return slot_count_; }
    int GetActiveSlot() const { return active_slot_; }
    Color GetSlotColor(int index) const;
    int GetSlotAlpha(int index) const;
    SlotValue GetSlot(int index) const;
    Vector<SlotValue> GetSlots() const;
    String GetSlotLabel(int index) const;
    bool IsAlphaEnabled() const { return alpha_enabled_; }
    SpectrumMode GetSpectrumMode() const { return spectrum_mode_; }
    HarmonyMode GetHarmonyMode() const { return recipe_.harmony; }
    DistributionMode GetDistributionMode() const { return recipe_.distribution; }
    MediumMode GetMediumMode() const { return recipe_.medium; }
    int GetGeneratorCount() const { return recipe_.requested_count; }
    Vector<SlotValue> GetGeneratedPalette() const;
    const Image& GetGeneratorImage() const { return image_; }
    Vector<SlotValue> GetImagePalette() const;
    int GetUserSwatchCount() const { return stash_.GetCount(); }
    int GetRecentSwatchCount() const { return recent_.GetCount(); }
    Vector<SlotValue> GetUserSwatches() const { return clone(stash_); }
    bool IsSessionPersistenceEnabled() const { return session_persistence_; }
    bool IsEyedropperAvailable() const;
    bool IsEyedropperActive() const { return eyedropper_active_; }

    void Accept();
    void Cancel();
    void FinishLiveGesture();
    void StopEyedropper(bool commit);
    void SampleEyedropper(bool final_commit);

    bool Key(dword key, int count);
    void CancelMode();

private:
    void BuildTree();
    void ConfigureControls();
    void WireEvents();
    void BuildColorPage();
    void BuildPalettesPage();
    void BuildGeneratorPage();
    void BuildImagePage();

    void LoadSession();
    void SaveSession(bool include_slots);

    void SyncAll();
    void SyncSlots();
    void SyncPage();
    void SyncColorControls();
    void SyncChannelRows();
    void ConfigureChannelRows();
    void SyncReadouts();
    void SyncFooter(const SlotValue& value);
    void PushRecent(Color color, int alpha);
    void RefreshStash();
    void HandleStashDrop(int target, const Vector<SlotValue>& values);
    void AddActiveSelection();

    void CommitColor(Color color, bool final_commit);
    void CommitAlpha(int alpha, bool final_commit);
    void CommitSlotValue(int slot, Color color, int alpha, bool final_commit);
    bool ApplyColorText(const String& text, bool final_commit);
    void HandleChannelValue(int row, double value, bool final_commit);

    void PopulateStaticPaletteDrop();
    void SetStaticPalette(int index);
    void SyncPaletteSelection(int index, const SlotValue& value);
    void AddPaletteSelection(bool all);

    void PopulateGeneratorControls();
    void RefreshGenerator(bool preserve_selection = true);
    void SyncGeneratorSelection(int index, const SlotValue& value);
    void SyncSelectedFamily();
    void AddGeneratorSelection(bool all);
    void SendImageToGenerator();

    void PopulateImageControls();
    void LoadImage();
    void MarkImageStale();
    void AnalyzeCurrentImage(bool mask_only = false);
    void RefreshImageDisplay(bool preserve_selection = true);
    void SyncImageSelection(int index, const SlotValue& value);
    void AddImageSelection(bool all);
    void EnsureManualPoints();
    void ResampleManualPoint(int index);
    void SyncImageStatus();

    void SavePaletteJson();
    void LoadPaletteJson();

    UiColorPicker& owner_;

    Vector<SlotValue> slots_;
    Vector<SlotValue> previous_;
    Vector<SlotValue> opening_;
    Vector<SlotValue> recent_;
    Vector<SlotValue> stash_;
    int slot_count_ = 4;
    int active_slot_ = 0;
    bool alpha_enabled_ = true;
    StashDropMode stash_drop_mode_ = STASH_REPLACE;
    PageMode page_mode_ = PAGE_COLOR;
    SpectrumMode spectrum_mode_ = SPECTRUM_HUE_STRIP;
    ChannelMode channel_mode_ = CHANNEL_RGB_FLOAT;
    int remembered_hue_ = 200;

    bool syncing_ = false;
    bool live_gesture_ = false;
    int live_slot_ = -1;
    SlotValue live_origin_;
    dword last_live_callback_ = 0;
    bool session_persistence_ = true;
    bool eyedropper_active_ = false;
    bool eyedropper_dragging_ = false;

    int palette_index_ = 0;
    GeneratorRecipe recipe_;
    Vector<GeneratedSwatch> generated_;
    int active_family_ = 0;

    Image image_;
    ImageAnalysisSettings image_settings_;
    ImageAnalysisResult image_result_;
    Vector<GeneratedSwatch> image_display_;
    Vector<Pointf> manual_points_;
    bool image_stale_ = false;
    bool image_show_mask_ = false;
    int image_gain_ = 0;
    int image_saturation_ = 100;
    int image_hero_ = -1;
    int image_hero_gain_ = 0;

    SlotValue footer_value_;

    UiBoxLayout root_ { UiDirection::V };
    UiBoxLayout navigation_ { UiDirection::H };
    UiButton page_button_[PAGE_COUNT];
    UiToolButton current_preview_;
    One<UiBoxLayout> slot_column_[8];
    UiToolButton primary_slot_[8];
    UiToolButton previous_slot_[8];

    UiStack page_stack_;
    UiBoxLayout color_page_ { UiDirection::V };
    UiBoxLayout palettes_page_ { UiDirection::V };
    UiBoxLayout generator_page_ { UiDirection::V };
    UiBoxLayout image_page_ { UiDirection::V };

    UiBoxLayout stash_section_ { UiDirection::V };
    UiBoxLayout stash_header_ { UiDirection::H };
    UiLabel stash_title_;
    UiDropdown stash_mode_drop_;
    UiButton stash_add_selected_;
    UiButton stash_export_;
    UiButton stash_import_;
    UiButton stash_clear_;
    SwatchFlow_ stash_flow_;

    UiBoxLayout footer_ { UiDirection::H };
    UiLineEdit footer_hex_;
    UiLineEdit footer_detail_;
    UiButton accept_button_;
    UiButton cancel_button_;

    UiBoxLayout color_columns_ { UiDirection::H };
    UiBoxLayout color_left_ { UiDirection::V };
    UiBoxLayout color_right_ { UiDirection::V };
    LabeledDrop_ spectrum_field_;
    One<ColorField_> color_field_;
    UiBoxLayout color_mode_row_ { UiDirection::H };
    UiDropdown channel_drop_;
    UiToolButton eyedropper_button_;
    UiLabel alpha_label_;
    UiToggle alpha_toggle_;
    UiBoxLayout channel_rows_ { UiDirection::V };
    One<NumericRow_> numeric_row_[5];
    int channel_row_count_ = 4;
    One<SliderRow_> hue_row_;
