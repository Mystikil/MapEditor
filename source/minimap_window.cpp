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
/*
CURRENT SITUATION:
1. Minimap refresh issues:
   - Updates when changing floors (minimap_window.cpp:93-101)
   - Does not update properly after paste operations (copybuffer.cpp:232-257)
   - No caching mechanism for faster reloading
   - No binary file storage for minimap data

IMPROVEMENT TASKS:
1. Paste Operation Minimap Update:
   - Add minimap update calls in copybuffer.cpp after paste operations
   - Track modified positions during paste for efficient updates
   - Reference: copybuffer.cpp:232-257

2. Minimap Caching:
   - Implement block-based caching system (already started in MinimapBlock struct)
   - Add floor-specific caching
   - Optimize block size for better performance (currently 256)
   - Reference: minimap_window.cpp:53-55

3. Binary File Storage:
   - Create binary file format for minimap data
   - Store color information and floor data
   - Implement save/load functions
   - Cache binary data for quick loading

4. Drawing Optimization:
   - Batch rendering operations
   - Implement dirty region tracking
   - Use hardware acceleration where possible
   - Reference: minimap_window.cpp:290-339

RELEVANT CODE SECTIONS:
- Minimap window core: minimap_window.cpp:82-177
- Copy buffer paste: copybuffer.cpp:232-257
- Selection handling: selection.cpp:1-365
*/
#include "main.h"

#include "graphics.h"
#include "editor.h"
#include "map.h"

#include "gui.h"
#include "map_display.h"
#include "minimap_window.h"

#include <thread>
#include <mutex>
#include <atomic>

BEGIN_EVENT_TABLE(MinimapWindow, wxPanel)
	EVT_PAINT(MinimapWindow::OnPaint)
	EVT_ERASE_BACKGROUND(MinimapWindow::OnEraseBackground)
	EVT_LEFT_DOWN(MinimapWindow::OnMouseClick)
	EVT_KEY_DOWN(MinimapWindow::OnKey)
	EVT_SIZE(MinimapWindow::OnSize)
	EVT_CLOSE(MinimapWindow::OnClose)
	EVT_TIMER(ID_MINIMAP_UPDATE, MinimapWindow::OnDelayedUpdate)
	EVT_TIMER(ID_RESIZE_TIMER, MinimapWindow::OnResizeTimer)
END_EVENT_TABLE()

MinimapWindow::MinimapWindow(wxWindow* parent) : 
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(205, 130), wxFULL_REPAINT_ON_RESIZE),
	update_timer(this),
	thread_running(false),
	needs_update(true),
	last_center_x(0),
	last_center_y(0),
	last_floor(0),
	last_start_x(0),
	last_start_y(0),
	is_resizing(false)
{
	for (int i = 0; i < 256; ++i) {
		pens[i] = newd wxPen(wxColor(minimap_color[i].red, minimap_color[i].green, minimap_color[i].blue));
	}
	
	// Initialize the update timer
	update_timer.SetOwner(this, ID_MINIMAP_UPDATE);
	
	// Initialize the resize timer
	resize_timer.SetOwner(this, ID_RESIZE_TIMER);
	
	StartRenderThread();
	
	// Schedule initial loading after a short delay
	update_timer.Start(100, true); // Start a one-shot timer for 100ms
}

MinimapWindow::~MinimapWindow() {
	StopRenderThread();
	for (int i = 0; i < 256; ++i) {
		delete pens[i];
	}
}

void MinimapWindow::StartRenderThread() {
	thread_running = true;
	render_thread = std::thread(&MinimapWindow::RenderThreadFunction, this);
}

void MinimapWindow::StopRenderThread() {
	thread_running = false;
	if(render_thread.joinable()) {
		render_thread.join();
	}
}

