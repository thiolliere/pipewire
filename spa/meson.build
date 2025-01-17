#project('spa', 'c')

#cc = meson.get_compiler('c')
#dl_lib = cc.find_library('dl', required : false)
#pthread_lib = dependencies('threads')
#mathlib = cc.find_library('m', required : false)

spa_inc = include_directories('include')

subdir('include')

if not get_option('spa-plugins').disabled()
  udevrulesdir = get_option('udevrulesdir')
  if udevrulesdir == ''
    # absolute path, otherwise meson prepends the prefix
    udevrulesdir = '/lib/udev/rules.d'
  endif

  # common dependencies
  libudev_dep = dependency('libudev', required: get_option('alsa').enabled() or get_option('v4l2').enabled() or get_option('udev').enabled())

  # plugin-specific dependencies
  alsa_dep = dependency('alsa', required: get_option('alsa'))
  bluez_dep = dependency('bluez', version : '>= 4.101', required: get_option('bluez5'))
  sbc_dep = dependency('sbc', required: get_option('bluez5'))
  ldac_dep = dependency('ldacBT-enc', required : false)
  ldac_abr_dep = dependency('ldacBT-abr', required : false)
  aptx_dep = dependency('libopenaptx', required : false)
  fdk_aac_dep = dependency('fdk-aac', required : false)
  avcodec_dep = dependency('libavcodec', required: get_option('ffmpeg'))
  avformat_dep = dependency('libavformat', required: get_option('ffmpeg'))
  jack_dep = dependency('jack', version : '>= 1.9.10', required: get_option('jack'))
  vulkan_dep = dependency('vulkan', version : '>= 1.1.69', required: get_option('vulkan'))
  libcamera_dep = dependency('camera', required: get_option('libcamera'))

  subdir('plugins')
endif

subdir('tools')
subdir('tests')
if not get_option('examples').disabled()
  subdir('examples')
endif

pkgconfig.generate(filebase : 'lib@0@'.format(spa_name),
  name : 'libspa',
  subdirs : spa_name,
  description : 'Simple Plugin API',
  version : spaversion,
  extra_cflags : '-D_REENTRANT')
