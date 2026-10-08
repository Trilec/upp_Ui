#include "UiFileBrowserModel.h"
#include <algorithm>
#include <map>
#include <vector>

namespace Upp
{

namespace
{

struct RawBrowserFile
{
    String path;
    String name;
    String extension;
    String prefix;
    int padding = 0;
    int64 frame = 0;
    int64 size = 0;
    Time modified;
    bool image = false;
    bool video = false;
    bool numbered = false;
};

String LowerExt(const String &value)
{
    // Public probes accept filenames too; .shot.1001.exr is not an extension token.
    bool token =
        value.StartsWith(".") && value.Find('.', 1) < 0 && value.Find('/') < 0 && value.Find('\\') < 0;
    return ToLower(token ? value : GetFileExt(value));
}

String UpperExtName(const String &ext)
{
    String text = ext;
    if(text.StartsWith("."))
        text = text.Mid(1);
    return ToUpper(text);
}

bool ParseNumberedImage(const String &name, String &prefix, int &padding, int64 &frame)
{
    String stem = GetFileTitle(name);
    int start = stem.GetCount();
    while(start > 0 && IsDigit(stem[start - 1]))
        --start;
    padding = stem.GetCount() - start;
    if(padding <= 0 || padding > 18)
        return false;
    frame = 0;
    for(int i = start; i < stem.GetCount(); ++i)
        frame = frame * 10 + stem[i] - '0';
    prefix = stem.Left(start);
    return true;
}

int64 Gcd64(int64 a, int64 b)
{
    a = abs(a);
    b = abs(b);
    while(b)
    {
        int64 t = a % b;
        a = b;
        b = t;
    }
    return a;
}

String FriendlySequenceName(const String &prefix)
{
    String out = prefix;
    while(!out.IsEmpty() && (out.EndsWith(".") || out.EndsWith("_") || out.EndsWith("-")))
        out.Trim(out.GetCount() - 1);
    return out.IsEmpty() ? String("Sequence") : out;
}

int EntryRank(const UiFileBrowserEntry &e)
{
    if(e.kind == UiFileBrowserEntryKind::Folder)
        return 0;
    if(e.kind == UiFileBrowserEntryKind::Sequence)
        return 1;
    if(e.kind == UiFileBrowserEntryKind::Video)
        return 2;
    if(e.kind == UiFileBrowserEntryKind::Image)
        return 3;
    return 4;
}

} // namespace

int UiFileBrowserEntry::GetExpectedFrameCount() const
{
    if(!IsSequence() || increment <= 0 || last < first)
        return IsSequence() ? frames.GetCount() : 0;
    int64 count = (last - first) / increment + 1;
    return count > INT_MAX ? INT_MAX : (int)count;
}

int UiFileBrowserEntry::GetPresentFrameCount() const
{
    if(!IsSequence())
        return 0;
    int count = 0;
    for(const UiFileBrowserFrame &f : frames)
        if(!f.missing)
            ++count;
    return count;
}

String UiFileBrowserFilterRule::GetTitle() const
{
    String field_name;
    switch(field)
    {
    case UiFileBrowserFilterField::Type:
        field_name = "Type";
        break;
    case UiFileBrowserFilterField::SequenceStatus:
        field_name = "Sequence";
        break;
    case UiFileBrowserFilterField::Name:
    default:
        field_name = "Name";
        break;
    }
    return String(include ? "Include · " : "Exclude · ") + field_name + " · " +
           (value.IsEmpty() ? String("*") : value);
}

bool UiFileBrowserModel::IsSupportedImageExtension(const String &ext)
{
    String e = LowerExt(ext);
    return e == ".exr" || e == ".jpg" || e == ".jpeg" || e == ".png" || e == ".tif" || e == ".tiff" ||
           e == ".hdr" || e == ".dpx";
}

bool UiFileBrowserModel::IsSupportedVideoExtension(const String &ext)
{
    String e = LowerExt(ext);
    return e == ".mp4" || e == ".mov" || e == ".m4v";
}

bool UiFileBrowserModel::IsSupportedMediaExtension(const String &ext)
{
    return IsSupportedImageExtension(ext) || IsSupportedVideoExtension(ext);
}

UiFileBrowserFileType UiFileBrowserModel::GetFileType(const UiFileBrowserEntry &entry)
{
    using Type = UiFileBrowserFileType;
    if(entry.IsDirectory())
        return Type::Folder;
    if(entry.IsImage())
        return Type::Image;
    if(entry.IsVideo())
        return Type::Video;
    String ext = ToLower(entry.extension);
    String name = ToLower(entry.name);
    auto in = [&](const char *extensions) { return String(extensions).Find("|" + ext + "|") >= 0; };
    if(name == "readme" || name == "license" || name == "copying" || name == "authors" ||
       name == "changelog" || name == "news")
        return Type::Text;
    if(name == "makefile" || name == "dockerfile" || name == "cmakelists.txt")
        return Type::Code;
    if(name == ".gitignore" || name == ".gitattributes" || name == ".editorconfig" || name == ".bashrc" ||
       name == ".zshrc" || name == ".profile")
        return Type::Config;
    if(in("|.bmp|.gif|.webp|.avif|.heic|.heif|.jxl|.svg|.ico|.icns|.ppm|.pgm|"))
        return Type::Image;
    if(in("|.avi|.mkv|.webm|.mpg|.mpeg|.mxf|.mts|.m2ts|.vob|"))
        return Type::Video;
    if(in("|.wav|.mp3|.flac|.aif|.aiff|.aac|.ogg|.opus|.m4a|.wma|.mid|.midi|"))
        return Type::Audio;
    if(in("|.exe|.com|.msi|.msix|.appx|.dll|.so|.dylib|.run|.appimage|.deb|.rpm|.pkg|"))
        return Type::Executable;
    if(in("|.bat|.cmd|.ps1|.sh|.bash|.zsh|.fish|.py|.lua|.pl|.rb|"))
        return Type::Script;
    if(in("|.txt|.md|.rst|.log|.nfo|.asc|"))
        return Type::Text;
    if(in("|.pdf|.doc|.docx|.odt|.rtf|"))
        return Type::Document;
    if(in("|.xls|.xlsx|.ods|.csv|.tsv|"))
        return Type::Spreadsheet;
    if(in("|.ppt|.pptx|.odp|"))
        return Type::Presentation;
    if(in("|.zip|.7z|.rar|.tar|.gz|.bz2|.xz|.zst|.tgz|.dmg|.iso|"))
        return Type::Archive;
    if(in("|.cpp|.c|.h|.hpp|.cc|.cxx|.cs|.js|.ts|.tsx|.jsx|.html|.css|.scss|.glsl|.rs|.go|.java|.swift|.m|."
          "mm|"))
        return Type::Code;
    if(in("|.ini|.cfg|.conf|.toml|.yaml|.yml|.plist|.env|.reg|"))
        return Type::Config;
    if(in("|.json|.xml|.db|.sqlite|.bin|.dat|"))
        return Type::Data;
    if(in("|.ttf|.otf|.woff|.woff2|"))
        return Type::Font;
    if(in("|.obj|.fbx|.abc|.usd|.usda|.usdc|.stl|.gltf|.glb|"))
        return Type::Model;
    if(in("|.mtl|.sbs|.sbsar|.mat|"))
        return Type::Material;
    if(in("|.nk|.ntp|.hip|.hipnc|.blend|.ma|.mb|.aep|.prproj|.psd|.upp|"))
        return Type::Project;
    if(in("|.lnk|.url|.desktop|"))
        return Type::Shortcut;
    return Type::Other;
}

String UiFileBrowserModel::FormatBytes(int64 bytes)
{
    if(bytes < 0)
        return "—";
    static const char *suffix[] = {"B", "KB", "MB", "GB", "TB"};
    double value = (double)bytes;
    int unit = 0;
    while(value >= 1024.0 && unit < 4)
    {
        value /= 1024.0;
        ++unit;
    }
    return unit == 0 ? Format("%lld B", bytes)
                     : Format(value >= 100  ? "%.0f %s"
                              : value >= 10 ? "%.1f %s"
                                            : "%.2f %s",
                              value, suffix[unit]);
}

String UiFileBrowserModel::FormatModified(const Time &tm)
{
    if(IsNull(tm))
        return "—";
    return Format("%04d-%02d-%02d  %02d:%02d", tm.year, tm.month, tm.day, tm.hour, tm.minute);
}

int UiFileBrowserModel::CompareNames(const String &a, const String &b)
{
    String left = ToLower(a), right = ToLower(b);
    int i = 0, j = 0;
    while(i < left.GetCount() && j < right.GetCount())
    {
        if(IsDigit(left[i]) && IsDigit(right[j]))
        {
            int ie = i, je = j;
            while(ie < left.GetCount() && IsDigit(left[ie]))
                ++ie;
            while(je < right.GetCount() && IsDigit(right[je]))
                ++je;
            int is = i, js = j;
            while(is < ie && left[is] == '0')
                ++is;
            while(js < je && right[js] == '0')
                ++js;
            if(ie - is != je - js)
                return SgnCompare(ie - is, je - js);
            for(int k = 0; k < ie - is; ++k)
                if(left[is + k] != right[js + k])
                    return SgnCompare(left[is + k], right[js + k]);
            if(ie - i != je - j)
                return SgnCompare(ie - i, je - j);
            i = ie;
            j = je;
        }
        else
        {
            if(left[i] != right[j])
                return SgnCompare(left[i], right[j]);
            ++i;
            ++j;
        }
    }
    return SgnCompare(left.GetCount() - i, right.GetCount() - j);
}

bool UiFileBrowserModel::MatchesFilter(const UiFileBrowserEntry &entry, const UiFileBrowserFilterRule &rule)
{
    String needle = ToLower(TrimBoth(rule.value));
    if(needle.IsEmpty() || needle == "*")
        return true;

    switch(rule.field)
    {
    case UiFileBrowserFilterField::Name:
        return ToLower(entry.name).Find(needle) >= 0 || ToLower(entry.friendly_name).Find(needle) >= 0 ||
               ToLower(entry.pattern).Find(needle) >= 0;
    case UiFileBrowserFilterField::Type:
    {
        String type = ToLower(entry.type_label + " " + entry.extension);
        if(needle == "sequence" || needle == "sequences")
            return entry.IsSequence();
        if(needle == "folder" || needle == "folders")
            return entry.IsDirectory();
        if(needle == "video" || needle == "videos")
            return entry.IsVideo();
        if(needle == "image" || needle == "images")
            return entry.IsImage();
        return type.Find(needle) >= 0;
    }
    case UiFileBrowserFilterField::SequenceStatus:
        if(!entry.IsSequence())
            return false;
        if(needle.Find("missing") >= 0 || needle.Find("incomplete") >= 0)
            return !entry.complete;
        if(needle.Find("complete") >= 0)
            return entry.complete;
        return false;
    }
    return true;
}

bool UiFileBrowserModel::Scan(const String &folder, bool group_sequences, String &error,
                              const std::function<bool()> &cancelled,
                              const std::function<void(int)> &progress)
{
    error.Clear();
    entries_.Clear();

    String normalized = NormalizePath(folder);
    if(normalized.IsEmpty() || !DirectoryExists(normalized))
    {
        error = "Folder does not exist: " + folder;
        return false;
    }
    folder_ = normalized;

    std::vector<RawBrowserFile> raw;
    auto stopped = [&] { return cancelled && cancelled(); };
    int scanned = 0;
    if(stopped())
        return false;
    FindFile ff(AppendFileName(folder_, "*"));
#ifdef PLATFORM_WIN32
    if(!ff)
    {
        DWORD code = GetLastError();
        if(code != ERROR_FILE_NOT_FOUND && code != ERROR_NO_MORE_FILES)
        {
            error = "Cannot enumerate folder: " + folder_ + " · " + GetErrorMessage(code);
            return false;
        }
    }
#endif
    for(; ff; ff.Next())
    {
        if(stopped())
            return false;
        String name = ff.GetName();
        if(name == "." || name == "..")
            continue;
        if(progress && (++scanned % 256) == 0)
            progress(scanned);
        if(ff.IsFolder())
        {
            UiFileBrowserEntry &e = entries_.Add();
            e.kind = UiFileBrowserEntryKind::Folder;
            e.path = AppendFileName(folder_, name);
            e.name = e.friendly_name = name;
            e.type_label = "Folder";
            e.modified = ff.GetLastWriteTime();
            continue;
        }
        if(!ff.IsFile())
            continue;

        RawBrowserFile r;
        r.path = AppendFileName(folder_, name);
        r.name = name;
        // This input is always a filename, including extensionless dotfiles.
        r.extension = name.ReverseFind('.') > 0 ? ToLower(GetFileExt(name)) : String();
        r.size = ff.GetLength();
        r.modified = ff.GetLastWriteTime();
        r.image = IsSupportedImageExtension(r.extension);
        r.video = IsSupportedVideoExtension(r.extension);
        if(r.image)
            r.numbered = ParseNumberedImage(name, r.prefix, r.padding, r.frame);
        raw.push_back(r);
    }
#ifdef PLATFORM_WIN32
    DWORD enumeration_error = GetLastError();
    if(enumeration_error != ERROR_NO_MORE_FILES && enumeration_error != ERROR_FILE_NOT_FOUND &&
       enumeration_error != ERROR_SUCCESS)
    {
        error = "Folder scan interrupted: " + folder_ + " · " + GetErrorMessage(enumeration_error);
        entries_.Clear();
        return false;
    }
#endif
    if(progress)
        progress(scanned);

    std::map<std::string, std::vector<int>> groups;
    std::vector<bool> consumed(raw.size(), false);
    if(group_sequences)
    {
        for(int i = 0; i < (int)raw.size(); ++i)
        {
            if((i % 256) == 0 && stopped())
                return false;
            const RawBrowserFile &r = raw[i];
            if(!r.image || !r.numbered)
                continue;
            // Win32 paths are case-insensitive; POSIX prefixes are not.
#ifdef PLATFORM_WIN32
            String prefix = ToLower(r.prefix);
#else
            String prefix = r.prefix;
#endif
            String key = prefix + "|" + AsString(r.padding) + "|" + r.extension;
            groups[std::string(~key)].push_back(i);
        }
    }

    for(auto &group_pair : groups)
    {
        if(stopped())
            return false;
        auto &members = group_pair.second;
        if(members.size() < 2)
            continue;

        std::sort(members.begin(), members.end(), [&](int a, int b) { return raw[a].frame < raw[b].frame; });

        int64 first = raw[members.front()].frame;
        int64 last = raw[members.back()].frame;
        if(last < first || last - first >= 1000000)
            continue;

        int64 step = 0;
        for(size_t i = 1; i < members.size(); ++i)
            step = Gcd64(step, raw[members[i]].frame - raw[members[i - 1]].frame);
        if(step <= 0 || step > INT_MAX)
            step = 1;

        const RawBrowserFile &seed = raw[members.front()];
        UiFileBrowserEntry &e = entries_.Add();
        e.kind = UiFileBrowserEntryKind::Sequence;
        e.path = seed.path;
        e.name = seed.name;
        e.friendly_name = FriendlySequenceName(seed.prefix);
        e.pattern = seed.prefix + "%0" + AsString(seed.padding) + "d" + seed.extension;
        e.extension = seed.extension;
        e.type_label = UpperExtName(seed.extension) + " sequence";
        e.first = first;
        e.last = last;
        e.increment = (int)step;
        e.padding = seed.padding;
        e.size = 0;
        e.modified = seed.modified;

        std::map<int64, int> by_frame;
        for(int index : members)
        {
            consumed[index] = true;
            by_frame[raw[index].frame] = index;
            e.size += raw[index].size;
            if(raw[index].modified > e.modified)
                e.modified = raw[index].modified;
        }

        for(int64 number = first; number <= last; number += step)
        {
            // Sparse sequences can contain nearly a million missing frames.
            // Superseded scans must cancel during expansion, not only enumeration.
            if((e.frames.GetCount() % 256) == 0 && stopped())
                return false;
            UiFileBrowserFrame &frame = e.frames.Add();
            frame.number = number;
            auto it = by_frame.find(number);
            if(it == by_frame.end())
            {
                frame.missing = true;
                e.complete = false;
            }
            else
            {
                const RawBrowserFile &source = raw[it->second];
                frame.path = source.path;
                frame.size = source.size;
                frame.modified = source.modified;
            }
            if(number > INT64_MAX - step)
                break;
        }
    }

    for(int i = 0; i < (int)raw.size(); ++i)
    {
        if((i % 256) == 0 && stopped())
            return false;
        if(consumed[i])
            continue;
        const RawBrowserFile &r = raw[i];
        UiFileBrowserEntry &e = entries_.Add();
        e.path = r.path;
        e.name = e.friendly_name = r.name;
        e.extension = r.extension;
        e.size = r.size;
        e.modified = r.modified;
        if(r.video)
        {
            e.kind = UiFileBrowserEntryKind::Video;
            e.type_label = UpperExtName(r.extension) + " video";
        }
        else if(r.image)
        {
            e.kind = UiFileBrowserEntryKind::Image;
            e.type_label = UpperExtName(r.extension) + " image";
        }
        else
        {
            e.kind = UiFileBrowserEntryKind::File;
            String ext = UpperExtName(r.extension);
            e.type_label = ext.IsEmpty() ? String("File") : ext + " file";
        }
    }

    Sort(entries_,
         [](const UiFileBrowserEntry &a, const UiFileBrowserEntry &b)
         {
             int ar = EntryRank(a), br = EntryRank(b);
             if(ar != br)
                 return ar < br;
             return CompareNames(a.friendly_name, b.friendly_name) < 0;
         });

    return true;
}

} // namespace Upp