void MinimapWindow::RenderThreadFunction() {
	while(thread_running) {
		if(needs_update && g_gui.IsEditorOpen()) {
			Editor& editor = *g_gui.GetCurrentEditor();
			MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
			
			int center_x, center_y;
			canvas->GetScreenCenter(&center_x, &center_y);
			int floor = g_gui.GetCurrentFloor();
			
			// Force update if floor changed
			if(floor != last_floor) {
				// Clear the buffer when floor changes
				std::lock_guard<std::mutex> lock(buffer_mutex);
				buffer = wxBitmap(GetSize().GetWidth(), GetSize().GetHeight());
				
				// Clear block cache
				std::lock_guard<std::mutex> blockLock(m_mutex);
				m_blocks.clear();
			}
			
			// Always update if floor changed or position changed
			if(center_x != last_center_x || 
			   center_y != last_center_y || 
			   floor != last_floor) {
				
				int window_width = GetSize().GetWidth();
				int window_height = GetSize().GetHeight();
				
				// Create temporary bitmap
				wxBitmap temp_buffer(window_width, window_height);
				wxMemoryDC dc(temp_buffer);
				
				dc.SetBackground(*wxBLACK_BRUSH);
				dc.Clear();
				
				int start_x = center_x - window_width / 2;
				int start_y = center_y - window_height / 2;
				
				// Batch drawing by color
				std::vector<std::vector<wxPoint>> colorPoints(256);
				
				for(int y = 0; y < window_height; ++y) {
					for(int x = 0; x < window_width; ++x) {
						int map_x = start_x + x;
						int map_y = start_y + y;
						
						if(map_x >= 0 && map_y >= 0 && 
						   map_x < editor.map.getWidth() && 
						   map_y < editor.map.getHeight()) {
							
							Tile* tile = editor.map.getTile(map_x, map_y, floor);
							if(tile) {
								uint8_t color = tile->getMiniMapColor();
								if(color) {
									colorPoints[color].push_back(wxPoint(x, y));
								}
							}
						}
					}
				}
				
				// Draw points by color
				for(int color = 0; color < 256; ++color) {
					if(!colorPoints[color].empty()) {
						dc.SetPen(*pens[color]);
						for(const wxPoint& pt : colorPoints[color]) {
							dc.DrawPoint(pt.x, pt.y);
						}
					}
				}
				
				// Update buffer safely
				{
					std::lock_guard<std::mutex> lock(buffer_mutex);
					buffer = temp_buffer;
				}
				
				// Store current state
				last_center_x = center_x;
				last_center_y = center_y;
				last_floor = floor;
				
				// Request refresh of the window
				wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED);
				evt.SetId(ID_MINIMAP_UPDATE);
				wxPostEvent(this, evt);
			}
			
			needs_update = false;
		}
		
		// Sleep to prevent excessive CPU usage
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

void MinimapWindow::OnSize(wxSizeEvent& event) {
	// Get the new size
	wxSize newSize = event.GetSize();
	
	// If we're already at this size, skip
	if (newSize == GetSize()) {
		event.Skip();
		return;
	}
	
	// Mark that we're resizing
	is_resizing = true;
	
	// Stop any previous resize timer
	if (resize_timer.IsRunning()) {
		resize_timer.Stop();
	}
	
	// Create a new buffer at the new size
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer = wxBitmap(newSize.GetWidth(), newSize.GetHeight());
	}
	
	// Clear the block cache since we're changing size
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_blocks.clear();
	}
	
	// Start the resize timer (will fire when resize is complete)
	resize_timer.Start(50, true); // Reduced to 50ms for faster response
	
	// Skip the event to allow default handling
	event.Skip();
}

void MinimapWindow::OnClose(wxCloseEvent& event) {
	// Hide the window instead of destroying it
	// This allows the minimap to be reopened in the same position
	if (wxWindow::GetParent()) {
		g_gui.HideMinimap();
		event.Veto(); // Prevent the window from being destroyed
	} else {
		event.Skip(); // Allow default handling if no parent
	}
}

void MinimapWindow::OnDelayedUpdate(wxTimerEvent& event) {
	if (g_gui.IsEditorOpen()) {
		// If editor is already open, load the minimap
		if (event.GetId() == ID_MINIMAP_UPDATE) {
			InitialLoad();
		}
	}
	
	needs_update = true;
}

