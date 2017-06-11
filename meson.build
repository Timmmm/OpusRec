project('OpusRec', 'cpp', default_options: ['c_std=gnu11', 'cpp_std=c++11'])
# gnu11 is needed for ALSA, which is a bit broken. See https://stackoverflow.com/questions/32672333/including-alsa-asoundlib-h-and-sys-time-h-results-in-multiple-definition-co

docopt = subproject('docopt').get_variable('docopt')
libsoundio = subproject('libsoundio').get_variable('libsoundio')
libwebm = subproject('libwebm').get_variable('libwebm')
opus = subproject('opus').get_variable('opus')

# Main program.
opusrec_src = [
	'main.cpp',
	'CtrlC.cpp',
	'CtrlC.h',
	'RingBuffer.h',
]

executable('opusrec', opusrec_src, dependencies: [docopt, libsoundio, libwebm, opus])
