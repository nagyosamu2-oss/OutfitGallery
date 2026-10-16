#pragma once
#include <Windows.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include "DeletionPath.h"

namespace Gallery {
// Render-thread transaction. The preset/photo identifier never changes: no JSON,
// equipment, categories, order, or automatic assignments are rewritten.
class PhotoUpdate {
public:
    explicit PhotoUpdate(std::filesystem::path root) : root_(std::move(root)) {}
    const std::string& Photo() const { return photo_; }
    std::filesystem::path Target() const { return root_/photo_; }
    const std::filesystem::path& Draft() const { return draft_; }
    void Begin(const std::string& photo) {
        const std::filesystem::path name(photo);
        if(name.filename()!=name || name.extension()!=L".png" || !IsDirectDeletionChild(root_/name,root_))
            throw std::runtime_error("Invalid photo filename.");
        Cancel(); photo_=photo;
    }
    void Stage(const std::filesystem::path& path) {
        if(photo_.empty() || !IsDirectDeletionChild(path,root_/"PhotoRetake") || path.extension()!=L".png" ||
           !std::filesystem::is_regular_file(path) || !std::filesystem::file_size(path))
            throw std::runtime_error("Invalid photo preview.");
        if(path==draft_) return;
        DiscardDraft(); draft_=path;
    }
    void Cancel() noexcept { DiscardDraft(); photo_.clear(); }
    void Commit() {
        if(photo_.empty() || draft_.empty() || !std::filesystem::is_regular_file(draft_) || !std::filesystem::file_size(draft_))
            throw std::runtime_error("No photo preview to save.");
        const auto target=Target();
        // Copy beside the destination before the final rename: MO2 can map the
        // capture directory and draft directory to different physical volumes.
        auto staging=target; staging+=L".photo-update.tmp";
        if(!CopyFileW(draft_.c_str(),staging.c_str(),FALSE))
            throw std::runtime_error("Could not replace photo. Original photo kept; retry or cancel.");
        if(!MoveFileExW(staging.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
            std::error_code ec; std::filesystem::remove(staging,ec);
            throw std::runtime_error("Could not replace photo. Original photo kept; retry or cancel.");
        }
        // Keep draft until the UI releases its cached texture and calls Cancel.
    }
private:
    void DiscardDraft() noexcept {
        if(!draft_.empty()) {std::error_code ec;std::filesystem::remove(draft_,ec);draft_.clear();}
    }
    std::filesystem::path root_,draft_;
    std::string photo_;
};
}