void MinimapWindow::DelayedUpdate() {
	update_timer.Start(100, true);  // 100ms single-shot timer
}

void MinimapWindow::OnResizeTimer(wxTimerEvent& event) {
	// Resizing has stopped
	is_resizing = false;
	
	// Force a complete redraw with the new size
	needs_update = true;
	
	// Request a full refresh
	Refresh();
	
	// Trigger an immediate update
	if (g_gui.IsEditorOpen()) {
		Editor& editor = *g_gui.GetCurrentEditor();
		MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
		
		int center_x, center_y;
		canvas->GetScreenCenter(&center_x, &center_y);
		
		// Force update of the render thread
		last_center_x = center_x;
		last_center_y = center_y;
		last_floor = g_gui.GetCurrentFloor();
	}
}

void MinimapWindow::OnPaint(wxPaintEvent& event) {
	wxBufferedPaintDC dc(this);
	dc.SetBackground(*wxBLACK_BRUSH);
	dc.Clear();
	
	if (!g_gui.IsEditorOpen()) return;
	
	Editor& editor = *g_gui.GetCurrentEditor();
	MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
	
	int centerX, centerY;
	canvas->GetScreenCenter(&centerX, &centerY);
	int floor = g_gui.GetCurrentFloor();
	
	// Store current state
	last_center_x = centerX;
	last_center_y = centerY;
	last_floor = floor;

	// Get window dimensions
	int windowWidth = wxPanel::GetSize().GetWidth();
	int windowHeight = wxPanel::GetSize().GetHeight();
	
	// Draw header with map info (300px high)
	int headerHeight = 30;
	wxRect headerRect(0, 0, windowWidth, headerHeight);
	dc.SetBrush(wxBrush(wxColour(40, 40, 40)));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(headerRect);
	
	// Draw map info text
	dc.SetTextForeground(wxColour(220, 220, 220));
	wxFont font = dc.GetFont();
	font.SetPointSize(9);
	dc.SetFont(font);
	
	wxString mapInfo = wxString::Format("Floor: %d | Position: %d,%d", 
		floor, centerX, centerY);
	dc.DrawText(mapInfo, 10, 8);
	
	// Calculate visible area to draw
	int startX = centerX - (windowWidth / 2);
	int startY = centerY - ((windowHeight - headerHeight) / 2);
	int endX = startX + windowWidth;
	int endY = startY + (windowHeight - headerHeight);
	
	// Simple optimization - limit the rendering area to reasonable bounds
	int mapWidth = editor.map.getWidth();
	int mapHeight = editor.map.getHeight();
	
	startX = std::max(0, startX);
	startY = std::max(0, startY);
	endX = std::min(mapWidth, endX);
	endY = std::min(mapHeight, endY);
	
	// Batch drawing by color for better performance
	std::vector<std::vector<wxPoint>> colorPoints(256);
	
	// Iterate over visible area only
	for (int y = startY; y < endY; ++y) {
		for (int x = startX; x < endX; ++x) {
			if (x >= 0 && y >= 0 && x < mapWidth && y < mapHeight) {
				Tile* tile = editor.map.getTile(x, y, floor);
				if (tile) {
					uint8_t color = tile->getMiniMapColor();
					if (color) {
						int drawX = x - startX;
						int drawY = y - startY + headerHeight;
						colorPoints[color].push_back(wxPoint(drawX, drawY));
					}
				}
			}
		}
	}
	
	// Draw all points for each color at once
	for (int color = 0; color < 256; ++color) {
		if (!colorPoints[color].empty()) {
			dc.SetPen(*pens[color]);
			for (const wxPoint& pt : colorPoints[color]) {
				dc.DrawPoint(pt);
			}
		}
	}
	
	// Draw a marker for the center position
	dc.SetPen(wxPen(wxColour(255, 0, 0), 2));
	int centerDrawX = windowWidth / 2;
	int centerDrawY = (windowHeight - headerHeight) / 2 + headerHeight;
	dc.DrawLine(centerDrawX - 5, centerDrawY, centerDrawX + 5, centerDrawY);
	dc.DrawLine(centerDrawX, centerDrawY - 5, centerDrawX, centerDrawY + 5);
}

