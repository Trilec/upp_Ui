#include "UiOsFileDialog.h"

#ifdef PLATFORM_COCOA

#import <Cocoa/Cocoa.h>

namespace Upp {
namespace {

static NSString* ToNsString(const String& s)
{
    return [NSString stringWithUTF8String:s.ToStd().c_str()];
}

static String ToUppString(NSString* s)
{
    return s ? String([s UTF8String]) : String();
}

static NSArray<NSString*>* BuildAllowedExtensions(const Vector<UiOsFileDialog::Filter>& filters, int index)
{
    NSMutableArray<NSString*>* out = [NSMutableArray array];
    if(index < 0 || index >= filters.GetCount())
        return out;

    const auto& f = filters[index];
    for(const String& raw_pattern : f.patterns) {
        String p = TrimBoth(raw_pattern);

        if(p == "*" || p == "*.*")
            continue;

        if(p.StartsWith("*."))
            p = p.Mid(2);
        else if(p.StartsWith("."))
            p = p.Mid(1);

        if(p.Find('*') >= 0 || p.Find('?') >= 0)
            continue;

        p = TrimBoth(p);
        if(!p.IsEmpty())
            [out addObject:ToNsString(p)];
    }

    return out;
}

}

bool UiOsFileDialog::ExecuteMac(Ctrl*)
{
    result_paths_.Clear();

    @autoreleasepool {
        if(mode_ == Mode::SaveFile) {
            NSSavePanel* panel = [NSSavePanel savePanel];

            if(!title_.IsEmpty())
                [panel setTitle:ToNsString(title_)];
            if(!initial_directory_.IsEmpty())
                [panel setDirectoryURL:[NSURL fileURLWithPath:ToNsString(initial_directory_)]];
            if(!suggested_name_.IsEmpty())
                [panel setNameFieldStringValue:ToNsString(suggested_name_)];

            if(!default_extension_.IsEmpty())
                [panel setAllowedFileTypes:@[ToNsString(default_extension_)]];
            else if(filter_index_ >= 0 && filter_index_ < filters_.GetCount()) {
                NSArray<NSString*>* ext = BuildAllowedExtensions(filters_, filter_index_);
                if([ext count] > 0)
                    [panel setAllowedFileTypes:ext];
            }

            [panel setCanCreateDirectories:create_prompt_];
            [panel setExtensionHidden:NO];

            NSInteger rc = [panel runModal];
            if(rc != NSModalResponseOK)
                return false;

            NSURL* url = [panel URL];
            if(url)
                SetSingleResult(ToUppString([url path]));

            return !result_paths_.IsEmpty();
        }

        NSOpenPanel* panel = [NSOpenPanel openPanel];

        if(!title_.IsEmpty())
            [panel setTitle:ToNsString(title_)];
        if(!initial_directory_.IsEmpty())
            [panel setDirectoryURL:[NSURL fileURLWithPath:ToNsString(initial_directory_)]];
        if(mode_ != Mode::PickFolder && filter_index_ >= 0 && filter_index_ < filters_.GetCount()) {
            NSArray<NSString*>* ext = BuildAllowedExtensions(filters_, filter_index_);
            if([ext count] > 0)
                [panel setAllowedFileTypes:ext];
        }

        [panel setCanChooseDirectories:(mode_ == Mode::PickFolder)];
        [panel setCanChooseFiles:(mode_ != Mode::PickFolder)];
        [panel setAllowsMultipleSelection:(mode_ == Mode::OpenFiles)];
        [panel setResolvesAliases:follow_aliases_];
        [panel setCanCreateDirectories:create_prompt_];

        NSInteger rc = [panel runModal];
        if(rc != NSModalResponseOK)
            return false;

        for(NSURL* url in [panel URLs]) {
            if(url)
                AddResult(ToUppString([url path]));
        }

        return !result_paths_.IsEmpty();
    }
}

}
#endif