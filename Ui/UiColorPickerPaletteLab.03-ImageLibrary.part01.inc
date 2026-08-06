{
    static Vector<StaticPaletteDefinition> palette;
    ONCELOCK {
        palette.Add(MakeHuePalette("full-spectrum", "Full Spectrum", 6, 96, 100, 5, 12));
        palette.Add(MakeHuePalette("strong-vivid", "Strong Vivid", 5, 100, 100, 4, 13));

        StaticPaletteDefinition pastel;
        pastel.id = "pastel-soft"; pastel.name = "Pastel Soft"; pastel.category = "Creative";
        pastel.preferred_columns = 12;
        for(int row = 0; row < 4; row++)
            for(int column = 0; column < 12; column++)
                AddPaletteColor(pastel, HslToColor(column * 30, 48 - row * 6, 91 - row * 8));
        palette.Add(pick(pastel));

        StaticPaletteDefinition tint;
        tint.id = "light-tints"; tint.name = "Light Tints"; tint.category = "Creative";
        tint.preferred_columns = 12;
        for(int row = 0; row < 4; row++)
            for(int column = 0; column < 12; column++)
                AddPaletteColor(tint, HslToColor(column * 30, 62 - row * 7, 96 - row * 6));
        palette.Add(pick(tint));

        StaticPaletteDefinition earth;
        earth.id = "earth-muted"; earth.name = "Earth Muted"; earth.category = "Creative";
        earth.preferred_columns = 12;
        const int earth_hue[12] = { 10, 20, 31, 42, 55, 70, 86, 104, 126, 150, 178, 205 };
        for(int row = 0; row < 5; row++)
            for(int column = 0; column < 12; column++)
                AddPaletteColor(earth, HslToColor(earth_hue[column], 58 - row * 6, 28 + row * 12));
        palette.Add(pick(earth));

        StaticPaletteDefinition ui;
        ui.id = "bright-ui"; ui.name = "Bright UI Accents"; ui.category = "Creative";
        ui.preferred_columns = 5;
        const Color bases[5] = { Color(34,197,94), Color(245,158,11), Color(239,68,68),
                                 Color(59,130,246), Color(168,85,247) };
        const int mix[5] = { 185, 110, 0, -55, -115 };
        for(int row = 0; row < 5; row++)
            for(int column = 0; column < 5; column++)
                AddPaletteColor(ui, mix[row] >= 0 ? Blend(bases[column], White(), mix[row])
                                                  : Blend(bases[column], Black(), -mix[row]));
        palette.Add(pick(ui));

        StaticPaletteDefinition grey;
        grey.id = "grayscale"; grey.name = "Grayscale"; grey.category = "Creative";
        grey.preferred_columns = 12;
        for(int i = 0; i < 12; i++) {
            int level = 255 - i * 23;
            AddPaletteColor(grey, Color(level, level, level), Format("Gray %d", level));
        }
        palette.Add(pick(grey));

        StaticPaletteDefinition web;
        web.id = "web-safe"; web.name = "Web Safe"; web.category = "Creative";
        web.preferred_columns = 12;
        const int levels[6] = { 0, 51, 102, 153, 204, 255 };
        for(int r : levels)
            for(int g : levels)
                for(int b : levels)
                    AddPaletteColor(web, Color(r, g, b));
        palette.Add(pick(web));

        palette.Add(MakeHuePalette("wide-gamut", "HDR / Wide Gamut Reference", 5, 100, 100, 2, 9));

        StaticPaletteDefinition skin;
        skin.id = "skin-tones"; skin.name = "Skin Tones"; skin.category = "Reference";
        skin.source_reference = "Monk Skin Tone screen-reference sequence";
        skin.preferred_columns = 11;
        const Color skin_values[11] = {
            Color(246,237,228), Color(243,231,219), Color(247,234,208), Color(234,218,186),
            Color(215,189,150), Color(181,151,111), Color(160,126,86), Color(130,92,67),
            Color(96,65,52), Color(62,43,35), Color(41,30,25)
        };
        for(int i = 0; i < 11; i++) AddPaletteColor(skin, skin_values[i], Format("Tone %d", i + 1));
        palette.Add(pick(skin));

        StaticPaletteDefinition xrite;
        xrite.id = "xrite-colorchecker"; xrite.name = "X-Rite ColorChecker";
        xrite.category = "Calibration"; xrite.authoritative = false;
        xrite.source_reference = "ColorChecker Classic 24-patch sRGB screen reference; not measured chart truth";
        xrite.preferred_columns = 6;
        const Color xrite_values[24] = {
            Color(0x73,0x52,0x44), Color(0xC2,0x96,0x82), Color(0x62,0x7A,0x9D), Color(0x57,0x6C,0x43),
            Color(0x85,0x80,0xB1), Color(0x67,0xBD,0xAA), Color(0xD6,0x7E,0x2C), Color(0x50,0x5B,0xA6),
            Color(0xC1,0x5A,0x63), Color(0x5E,0x3C,0x73), Color(0x9D,0xBC,0x40), Color(0xE0,0xA3,0x2E),
            Color(0x38,0x3D,0x88), Color(0x46,0x94,0x49), Color(0xAF,0x36,0x3C), Color(0xF3,0xC3,0x00),
            Color(0x9C,0x5A,0xA5), Color(0x00,0xA1,0xC7), Color(0xF3,0xF3,0xF2), Color(0xC8,0xC8,0xC8),
            Color(0xA0,0xA0,0xA0), Color(0x7A,0x7A,0x7A), Color(0x55,0x55,0x55), Color(0x34,0x34,0x34)
        };
        for(int i = 0; i < 24; i++) AddPaletteColor(xrite, xrite_values[i], Format("Patch %02d", i + 1));
        palette.Add(pick(xrite));

        StaticPaletteDefinition big;
        big.id = "big-color-gray"; big.name = "B.I.G. Color & Gray Card";
        big.category = "Calibration"; big.source_reference = "Prior UiColorPicker 9-patch screen reference";
        big.preferred_columns = 3;
        const Color big_values[9] = {
            Color(245,245,245), Color(119,119,119), Color(35,35,35), Color(175,50,50),
            Color(50,150,50), Color(50,50,170), Color(0,180,180), Color(180,50,140), Color(220,190,30)
        };
        for(int i = 0; i < 9; i++) AddPaletteColor(big, big_values[i], Format("Patch %d", i + 1));
        palette.Add(pick(big));

        StaticPaletteDefinition video;
        video.id = "passport-video"; video.name = "ColorChecker Passport Video";
        video.category = "Calibration"; video.source_reference = "Prior UiColorPicker 24-patch screen reference";
        video.preferred_columns = 6;
        const Color video_values[24] = {
            Color(255,0,0), Color(0,255,0), Color(0,0,255), Color(0,255,255), Color(255,0,255), Color(255,255,0),
            Color(245,215,196), Color(232,184,154), Color(212,154,122), Color(184,122,92), Color(154,92,66), Color(122,66,46),
            Color(245,245,245), Color(208,208,208), Color(171,171,171), Color(134,134,134), Color(97,97,97), Color(60,60,60),
            Color(255,250,240), Color(240,230,210), Color(46,46,46), Color(31,31,31), Color(255,107,61), Color(61,107,255)
        };
        for(int i = 0; i < 24; i++) AddPaletteColor(video, video_values[i], Format("Video %02d", i + 1));
        palette.Add(pick(video));

        StaticPaletteDefinition spyder24;
        spyder24.id = "spyder-24"; spyder24.name = "SpyderCheckr 24"; spyder24.category = "Calibration";
        spyder24.source_reference = "Prior UiColorPicker 24-patch screen reference"; spyder24.preferred_columns = 6;
        const Color spyder_values[24] = {
            Color(157,107,83), Color(199,154,125), Color(110,139,90), Color(90,122,184),
            Color(184,90,110), Color(83,110,157), Color(142,76,54), Color(196,150,110),
            Color(78,62,48), Color(175,119,84), Color(122,139,175), Color(191,175,153),
            Color(240,240,240), Color(192,192,192), Color(144,144,144), Color(96,96,96),
            Color(48,48,48), Color(0,0,0), Color(255,0,0), Color(0,255,0),
            Color(0,0,255), Color(255,255,0), Color(255,0,255), Color(0,255,255)
        };
        for(int i = 0; i < 24; i++) AddPaletteColor(spyder24, spyder_values[i], Format("Patch %02d", i + 1));
        palette.Add(pick(spyder24));

        StaticPaletteDefinition spyder48;
        spyder48.id = "spyder-48"; spyder48.name = "SpyderCheckr 48"; spyder48.category = "Calibration";
