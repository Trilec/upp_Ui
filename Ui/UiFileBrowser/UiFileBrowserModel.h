/*
    Headless directory catalogue: metadata records, numeric sequence grouping,
    missing-frame detection, natural sorting and filter predicates. Scans build
    a replacement catalogue and accept cancellation/progress hooks. No GUI or
    imaging dependency; file-family classification does not imply decode support.
*/
#ifndef _UiFileBrowser_UiFileBrowserModel_h_
#define _UiFileBrowser_UiFileBrowserModel_h_

#include <Core/Core.h>
#include <functional>

namespace Upp
{

enum class UiFileBrowserEntryKind : byte
{
    Folder,
    File,
    Image,
    Video,
    Sequence
};

enum class UiFileBrowserFileType : byte
{
    Folder,
    Image,
    Video,
    Audio,
    Executable,
    Script,
    Text,
    Document,
    Spreadsheet,
    Presentation,
    Archive,
    Code,
    Config,
    Data,
    Font,
    Model,
    Material,
    Project,
    Shortcut,
    Other
};

enum class UiFileBrowserFilterField : byte
{
    Name,
    Type,
    SequenceStatus
};

struct UiFileBrowserFrame : Moveable<UiFileBrowserFrame>
{
    int64 number = 0;
    String path;
    int64 size = 0;
    Time modified;
    bool missing = false;
};

struct UiFileBrowserEntry : Moveable<UiFileBrowserEntry>
{
    UiFileBrowserEntryKind kind = UiFileBrowserEntryKind::File;
    String path;
    String name;
    String friendly_name;
    String pattern;
    String extension;
    String type_label;
    int64 size = 0;
    Time modified;
    int64 first = 0;
    int64 last = 0;
    int increment = 1;
    int padding = 0;
    Vector<UiFileBrowserFrame> frames;
    bool complete = true;

    bool IsDirectory() const { return kind == UiFileBrowserEntryKind::Folder; }
    bool IsSequence() const { return kind == UiFileBrowserEntryKind::Sequence; }
    bool IsImage() const
    {
        return kind == UiFileBrowserEntryKind::Image || kind == UiFileBrowserEntryKind::Sequence;
    }
    bool IsVideo() const { return kind == UiFileBrowserEntryKind::Video; }
    int GetExpectedFrameCount() const;
    int GetPresentFrameCount() const;
};

struct UiFileBrowserFilterRule : Moveable<UiFileBrowserFilterRule>
{
    bool include = true;
    UiFileBrowserFilterField field = UiFileBrowserFilterField::Name;
    String value;

    String GetTitle() const;
};

class UiFileBrowserModel
{
public:
    bool Scan(const String &folder, bool group_sequences, String &error,
              const std::function<bool()> &cancelled = {}, const std::function<void(int)> &progress = {});

    int GetCount() const { return entries_.GetCount(); }
    const UiFileBrowserEntry &operator[](int i) const { return entries_[i]; }
    const Vector<UiFileBrowserEntry> &Entries() const { return entries_; }
    String GetFolder() const { return folder_; }

    static bool IsSupportedImageExtension(const String &ext);
    static bool IsSupportedVideoExtension(const String &ext);
    static bool IsSupportedMediaExtension(const String &ext);
    static bool MatchesFilter(const UiFileBrowserEntry &entry, const UiFileBrowserFilterRule &rule);
    static String FormatBytes(int64 bytes);
    static String FormatModified(const Time &tm);
    static int CompareNames(const String &a, const String &b);
    static UiFileBrowserFileType GetFileType(const UiFileBrowserEntry &entry);

private:
    String folder_;
    Vector<UiFileBrowserEntry> entries_;
};

} // namespace Upp

#endif
