//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "creature_sprite_manager.h"
#include "creatures.h"
#include "creature.h"
#include "gui.h"
#include "creature_brush.h"

CreatureSpriteManager g_creature_sprites;

CreatureSpriteManager::CreatureSpriteManager() {
    // Constructor
}

CreatureSpriteManager::~CreatureSpriteManager() {
    clear();
}

void CreatureSpriteManager::clear() {
    // Delete all cached sprites
    for (auto& pair : sprite_cache) {
        delete pair.second;
    }
    sprite_cache.clear();
}

wxBitmap* CreatureSpriteManager::getSpriteBitmap(int looktype, int width, int height) {
    // Use a cache key that includes the dimensions
    int key = looktype;
    
    // Check if we already have this bitmap
    auto it = sprite_cache.find(key);
    if (it != sprite_cache.end()) {
        return it->second;
    }
    
    // Create a new bitmap
    wxBitmap* bitmap = createSpriteBitmap(looktype, width, height);
    if (bitmap) {
        sprite_cache[key] = bitmap;
    }
    
    return bitmap;
}

void CreatureSpriteManager::generateCreatureSprites(const BrushVector& creatures, int width, int height) {
    // Pre-generate sprites for a vector of creature brushes
    for (Brush* brush : creatures) {
        if (brush->isCreature()) {
            CreatureBrush* cb = static_cast<CreatureBrush*>(brush);
            if (cb->getType()) {
                int looktype = cb->getType()->outfit.lookType;
                if (looktype > 0) {
                    getSpriteBitmap(looktype, width, height);
                }
            }
        }
    }
}

wxBitmap* CreatureSpriteManager::createSpriteBitmap(int looktype, int width, int height) {
    // Create a bitmap for a creature with the specified looktype
    GameSprite* spr = g_gui.gfx.getCreatureSprite(looktype);
    if (!spr) {
        return nullptr;
    }
    
    // Use existing sprite drawing methods
    // Adjust for your setup - we'll use GameSprite's built-in DC drawing
    const int spriteSize = 32;  // Default sprite size
    
    // Create our bitmap with the specified dimensions
    wxBitmap* bitmap = new wxBitmap(width, height);
    wxMemoryDC dc(*bitmap);
    
    // Set background color
    dc.SetBackground(wxBrush(wxColour(255, 0, 255)));  // Magenta for transparency
    dc.Clear();
    
    // Draw the creature sprite centered in the bitmap
    if (spr) {
        // If we have a GameSprite, use its drawing method
        int offsetX = (width - spriteSize) / 2;
        int offsetY = (height - spriteSize) / 2;
        
        // Get the default appearance of this looktype
        Outfit outfit;
        outfit.lookType = looktype;
        
        // Try to find a creature with this looktype to get its default appearance
        for (CreatureDatabase::iterator it = g_creatures.begin(); it != g_creatures.end(); ++it) {
            CreatureType* type = it->second;
            if (type && type->outfit.lookType == looktype) {
                outfit = type->outfit;
                break;
            }
        }
        
        // Attempt to use the sprite's drawing function to render to our DC
        spr->DrawTo(&dc, width >= 32 ? SPRITE_SIZE_32x32 : SPRITE_SIZE_16x16, offsetX, offsetY);
    }
    
    // Ensure transparency works
    wxImage img = bitmap->ConvertToImage();
    img.SetMaskColour(255, 0, 255);
    *bitmap = wxBitmap(img);
    
    return bitmap;
} 