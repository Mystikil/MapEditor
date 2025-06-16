#ifndef RME_MONSTER_MANAGER_H_
#define RME_MONSTER_MANAGER_H_

#include "main.h"
#include <map>
#include <string>
#include <vector>

struct MonsterEntry {
    std::string name;
    std::string filename;
    std::string full_path;   // Full absolute path to monster file
    bool loaded;
    
    // Basic monster properties that can be read from the XML
    std::string race;
    int experience;
    int speed;
    int manacost;
    int health_max;
    
    // Look properties
    int look_type;
    int look_head;
    int look_body;
    int look_legs;
    int look_feet;
    int look_addons;
    int look_mount;
    
    MonsterEntry() : loaded(false), experience(0), speed(0), manacost(0), 
                     health_max(0), look_type(0), look_head(0), look_body(0), 
                     look_legs(0), look_feet(0), look_addons(0), look_mount(0) {}
};

class MonsterManager {
public:
    MonsterManager();
    ~MonsterManager();
    
    // Scan the data directory for monsters.xml and load monster entries
    bool scanMonstersDirectory(const std::string& data_directory);
    
    // Find monster entry by name
    MonsterEntry* findByName(const std::string& name) const;
    
    // Get all loaded monsters
    const std::map<std::string, MonsterEntry>& getAllMonsters() const { return monster_entries; }
    
    // Load detailed monster data from individual XML file
    bool loadMonsterDetails(MonsterEntry& entry) const;
    
    // Create a new monster XML file
    bool createMonsterXML(const MonsterEntry& entry, const std::string& output_path) const;
    
    // Check if monsters are loaded
    bool isLoaded() const { return loaded; }
    
    // Get the base data directory
    const std::string& getDataDirectory() const { return data_directory; }
    
    // Clear all loaded monster data
    void clear();
    
    // Get statistics for debugging
    std::string getScanStatistics() const;

private:
    std::map<std::string, MonsterEntry> monster_entries;
    std::string data_directory;
    bool loaded;
    
    // Helper methods
    bool parseMonstersXML(const std::string& monsters_xml_path);
    bool parseMonsterXML(const std::string& monster_xml_path, MonsterEntry& entry) const;
};

#endif // RME_MONSTER_MANAGER_H_ 