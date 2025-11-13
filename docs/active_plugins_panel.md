# Active Plugins Panel Implementation

## Overview
The Active Plugins Panel presents the live audio processing chain as a vertical stack of expandable plugin sections. Each plugin exposes a grid of inline parameter controls so users can tweak sounds without opening a separate window. The panel keeps UI state in sync with the audio engine while protecting users from parameter jumps that can occur during rapid automation updates.

## Feature Summary

### Plugin Header
- Displays plugin name, URI tooltips, selection highlight, and hover feedback
- Provides **Bypass** and **Remove** push buttons aligned to the right edge
- Includes a global **Remove All Plugins** button pinned to the top of the panel

### Parameter Controls
- Parameters render as **round knob controls** arranged in a three-column grid
- Knobs show name labels above and numeric value readouts below
- Inline **− / +** nudge buttons sit on either side of each knob for precise adjustments
- Values update immediately while preserving pending user edits even if the backend lags

### Layout & Scrolling
- Grid constants (`PARAMETERS_PER_ROW`, `COLUMN_WIDTH`, `PARAM_HEIGHT`, `LABEL_HEIGHT`) keep knob geometry consistent
- Row height supports taller circular knobs (72 px default) with generous hit targets
- Panel supports smooth vertical scrolling with a Windows scrollbar when content exceeds the viewport

### Interaction & Feedback
- Selection highlight follows single-clicks; double-click headers toggle parameter expansion
- Context menu (right-click) exposes remove/bypass/move actions for future extensions
- Hover tracking updates plugin headers to aid discovery
- Timer-driven refresh (100 ms) keeps knobs in sync while skipping user-controlled widgets during drags

## UI Layout

```
┌───────────────────────────────────────────────┐
│ Remove All Plugins [button]                   │
├───────────────────────────────────────────────┤
│ Plugin Name                             [B] [X]
│ ┌─────────────┬─────────────┬─────────────┐
│ │ Label       │ Label       │ Label       │
│ │   ◯ knob    │   ◯ knob    │   ◯ knob    │
│ │ [-]     [+] │ [-]     [+] │ [-]     [+] │
│ │   value     │   value     │   value     │
│ └─────────────┴─────────────┴─────────────┘
│ (additional rows as needed...)              │
└───────────────────────────────────────────────┘
```

## Parameter Control Details
- **KnobControl** windows receive circular regions (`CreateEllipticRgn`) so only the dial is interactive
- Background fill uses `FillRgn` to maintain a round footprint that respects theme colors
- Knob range maps across 270° sweep (135° start, 405° end) with pointer and center dot styling
- Pending value cache (`pendingValues_`) avoids snap-backs while the audio thread applies changes
- Interaction timers (`TIMER_ID_INTERACTION`) prevent backend refresh from fighting active drags
- Numeric display precision adapts to integer vs floating-point parameters

## Event Flow
1. **Creation**: `CreateParameterControls` queries `ProcessingNode` metadata and instantiates label/value statics, buttons, and knobs per parameter
2. **User Input**: Button/knob messages map through `sliderToParam_` into `AudioProcessingChain::SetParameter`
3. **Sync Loop**: WM_TIMER drives `UpdateParameterControls`, reconciling backend values with pending cache
4. **Layout**: `RecalculateLayout` repositions headers and knobs on resize or scroll, using cached column/row indices for stability

## Integration Points
- `MainWindow`: owns and resizes the panel alongside the plugin browser
- `AudioProcessingChain`: supplies parameter data, node IDs, and current values
- `PluginManager`: fills `ParameterInfo` structs with min/max/default metadata
- `KnobControl`: dedicated window class defined in `src/ui/knob_control.cpp`

## Manual Testing Checklist
1. Load multiple plugins and confirm knobs appear three per row
2. Drag knobs; verify values hold steady when new audio updates arrive
3. Use +/- buttons to step values; confirm pending cache resolves once audio catches up
4. Collapse/expand plugins and ensure controls destroy/recreate cleanly without leaks
5. Scroll through long chains; check headers/buttons track positions correctly
6. Remove plugins individually and en-masse; ensure orphaned controls are destroyed

## Future Enhancements
- Keyboard focus and accessibility for knobs and inline buttons
- Drag-and-drop reordering of plugins within the panel
- Per-plugin meters positioned beneath the knob grid
- Optional compact mode for laptops with limited vertical space

---

**Status**: ✅ Inline knob grid shipped & synced with audio engine (November 2025)  
**Next**: Keyboard accessibility and chain reordering
