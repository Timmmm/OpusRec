project('docopt', 'cpp', default_options: ['cpp_std=c++14'])

src = [
	'docopt/docopt.cpp',
	'docopt/docopt.h',
	'docopt/docopt_private.h',
	'docopt/docopt_util.h',
	'docopt/docopt_value.h',
]

docopt_inc = include_directories('docopt')
docopt_lib = static_library('docopt', src)

docopt = declare_dependency(link_with: docopt_lib, include_directories: docopt_inc)
