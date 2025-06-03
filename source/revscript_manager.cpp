#include "main.h"
#include "revscript_manager.h"
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/process.h>
#include <wx/utils.h>
#include <wx/stdpaths.h>
#include <regex>
#include <fstream>

RevScriptManager::RevScriptManager() : loaded(false) {
}

RevScriptManager::~RevScriptManager() {
    clear();
}

bool RevScriptManager::scanRevScriptsDirectory(const std::string& scripts_dir) {
    clear();
    scripts_directory = scripts_dir;
    
    if (!wxDir::Exists(wxstr(scripts_dir))) {
        return false;
    }
    
    // Use recursive function to scan all directories
    scanDirectoryRecursive(scripts_dir);
    
    loaded = true;
    
    // Debug output - count total entries found
    int total_entries = 0;
    for (const auto& pair : action_id_map) {
        total_entries += pair.second.size();
    }
    for (const auto& pair : unique_id_map) {
        total_entries += pair.second.size();
    }
    for (const auto& pair : item_id_map) {
        total_entries += pair.second.size();
    }
    
    return true;
}

void RevScriptManager::scanDirectoryRecursive(const std::string& dir_path) {
    wxDir dir(wxstr(dir_path));
    if (!dir.IsOpened()) {
        return;
    }
    
    wxString filename;
    
    // First, scan all .lua files in current directory
    bool found = dir.GetFirst(&filename, "*.lua", wxDIR_FILES);
    while (found) {
        wxString fullpath = wxstr(dir_path) + wxFileName::GetPathSeparator() + filename;
        scanLuaFile(nstr(fullpath));
        found = dir.GetNext(&filename);
    }
    
    // Then recursively scan all subdirectories
    found = dir.GetFirst(&filename, wxEmptyString, wxDIR_DIRS);
    while (found) {
        // Skip hidden directories and system directories
        if (!filename.StartsWith(".")) {
            wxString subdirpath = wxstr(dir_path) + wxFileName::GetPathSeparator() + filename;
            scanDirectoryRecursive(nstr(subdirpath));
        }
        found = dir.GetNext(&filename);
    }
}

bool RevScriptManager::scanLuaFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    int line_number = 0;
    std::string current_function = "";
    
    // Regex patterns for different ID types
    std::regex aid_pattern(R"((\w+):aid\s*\(\s*(\d+)\s*\))");
    std::regex uid_pattern(R"((\w+):uid\s*\(\s*(\d+)\s*\))");
    std::regex id_pattern(R"((\w+):id\s*\(\s*(\d+)\s*\))");
    std::regex function_pattern(R"(function\s+(\w+[\w\.]*)\s*\()");
    
    while (std::getline(file, line)) {
        line_number++;
        
        // Check for function definitions to track context
        std::smatch function_match;
        if (std::regex_search(line, function_match, function_pattern)) {
            current_function = function_match[1].str();
        }
        
        // Check for action ID patterns
        std::smatch aid_match;
        if (std::regex_search(line, aid_match, aid_pattern)) {
            RevScriptEntry entry;
            entry.filename = filepath;
            entry.function_name = current_function;
            entry.line_number = line_number;
            entry.id_value = std::stoi(aid_match[2].str());
            entry.id_type = "aid";
            addEntry(entry);
        }
        
        // Check for unique ID patterns
        std::smatch uid_match;
        if (std::regex_search(line, uid_match, uid_pattern)) {
            RevScriptEntry entry;
            entry.filename = filepath;
            entry.function_name = current_function;
            entry.line_number = line_number;
            entry.id_value = std::stoi(uid_match[2].str());
            entry.id_type = "uid";
            addEntry(entry);
        }
        
        // Check for item ID patterns
        std::smatch id_match;
        if (std::regex_search(line, id_match, id_pattern)) {
            RevScriptEntry entry;
            entry.filename = filepath;
            entry.function_name = current_function;
            entry.line_number = line_number;
            entry.id_value = std::stoi(id_match[2].str());
            entry.id_type = "id";
            addEntry(entry);
        }
    }
    
    return true;
}

void RevScriptManager::addEntry(const RevScriptEntry& entry) {
    if (entry.id_type == "aid") {
        action_id_map[entry.id_value].push_back(entry);
    } else if (entry.id_type == "uid") {
        unique_id_map[entry.id_value].push_back(entry);
    } else if (entry.id_type == "id") {
        item_id_map[entry.id_value].push_back(entry);
    }
}

std::vector<RevScriptEntry> RevScriptManager::findByActionID(int action_id) const {
    auto it = action_id_map.find(action_id);
    if (it != action_id_map.end()) {
        return it->second;
    }
    return std::vector<RevScriptEntry>();
}

std::vector<RevScriptEntry> RevScriptManager::findByUniqueID(int unique_id) const {
    auto it = unique_id_map.find(unique_id);
    if (it != unique_id_map.end()) {
        return it->second;
    }
    return std::vector<RevScriptEntry>();
}

std::vector<RevScriptEntry> RevScriptManager::findByItemID(int item_id) const {
    auto it = item_id_map.find(item_id);
    if (it != item_id_map.end()) {
        return it->second;
    }
    return std::vector<RevScriptEntry>();
}

bool RevScriptManager::openRevScript(const RevScriptEntry& entry) const {
    // Use the default Windows application to open the file
    wxString filename = wxstr(entry.filename);
    return wxLaunchDefaultApplication(filename);
}

std::string RevScriptManager::getDefaultEditor() const {
    // This method is no longer needed since we use default app
    // but keeping it for potential future use
    return "";
}

void RevScriptManager::clear() {
    action_id_map.clear();
    unique_id_map.clear();
    item_id_map.clear();
    loaded = false;
    scripts_directory.clear();
}

std::string RevScriptManager::getScanStatistics() const {
    int action_ids = 0, unique_ids = 0, item_ids = 0;
    
    for (const auto& pair : action_id_map) {
        action_ids += pair.second.size();
    }
    for (const auto& pair : unique_id_map) {
        unique_ids += pair.second.size();
    }
    for (const auto& pair : item_id_map) {
        item_ids += pair.second.size();
    }
    
    std::string result = "RevScript Statistics:\n";
    result += "Directory: " + scripts_directory + "\n";
    result += "Action IDs: " + std::to_string(action_ids) + "\n";
    result += "Unique IDs: " + std::to_string(unique_ids) + "\n";
    result += "Item IDs: " + std::to_string(item_ids) + "\n";
    result += "Total entries: " + std::to_string(action_ids + unique_ids + item_ids);
    
    return result;
} 