void MinimapWindow::OnMouseClick(wxMouseEvent& event) {
	if (!g_gui.IsEditorOpen())
		return;

	Editor& editor = *g_gui.GetCurrentEditor();
	MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
	
	int centerX, centerY;
	canvas->GetScreenCenter(&centerX, &centerY);
	
	int windowWidth = GetSize().GetWidth();
	int windowHeight = GetSize().GetHeight();
	int headerHeight = 30; // Match the header height from OnPaint
	
	// Calculate the map position clicked
	int clickX = event.GetX();
	int clickY = event.GetY() - headerHeight; // Adjust for header
	
	int mapX = centerX - (windowWidth / 2) + clickX;
	int mapY = centerY - ((windowHeight - headerHeight) / 2) + clickY;
	
	// Only process clicks below the header
	if (event.GetY() > headerHeight) {
		g_gui.SetScreenCenterPosition(Position(mapX, mapY, g_gui.GetCurrentFloor()));
		Refresh();
		g_gui.RefreshView();
	}
}

void MinimapWindow::OnKey(wxKeyEvent& event) {
	if (g_gui.GetCurrentTab() != nullptr) {
		g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}

MinimapWindow::BlockPtr MinimapWindow::getBlock(int x, int y) {
	std::lock_guard<std::mutex> lock(m_mutex);
	uint32_t index = getBlockIndex(x, y);
	
	auto it = m_blocks.find(index);
	if (it == m_blocks.end()) {
		auto block = std::make_shared<MinimapBlock>();
		m_blocks[index] = block;
		return block;
	}
	return it->second;
}

void MinimapWindow::updateBlock(BlockPtr block, int startX, int startY, int floor) {
	Editor& editor = *g_gui.GetCurrentEditor();
	
	// Always update if the block's floor doesn't match current floor
	if (!block->needsUpdate && block->floor != floor) {
		block->needsUpdate = true;
	}
	
	if (!block->needsUpdate) return;
	
	wxBitmap bitmap(BLOCK_SIZE, BLOCK_SIZE);
	wxMemoryDC dc(bitmap);
	dc.SetBackground(*wxBLACK_BRUSH);
	dc.Clear();
	
	// Store the floor this block was rendered for
	block->floor = floor;
	
	// Batch drawing by color like OTClient
	std::vector<std::vector<wxPoint>> colorPoints(256);
	
	for (int y = 0; y < BLOCK_SIZE; ++y) {
		for (int x = 0; x < BLOCK_SIZE; ++x) {
			int mapX = startX + x;
			int mapY = startY + y;
			
			Tile* tile = editor.map.getTile(mapX, mapY, floor);
			if (tile) {
				uint8_t color = tile->getMiniMapColor();
				if (color != 255) {  // Not transparent
					colorPoints[color].push_back(wxPoint(x, y));
				}
			}
		}
	}
	
	// Draw all points of same color at once
	for (int color = 0; color < 256; ++color) {
		if (!colorPoints[color].empty()) {
			dc.SetPen(*pens[color]);
			for (const wxPoint& pt : colorPoints[color]) {
				dc.DrawPoint(pt);
			}
		}
	}
	
	block->bitmap = bitmap;
	block->needsUpdate = false;
	block->wasSeen = true;
}

void MinimapWindow::ClearCache() {
	// Simply refresh the window
	Refresh();
}

void MinimapWindow::UpdateDrawnTiles(const PositionVector& positions) {
	// Just refresh the entire minimap - it's faster than checking which tiles need updating
	Refresh();
}

void MinimapWindow::PreCacheEntireMap() {
	// No longer needed - we'll just draw the visible area on demand
	return;
}

void MinimapWindow::InitialLoad() {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	// Clear any existing cache
	std::lock_guard<std::mutex> lock(m_mutex);
	m_blocks.clear();
	
	// Force an immediate refresh
	needs_update = true;
	Refresh();
}

