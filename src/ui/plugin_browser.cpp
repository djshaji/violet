#include "violet/plugin_browser.h"
#include "violet/plugin_manager.h"
#include "violet/utils.h"
#include <algorithm>
#include <windowsx.h>

namespace violet {

const wchar_t* PluginBrowser::CLASS_NAME = L"VioletPluginBrowser";

PluginBrowser::PluginBrowser()
    : hwnd_(nullptr)
    , hTreeView_(nullptr)
    , hSearchEdit_(nullptr)
    , hInstance_(nullptr)
    , pluginManager_(nullptr) {
}

PluginBrowser::~PluginBrowser() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
}

bool PluginBrowser::Create(HWND parent, HINSTANCE hInstance, int x, int y, int width, int height) {
    hInstance_ = hInstance;
    
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    
    // Check if already registered
    WNDCLASSEX existingClass;
    if (!GetClassInfoEx(hInstance, CLASS_NAME, &existingClass)) {
        if (!RegisterClassEx(&wc)) {
            return false;
        }
    }
    
    // Create the window
    hwnd_ = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Plugin Browser",
        WS_CHILD | WS_VISIBLE,
        x, y, width, height,
        parent,
        nullptr,
        hInstance,
        this
    );
    
    if (!hwnd_) {
        return false;
    }
    
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    
    return true;
}

void PluginBrowser::SetPluginManager(PluginManager* manager) {
    pluginManager_ = manager;
    if (hwnd_ && pluginManager_) {
        RefreshPluginList();
    }
}

void PluginBrowser::RefreshPluginList() {
    if (hTreeView_ && pluginManager_) {
        PopulateTreeView();
    }
}

void PluginBrowser::SetSearchFilter(const std::string& filter) {
    searchFilter_ = filter;
    FilterPlugins();
}

void PluginBrowser::ClearSearchFilter() {
    searchFilter_.clear();
    if (hSearchEdit_) {
        SetWindowText(hSearchEdit_, L"");
    }
    RefreshPluginList();
}

std::string PluginBrowser::GetSelectedPluginUri() const {
    if (!hTreeView_) {
        return "";
    }
    
    HTREEITEM hSelected = TreeView_GetSelection(hTreeView_);
    if (!hSelected) {
        return "";
    }
    
    // Get the item data
    TVITEM item = {};
    item.hItem = hSelected;
    item.mask = TVIF_PARAM;
    
    if (TreeView_GetItem(hTreeView_, &item) && item.lParam) {
        TreeItemData* data = reinterpret_cast<TreeItemData*>(item.lParam);
        if (!data->isCategory) {
            return data->uri;
        }
    }
    
    return "";
}

void PluginBrowser::Resize(int x, int y, int width, int height) {
    if (hwnd_) {
        SetWindowPos(hwnd_, nullptr, x, y, width, height, SWP_NOZORDER);
        OnSize(width, height);
    }
}

LRESULT CALLBACK PluginBrowser::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PluginBrowser* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<PluginBrowser*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->hwnd_ = hwnd;
    } else {
        pThis = reinterpret_cast<PluginBrowser*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT PluginBrowser::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        OnCreate();
        return 0;
        
    case WM_SIZE:
        OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;
        
    case WM_NOTIFY: {
        NMHDR* pnmhdr = reinterpret_cast<NMHDR*>(lParam);
        OnNotify(pnmhdr);
        return 0;
    }
    
    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == ID_SEARCH_EDIT) {
            // Search text changed
            wchar_t buffer[256];
            GetWindowText(hSearchEdit_, buffer, 256);
            searchFilter_ = utils::WStringToString(buffer);
            FilterPlugins();
        }
        return 0;
        
    default:
        return DefWindowProc(hwnd_, uMsg, wParam, lParam);
    }
}

void PluginBrowser::OnCreate() {
    CreateControls();
}

