# Plugin Browser Implementation

## Overview
The Plugin Browser is a new UI component that displays all available LV2 plugins in a hierarchical tree view, organized by category. This component is now integrated into the main window.

## Features Implemented

### 1. Tree View Display
- **Hierarchical Organization**: Plugins are grouped by category (Effect, Generator, Analyzer, Utility, etc.)
- **Plugin Information**: Each plugin shows:
  - Plugin name
  - Author name (in parentheses)
  - Audio port configuration [inputs/outputs]
  - Example: "Delay Plugin (Author Name) [2in/2out]"

### 2. Search Functionality
- **Real-time Search**: Type in the search box to filter plugins
- **Multi-field Search**: Searches both plugin name and author
- **Case-insensitive**: Search is not case-sensitive
- **Placeholder Text**: Search box shows "Search plugins..." hint

### 3. User Interface
- **Left Panel**: Plugin browser occupies 250px on the left side of the window
- **Resizable**: Automatically adjusts height when window is resized
- **Category Expansion**: All categories are expanded by default for easy browsing
- **Selection**: Click to select a plugin (highlighted in tree view)
- **Double-click Ready**: Infrastructure in place for double-click to load plugins

## Technical Details

### Files Created
- `include/violet/plugin_browser.h` - Header file with class definition
- `src/ui/plugin_browser.cpp` - Implementation file

### Integration
- Integrated into `MainWindow` class
- Connected to `PluginManager` for plugin enumeration
- Automatically populated with available LV2 plugins on startup

### API

#### Key Methods
```cpp
// Create the browser
bool Create(HWND parent, HINSTANCE hInstance, int x, int y, int width, int height);

// Set plugin manager
void SetPluginManager(PluginManager* manager);

// Refresh plugin list
void RefreshPluginList();

// Search
void SetSearchFilter(const std::string& filter);
void ClearSearchFilter();

// Get selected plugin
std::string GetSelectedPluginUri() const;
```

## Usage

The Plugin Browser is automatically created when the main window is initialized:

1. **Initialization**: Plugin manager scans for available LV2 plugins
2. **Display**: Plugins are automatically populated in the tree view
3. **Search**: User can type in search box to filter plugins
4. **Selection**: User can click on a plugin to select it
5. **Integration Ready**: Selected plugin URI can be retrieved for loading

## Next Steps

### Planned Enhancements
1. **Drag-and-Drop**: Drag plugins from browser to active plugins panel
2. **Context Menu**: Right-click menu for plugin actions
3. **Plugin Details**: Show detailed plugin information on selection
4. **Custom Categories**: Allow user to create custom category filters
5. **Favorites**: Mark plugins as favorites for quick access
6. **Recent**: Show recently used plugins
7. **Plugin Loading**: Double-click to load plugin into processing chain

## Example Use Case

```cpp
// In MainWindow
if (pluginBrowser_) {
    // Get selected plugin
    std::string pluginUri = pluginBrowser_->GetSelectedPluginUri();
    
    if (!pluginUri.empty()) {
        // Load plugin into processing chain
        if (processingChain_) {
            uint32_t nodeId = processingChain_->AddPlugin(pluginUri);
            // Display plugin in active plugins panel
        }
    }
}
```

## Screenshot Description

The Plugin Browser appears on the left side of the window:

```
+-------------------+----------------------------------+
| Violet - LV2 Plugin Host                           |
+-------------------+----------------------------------+
| File  Audio  Help |                                 |
+-------------------+----------------------------------+
| [Search plugins...] |                                |
| ├─ Effect          |                                |
| │  ├─ Delay (Auth)|                                |
| │  ├─ Reverb (...)|                                |
| │  └─ ...         |                                |
| ├─ Generator       |      Active Plugins Panel     |
| │  └─ ...         |        (To Be Implemented)     |
| ├─ Analyzer        |                                |
| └─ ...             |                                |
+-------------------+----------------------------------+
| Ready | Audio: Stopped | CPU: 0% | Plugins: 42      |
+------------------------------------------------------+
```

## Build Information

The plugin browser is automatically built as part of the Violet project:

```bash
meson setup build --cross-file cross-mingw64.txt
ninja -C build
```

The implementation successfully compiles with MinGW-w64 for Windows target.
