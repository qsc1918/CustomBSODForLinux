project('bsod', 'c',
  version: '1.0.1',
  license: 'MIT',
  meson_version: '>=0.61.0',
  default_options: [
    'c_std=gnu11',
    'warning_level=2',
    'buildtype=release',
    'b_lto=true',
  ])

cc = meson.get_compiler('c')

# libdrm 依赖：优先使用 pkg-config，找不到则回退到 -ldrm
drm_dep = dependency('libdrm', required: false)
if not drm_dep.found()
  drm_dep = cc.find_library('drm', required: true)
endif

# 文本渲染：FreeType
freetype_dep = dependency('freetype2', required: true)

# 系统字体发现（可选）：有 fontconfig 按族名找字体，否则回退目录扫描
fontconfig_dep = dependency('fontconfig', required: false)
conf = configuration_data()
conf.set('HAVE_FONTCONFIG', fontconfig_dep.found())
configure_file(output: 'config.h', configuration: conf)

# 系统日志监控：journald（sd-journal）
systemd_dep = dependency('libsystemd', required: true)

# 彩虹动画用到 fmod/fabs（某些 libc 需要显式链接 libm）
m_dep = cc.find_library('m', required: true)

# 所有源码与头文件均位于 src/ 目录
src_inc = include_directories('src')

sources = files(
  'src/main.c',
  'src/bsod.c',
  'src/bsod_theme.c',
  'src/drm_util.c',
  'src/vt_util.c',
  'src/render.c',
  'src/font_util.c',
  'src/qr.c',
  'src/qrcodegen.c',
  'src/monitor.c',
  'src/i18n.c',
)

deps = [drm_dep, freetype_dep, systemd_dep, m_dep]
if fontconfig_dep.found()
  deps += fontconfig_dep
endif

executable('bsod',
  sources,
  include_directories: src_inc,
  dependencies: deps,
  install: true,
)