void PluginBrowser::OnSize(int width, int height) {
    if (!hSearchEdit_ || !hTreeView_) {
        return;
    }
    
    // Layout: Search box at top, tree view below
    const int searchHeight = 25;
    const int margin = 5;
    
    SetWindowPos(hSearchEdit_, nullptr, margin, margin, 
                 width - 2 * margin, searchHeight, SWP_NOZORDER);
    
    SetWindowPos(hTreeView_, nullptr, margin, searchHeight + 2 * margin,
                 width - 2 * margin, height - searchHeight - 3 * margin, SWP_NOZORDER);
}

void PluginBrowser::OnNotify(NMHDR* pnmhdr) {
    if (pnmhdr->idFrom == ID_TREEVIEW) {
        if (pnmhdr->code == NM_DBLCLK) {
            // Double-click on tree item - notify parent window
            std::string selectedUri = GetSelectedPluginUri();
            if (!selectedUri.empty()) {
                // Forward the notification to parent
                NMHDR nm;
                nm.hwndFrom = hTreeView_;
                nm.idFrom = ID_TREEVIEW;
                nm.code = NM_DBLCLK;
                SendMessage(GetParent(hwnd_), WM_NOTIFY, ID_TREEVIEW, (LPARAM)&nm);
            }
        }
    }
}

void PluginBrowser::CreateControls() {
    CreateSearchBox();
    CreateTreeView();
    
    // Initial layout
    RECT rect;
    GetClientRect(hwnd_, &rect);
    OnSize(rect.right - rect.left, rect.bottom - rect.top);
}

void PluginBrowser::CreateTreeView() {
    hTreeView_ = CreateWindowEx(
        0,
        WC_TREEVIEW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
        0, 0, 0, 0,
        hwnd_,
        (HMENU)ID_TREEVIEW,
        hInstance_,
        nullptr
    );
    
    if (hTreeView_ && pluginManager_) {
        PopulateTreeView();
    }
}

void PluginBrowser::CreateSearchBox() {
    hSearchEdit_ = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        0, 0, 0, 0,
        hwnd_,
        (HMENU)ID_SEARCH_EDIT,
        hInstance_,
        nullptr
    );
    
    // Set placeholder text (Windows Vista+)
    if (hSearchEdit_) {
        SendMessage(hSearchEdit_, EM_SETCUEBANNER, FALSE, (LPARAM)L"Search plugins...");
    }
}

void PluginBrowser::PopulateTreeView() {
    if (!hTreeView_ || !pluginManager_) {
        return;
    }
    
    // Clear existing items and data
    TreeView_DeleteAllItems(hTreeView_);
    treeItemData_.clear();
    
    // Get all plugins
    auto plugins = pluginManager_->GetAvailablePlugins();
    
    // Sort plugins by category, then name
    std::sort(plugins.begin(), plugins.end(), 
              [](const PluginInfo& a, const PluginInfo& b) {
                  if (a.category != b.category) {
                      return a.category < b.category;
                  }
                  return a.name < b.name;
              });
    
    // Add plugins to tree view, grouped by category
    for (const auto& plugin : plugins) {
        // Apply search filter if set
        if (!searchFilter_.empty()) {
            std::string nameLower = plugin.name;
            std::string authorLower = plugin.author;
            std::string filterLower = searchFilter_;
            
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            std::transform(authorLower.begin(), authorLower.end(), authorLower.begin(), ::tolower);
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
            
            if (nameLower.find(filterLower) == std::string::npos &&
                authorLower.find(filterLower) == std::string::npos) {
                continue; // Skip this plugin
            }
        }
        
        HTREEITEM categoryItem = FindOrCreateCategory(plugin.category);
        AddPluginToTree(plugin, categoryItem);
    }
    
    // Expand all categories by default
    ExpandAllCategories();
}

