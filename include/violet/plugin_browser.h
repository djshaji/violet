#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <memory>

namespace violet {

class PluginManager;
struct PluginInfo;

// Plugin browser control that displays available LV2 plugins
class PluginBrowser {
public:
    PluginBrowser();
    ~PluginBrowser();

    // Create the plugin browser window
    bool Create(HWND parent, HINSTANCE hInstance, int x, int y, int width, int height);
    
    // Set the plugin manager to use for listing plugins
    void SetPluginManager(PluginManager* manager);
    
    // Refresh the plugin list
    void RefreshPluginList();
    
    // Get window handle
    HWND GetHandle() const { return hwnd_; }
    
    // Search functionality
    void SetSearchFilter(const std::string& filter);
    void ClearSearchFilter();
    
    // Get selected plugin URI
    std::string GetSelectedPluginUri() const;
    
    // Resize the browser
    void Resize(int x, int y, int width, int height);

private:
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Message handlers
    void OnCreate();
    void OnSize(int width, int height);
    void OnNotify(NMHDR* pnmhdr);
    
    // UI creation
    void CreateControls();
    void CreateTreeView();
    void CreateSearchBox();
    
    // Plugin list management
    void PopulateTreeView();
    void AddPluginToTree(const PluginInfo& plugin, HTREEITEM categoryItem);
    HTREEITEM FindOrCreateCategory(const std::string& category);
    void FilterPlugins();
    
    // Tree view helpers
    void ExpandAllCategories();
    void CollapseAllCategories();
    std::string GetItemText(HTREEITEM hItem) const;
    
    // Member variables
    HWND hwnd_;
    HWND hTreeView_;
    HWND hSearchEdit_;
    HINSTANCE hInstance_;
    
    PluginManager* pluginManager_;
    std::string searchFilter_;
    
    // Tree item data structure
    struct TreeItemData {
        std::string uri;
        bool isCategory;
    };
    
    std::vector<std::unique_ptr<TreeItemData>> treeItemData_;
    
    // Window class
    static const wchar_t* CLASS_NAME;
    
    // Control IDs
    static const int ID_TREEVIEW = 1001;
    static const int ID_SEARCH_EDIT = 1002;
};

} // namespace violet
