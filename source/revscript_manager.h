#ifndef RME_REVSCRIPT_MANAGER_H_
#define RME_REVSCRIPT_MANAGER_H_

#include "main.h"
#include <map>
#include <string>
#include <vector>

struct RevScriptEntry {
    std::string filename;
    std::string function_name;
    int line_number;
    int id_value;
    std::string id_type; // "aid", "uid", or "id"
};

class RevScriptManager {
public:
    RevScriptManager();
    ~RevScriptManager();
    
    // Scan the revscripts directory for files
    bool scanRevScriptsDirectory(const std::string& scripts_directory);
    
    // Find revscript entries by action ID
    std::vector<RevScriptEntry> findByActionID(int action_id) const;
    
    // Find revscript entries by unique ID  
    std::vector<RevScriptEntry> findByUniqueID(int unique_id) const;
    
    // Find revscript entries by item ID
    std::vector<RevScriptEntry> findByItemID(int item_id) const;
    
    // Open a revscript file in external editor
    bool openRevScript(const RevScriptEntry& entry) const;
    
    // Clear all loaded revscript data
    void clear();
    
    // Check if revscripts are loaded
    bool isLoaded() const { return loaded; }
    
    // Get the base scripts directory
    const std::string& getScriptsDirectory() const { return scripts_directory; }
    
    // Get statistics for debugging
    std::string getScanStatistics() const;

private:
    std::string scripts_directory;
    bool loaded;
    
    // Maps to store revscript entries by their IDs
    std::map<int, std::vector<RevScriptEntry>> action_id_map;
    std::map<int, std::vector<RevScriptEntry>> unique_id_map;
    std::map<int, std::vector<RevScriptEntry>> item_id_map;
    
    // Helper methods
    bool scanLuaFile(const std::string& filepath);
    void scanDirectoryRecursive(const std::string& dir_path);
    void addEntry(const RevScriptEntry& entry);
    std::string getDefaultEditor() const;
};

#endif // RME_REVSCRIPT_MANAGER_H_ 