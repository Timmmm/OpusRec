project('OpusRec', 'cpp', default_options: ['c_std=c11', 'cpp_std=c++14'])


docopt = subproject('docopt').get_variable('docopt')
libsoundio = subproject('libsoundio').get_variable('libsoundio')
libwebm = subproject('libwebm').get_variable('libwebm')

# Main program.
#opusrec_src = [
#    'main.cpp',
#	'config.h',
#]

#executable('opusrec', opusrec_src, dependencies: [docopt, libsoundio, libwebm, opus])

executable('build_test', ['build_test.cpp'], dependencies: [docopt, libsoundio, libwebm])