void PluginBrowser::AddPluginToTree(const PluginInfo& plugin, HTREEITEM categoryItem) {
    if (!hTreeView_ || !categoryItem) {
        return;
    }
    
    // Create display text: "Plugin Name (Author)"
    std::string displayText = plugin.name;
    if (!plugin.author.empty() && plugin.author != "Unknown") {
        displayText += " (" + plugin.author + ")";
    }
    
    // Add port count info
    displayText += " [";
    if (plugin.audioInputs > 0) {
        displayText += std::to_string(plugin.audioInputs) + "in";
    }
    if (plugin.audioOutputs > 0) {
        if (plugin.audioInputs > 0) displayText += "/";
        displayText += std::to_string(plugin.audioOutputs) + "out";
    }
    displayText += "]";
    
    // Create tree item data
    auto itemData = std::make_unique<TreeItemData>();
    itemData->uri = plugin.uri;
    itemData->isCategory = false;
    
    TVINSERTSTRUCTW tvins = {};
    tvins.hParent = categoryItem;
    tvins.hInsertAfter = TVI_LAST;
    tvins.item.mask = TVIF_TEXT | TVIF_PARAM;
    
    std::wstring wideText = utils::StringToWString(displayText);
    tvins.item.pszText = const_cast<LPWSTR>(wideText.c_str());
    tvins.item.lParam = reinterpret_cast<LPARAM>(itemData.get());
    
    HTREEITEM hItem = TreeView_InsertItem(hTreeView_, &tvins);
    
    if (hItem) {
        treeItemData_.push_back(std::move(itemData));
    }
}

HTREEITEM PluginBrowser::FindOrCreateCategory(const std::string& category) {
    if (!hTreeView_) {
        return nullptr;
    }
    
    // Search for existing category
    HTREEITEM hItem = TreeView_GetRoot(hTreeView_);
    while (hItem) {
        std::string itemText = GetItemText(hItem);
        if (itemText == category) {
            return hItem;
        }
        hItem = TreeView_GetNextSibling(hTreeView_, hItem);
    }
    
    // Create new category
    auto itemData = std::make_unique<TreeItemData>();
    itemData->uri = "";
    itemData->isCategory = true;
    
    TVINSERTSTRUCTW tvins = {};
    tvins.hParent = TVI_ROOT;
    tvins.hInsertAfter = TVI_SORT;
    tvins.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
    
    std::wstring wideCategory = utils::StringToWString(category);
    tvins.item.pszText = const_cast<LPWSTR>(wideCategory.c_str());
    tvins.item.lParam = reinterpret_cast<LPARAM>(itemData.get());
    tvins.item.state = TVIS_BOLD;
    tvins.item.stateMask = TVIS_BOLD;
    
    HTREEITEM hNewItem = TreeView_InsertItem(hTreeView_, &tvins);
    
    if (hNewItem) {
        treeItemData_.push_back(std::move(itemData));
    }
    
    return hNewItem;
}

void PluginBrowser::FilterPlugins() {
    // Simply repopulate the tree with the filter applied
    PopulateTreeView();
}

void PluginBrowser::ExpandAllCategories() {
    if (!hTreeView_) {
        return;
    }
    
    HTREEITEM hItem = TreeView_GetRoot(hTreeView_);
    while (hItem) {
        TreeView_Expand(hTreeView_, hItem, TVE_EXPAND);
        hItem = TreeView_GetNextSibling(hTreeView_, hItem);
    }
}

void PluginBrowser::CollapseAllCategories() {
    if (!hTreeView_) {
        return;
    }
    
    HTREEITEM hItem = TreeView_GetRoot(hTreeView_);
    while (hItem) {
        TreeView_Expand(hTreeView_, hItem, TVE_COLLAPSE);
        hItem = TreeView_GetNextSibling(hTreeView_, hItem);
    }
}

std::string PluginBrowser::GetItemText(HTREEITEM hItem) const {
    if (!hTreeView_ || !hItem) {
        return "";
    }
    
    wchar_t buffer[256];
    TVITEM item = {};
    item.hItem = hItem;
    item.mask = TVIF_TEXT;
    item.pszText = buffer;
    item.cchTextMax = 256;
    
    if (TreeView_GetItem(hTreeView_, &item)) {
        return utils::WStringToString(buffer);
    }
    
    return "";
}

} // namespace violet
