project('libsoundio', 'c', default_options: ['c_std=c11'])

src = [
	'libsoundio/soundio/endian.h',
	'libsoundio/soundio/soundio.h',
	
	'libsoundio/src/atomics.h',
	'libsoundio/src/channel_layout.c',
	'libsoundio/src/dummy.c',
	'libsoundio/src/dummy.h',
	'libsoundio/src/list.h',
	'libsoundio/src/os.c',
	'libsoundio/src/os.h',
	'libsoundio/src/ring_buffer.c',
	'libsoundio/src/ring_buffer.h',
	'libsoundio/src/soundio.c',
	'libsoundio/src/soundio_internal.h',
	'libsoundio/src/soundio_private.h',
	'libsoundio/src/util.c',
	'libsoundio/src/util.h',
]

alsa_src = [
	'libsoundio/src/alsa.h',
	'libsoundio/src/alsa.c',
]

coreaudio_src = [
	'libsoundio/src/coreaudio.h',
	'libsoundio/src/coreaudio.c',
]

jack_src = [
	'libsoundio/src/jack.h',
	'libsoundio/src/jack.c',
]

pulseaudio_src = [
	'libsoundio/src/pulseaudio.h',
	'libsoundio/src/pulseaudio.c',
]

wasapi_src = [
	'libsoundio/src/wasapi.c',
	'libsoundio/src/wasapi.h',
]

conf_data = configuration_data()
conf_data.set('LIBSOUNDIO_VERSION_MAJOR', '1')
conf_data.set('LIBSOUNDIO_VERSION_MINOR', '1')
conf_data.set('LIBSOUNDIO_VERSION_PATCH', '0')
conf_data.set('LIBSOUNDIO_VERSION', '1.1.0')

if get_option('enable_alsa') == 'true'
	conf_data.set('HAVE_ALSA', true)
	src += alsa_src
endif

if get_option('enable_coreaudio') == 'true'
	conf_data.set('HAVE_COREAUDIO', true)
	src += coreaudio_src
endif

if get_option('enable_jack') == 'true'
	conf_data.set('HAVE_JACK', true)
	src += jack_src
endif

if get_option('enable_pulseaudio') == 'true'
	conf_data.set('HAVE_PULSEAUDIO', true)
	src += pulseaudio_src
endif

if get_option('enable_wasapi') == 'true'
	conf_data.set('HAVE_WASAPI', true)
	src += wasapi_src
endif

configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)

libsoundio_inc = include_directories('libsoundio')
libsoundio_lib = library('libsoundio', src, include_directories: libsoundio_inc)

libsoundio = declare_dependency(link_with: libsoundio_lib, include_directories: libsoundio_inc, dependencies: dependency('threads'))


