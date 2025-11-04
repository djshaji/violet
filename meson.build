project('violet', 'cpp',
  version : '1.0.0',
  default_options : [
    'cpp_std=c++17',
    'warning_level=3',
    'optimization=2'
  ]
)

# Compiler setup
cpp = meson.get_compiler('cpp')

# Dependencies
thread_dep = dependency('threads')

# Windows-specific dependencies
windows_deps = []
if host_machine.system() == 'windows'
  # Core Windows API libraries
  windows_deps += cpp.find_library('user32', required : true)
  windows_deps += cpp.find_library('kernel32', required : true)
  windows_deps += cpp.find_library('gdi32', required : true)
  windows_deps += cpp.find_library('comctl32', required : true)
  windows_deps += cpp.find_library('comdlg32', required : true)
  windows_deps += cpp.find_library('shell32', required : true)
  
  # Audio API libraries
  windows_deps += cpp.find_library('winmm', required : true)
  windows_deps += cpp.find_library('dsound', required : true)
  windows_deps += cpp.find_library('ole32', required : true)
  windows_deps += cpp.find_library('oleaut32', required : true)
endif

# LV2/LILV dependencies from win32 folder
win32_inc = include_directories('win32/include')
win32_lib_dir = join_paths(meson.current_source_dir(), 'win32/lib')

if host_machine.system() == 'windows'
  # Use the provided win32 dependencies
  lilv_dep = declare_dependency(
    include_directories : [win32_inc, include_directories('win32/include/lilv-0')],
    dependencies : cpp.find_library('lilv-0', dirs: [win32_lib_dir], required : true)
  )
  lv2_dep = declare_dependency(
    include_directories : [win32_inc, include_directories('win32/include/lv2')],
    # LV2 is header-only, no library needed
  )
  serd_dep = cpp.find_library('serd-0', dirs: [win32_lib_dir], required : true)
  sord_dep = cpp.find_library('sord-0', dirs: [win32_lib_dir], required : true)
  sratom_dep = cpp.find_library('sratom-0', dirs: [win32_lib_dir], required : true)
  zix_dep = cpp.find_library('zix-0', dirs: [win32_lib_dir], required : true)
else
  # Try system dependencies for native builds
  lilv_dep = dependency('lilv-0', required : false)
  lv2_dep = dependency('lv2', required : false)
  serd_dep = dependency('serd-0', required : false)
  sord_dep = dependency('sord-0', required : false)
  sratom_dep = dependency('sratom-0', required : false)
  zix_dep = dependency('zix-0', required : false)
endif

# JSON library for configuration - we'll implement a simple parser
json_dep = declare_dependency()

# Include directories
inc_dirs = include_directories('include')

# Source files
violet_sources = [
  'src/main.cpp',
  'src/ui/main_window.cpp',
  'src/core/config_manager.cpp',
  'src/core/utils.cpp',
  'src/platform/windows_api.cpp',
]

# Create the executable
all_deps = [thread_dep, json_dep, lilv_dep, lv2_dep] + windows_deps
if host_machine.system() == 'windows'
  all_deps += [serd_dep, sord_dep, sratom_dep, zix_dep]
endif

violet_exe = executable('violet',
  violet_sources,
  include_directories : inc_dirs,
  dependencies : all_deps,
  install : true,
  win_subsystem : 'windows'  # GUI application, not console
)

# Optional: Create a console version for debugging
if get_option('debug')
  violet_console = executable('violet-console',
    violet_sources,
    include_directories : inc_dirs,
    dependencies : all_deps,
    win_subsystem : 'console'
  )
endif