#include "core/sentinel_scanner.h"

SentinelScanner::SentinelScanner(std::string sentinel) {
    sentinel_value = sentinel;
}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    if (sentinel_value.empty()) {
        Out result;
        result.safe_text = std::string(chunk);
        result.sentinel_found = true;
        return result;
    }

    std::string combined = pending_text + std::string(chunk);
    std::size_t keep = sentinel_value.size() - 1;

    std::size_t pos = combined.find(sentinel_value);
    if (pos != std::string::npos) {
        Out result;
        result.safe_text = combined.substr(0, pos);
        result.sentinel_found = true;
        pending_text.clear();
        return result;
    }

    if (combined.size() <= keep) {
        pending_text = combined;
        Out result;
        result.safe_text = "";
        result.sentinel_found = false;
        return result;
    }

    std::size_t safe_len = combined.size() - keep;
    Out result;
    result.safe_text = combined.substr(0, safe_len);
    result.sentinel_found = false;
    pending_text = combined.substr(safe_len);
    return result;
}

SentinelScanner::Out SentinelScanner::flush() {
    Out result;
    if (pending_text.empty()) {
        result.safe_text = "";
        result.sentinel_found = false;
        return result;
    }

    result.safe_text = pending_text;
    result.sentinel_found = false;
    pending_text.clear();
    return result;
}
