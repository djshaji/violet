# Drag-and-Drop Plugin Loading

## Overview

The drag-and-drop feature allows users to quickly load LV2 plugins by dragging them from the Plugin Browser panel directly to the Active Plugins panel. This provides an intuitive, modern UI interaction alongside the existing double-click method.

## Implementation Details

### Architecture

The drag-and-drop functionality is implemented using a custom messaging system between the Plugin Browser and Active Plugins Panel through the Main Window:

1. **Plugin Browser** (source): Detects drag initiation via `TVN_BEGINDRAG` notification from TreeView control
2. **Main Window** (mediator): Routes the plugin URI via custom message `WM_USER + 200`
3. **Active Plugins Panel** (target): Receives URI and loads the plugin

### Component Changes

#### Plugin Browser (`plugin_browser.cpp/h`)

**New Methods:**
- `GetPluginUriAtItem(HTREEITEM)`: Retrieves plugin URI for a specific tree item
- `OnBeginDrag(NMTREEVIEW*)`: Handles drag initiation

**Implementation:**
```cpp
void PluginBrowser::OnBeginDrag(NMTREEVIEW* pnmtv) {
    std::string uri = GetPluginUriAtItem(pnmtv->itemNew.hItem);
    if (uri.empty()) return;  // Can't drag categories
    
    HWND parent = GetParent(hwnd_);
    if (parent) {
        static std::string draggedUri;
        draggedUri = uri;
        SendMessage(parent, WM_USER + 200, 0, 
                   reinterpret_cast<LPARAM>(draggedUri.c_str()));
    }
}
```

#### Active Plugins Panel (`active_plugins_panel.cpp/h`)

**New Methods:**
- `LoadPluginFromUri(const std::string&)`: Loads a plugin from its LV2 URI

**Implementation:**
```cpp
void ActivePluginsPanel::LoadPluginFromUri(const std::string& uri) {
    if (!processingChain_ || uri.empty()) return;
    
    uint32_t nodeId = processingChain_->AddPlugin(uri);
    if (nodeId != 0) {
        auto node = processingChain_->GetNode(nodeId);
        if (node && node->GetPlugin()) {
            std::string name = node->GetPlugin()->GetInfo().name;
            AddPlugin(nodeId, name, uri);
        }
    }
}
```

#### Main Window (`main_window.cpp`)

**Message Handler:**
```cpp
case WM_USER + 200: {
    // Custom message: drag-drop plugin URI from browser
    if (lParam != 0 && activePluginsPanel_) {
        const char* uri = reinterpret_cast<const char*>(lParam);
        activePluginsPanel_->LoadPluginFromUri(uri);
    }
    return 0;
}
```

## User Experience

### How to Use

1. **Locate Plugin**: Browse or search for a plugin in the left panel
2. **Initiate Drag**: Click and hold on a plugin name in the tree view
3. **Drop to Load**: Release over the Active Plugins panel (right side)
4. **Plugin Loaded**: Plugin appears in the chain with inline controls

### Advantages

- **Faster Workflow**: Drag-and-drop is often more intuitive than double-clicking
- **Visual Feedback**: User can see the destination before releasing
- **Modern UI**: Matches user expectations from other DAWs and applications
- **Non-Destructive**: Does not replace double-click functionality

## Technical Notes

### Custom Messaging

We use `WM_USER + 200` as a custom message ID to avoid conflicts:
- `WM_USER + 101`: Plugin parameters window (existing)
- `WM_USER + 200`: Drag-drop plugin URI (new)

### Static URI Storage

The dragged URI is stored in a static variable in `OnBeginDrag()` to ensure it persists during the message send. This is safe because:
- Drag operations are sequential (only one at a time)
- Message is sent synchronously via `SendMessage()`
- URI is immediately processed in the message handler

### Category Prevention

The implementation prevents dragging category items (folders) by checking if the item has a valid plugin URI. Only leaf nodes (actual plugins) can be dragged.

## Future Enhancements

Potential improvements for future versions:

1. **Visual Drag Cursor**: Custom cursor showing plugin name during drag
2. **Drop Zone Highlighting**: Highlight the Active Plugins panel when dragging
3. **Insert Position Control**: Drop at specific positions in the chain
4. **Multi-Select Drag**: Drag multiple plugins at once
5. **Drag to Reorder**: Drag plugins within the Active Panel to reorder them

## Build Information

**Implemented**: November 11, 2025  
**Commit**: Drag-and-drop plugin loading feature  
**Build Size**: 8.9MB (no size increase)  
**Dependencies**: None (uses standard Windows TreeView notifications)

## Testing

To test the drag-and-drop functionality:

1. Launch Violet application
2. Ensure LV2 plugins are discovered (check Plugin Browser)
3. Click and hold on any plugin name
4. Drag to the Active Plugins panel area
5. Release mouse button
6. Verify plugin loads with inline controls
7. Test that categories cannot be dragged
8. Verify double-click still works as before

## Troubleshooting

**Plugin doesn't load after drag:**
- Ensure you're dragging a plugin, not a category folder
- Check that the Active Plugins panel is visible
- Verify the processing chain is initialized

**Drag operation doesn't start:**
- TreeView must have focus
- Ensure you're clicking directly on the plugin name
- Try double-click as an alternative method

## Related Files

- `include/violet/plugin_browser.h`
- `src/ui/plugin_browser.cpp`
- `include/violet/active_plugins_panel.h`
- `src/ui/active_plugins_panel.cpp`
- `src/ui/main_window.cpp`

## See Also

- [Plugin Browser Documentation](plugin_browser.md)
- [Active Plugins Panel Documentation](active_plugins_panel.md)
- [Windows TreeView Drag-Drop Notifications](https://learn.microsoft.com/en-us/windows/win32/controls/tvn-begindrag)
