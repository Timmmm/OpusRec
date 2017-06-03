project('libsoundio', 'c', default_options: ['c_std=gnu11'])
# gnu11 is required - see https://stackoverflow.com/questions/32672333/including-alsa-asoundlib-h-and-sys-time-h-results-in-multiple-definition-co

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

libs = []

alsa_src = [
	'libsoundio/src/alsa.h',
	'libsoundio/src/alsa.c',
]

alsa_libs = ['-lasound']

coreaudio_src = [
	'libsoundio/src/coreaudio.h',
	'libsoundio/src/coreaudio.c',
]

coreaudio_libs = ['-lCoreAudio']

jack_src = [
	'libsoundio/src/jack.h',
	'libsoundio/src/jack.c',
]

jack_libs = ['-ljack']

pulseaudio_src = [
	'libsoundio/src/pulseaudio.h',
	'libsoundio/src/pulseaudio.c',
]

pulseaudio_libs = ['-lpulse']

wasapi_src = [
	'libsoundio/src/wasapi.c',
	'libsoundio/src/wasapi.h',
]

wasapi_libs = [] # None required.

conf_data = configuration_data()
conf_data.set('LIBSOUNDIO_VERSION_MAJOR', '1')
conf_data.set('LIBSOUNDIO_VERSION_MINOR', '1')
conf_data.set('LIBSOUNDIO_VERSION_PATCH', '0')
conf_data.set('LIBSOUNDIO_VERSION', '1.1.0')

if get_option('enable_alsa') == 'true' or (get_option('enable_alsa') == 'auto' and meson.get_compiler('cpp').has_header('alsa/asoundlib.h'))
	
	conf_data.set('SOUNDIO_HAVE_ALSA', true)
	src += alsa_src
	libs += alsa_libs
endif

if get_option('enable_coreaudio') == 'true' or (get_option('enable_coreaudio') == 'auto' and meson.get_compiler('cpp').has_header('CoreAudio/CoreAudio.h') and meson.get_compiler('cpp').has_header('AudioUnit/AudioUnit.h'))
	
	conf_data.set('SOUNDIO_HAVE_COREAUDIO', true)
	src += coreaudio_src
	libs += coreaudio_libs
endif

if get_option('enable_jack') == 'true' or (get_option('enable_jack') == 'auto' and meson.get_compiler('cpp').has_header('jack/jack.h'))
	
	conf_data.set('SOUNDIO_HAVE_JACK', true)
	src += jack_src
	libs += jack_libs
endif

if get_option('enable_pulseaudio') == 'true' or (get_option('enable_pulseaudio') == 'auto' and meson.get_compiler('cpp').has_header('pulse/pulseaudio.h'))
	
	conf_data.set('SOUNDIO_HAVE_PULSEAUDIO', true)
	src += pulseaudio_src
	libs += pulseaudio_libs
endif

if get_option('enable_wasapi') == 'true' or (get_option('enable_wasapi') == 'auto' and meson.get_compiler('cpp').has_header('Audioclient.h'))
	
	conf_data.set('SOUNDIO_HAVE_WASAPI', true)
	src += wasapi_src
	libs += wasapi_libs
endif

configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)

libsoundio_inc = include_directories('libsoundio')
libsoundio_lib = static_library('libsoundio', src, include_directories: libsoundio_inc, c_args: ['-DSOUNDIO_STATIC_LIBRARY'])

libsoundio = declare_dependency(
	link_with: libsoundio_lib,
	include_directories: libsoundio_inc,
	dependencies: dependency('threads'),
	compile_args: ['-DSOUNDIO_STATIC_LIBRARY'],
	link_args: libs
)


