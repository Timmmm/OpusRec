import qbs

CppApplication {
	name: "OpusRec"
	files: [
		"config.h",
		"docopt/docopt.cpp",
		"docopt/docopt.h",
		"docopt/docopt_private.h",
		"docopt/docopt_util.h",
		"docopt/docopt_value.h",
		"libsoundio/soundio/endian.h",
		"libsoundio/soundio/soundio.h",
		"libsoundio/src/atomics.h",
		"libsoundio/src/channel_layout.c",
		"libsoundio/src/dummy.c",
		"libsoundio/src/dummy.h",
		"libsoundio/src/list.h",
		"libsoundio/src/os.c",
		"libsoundio/src/os.h",
		"libsoundio/src/ring_buffer.c",
		"libsoundio/src/ring_buffer.h",
		"libsoundio/src/soundio.c",
		"libsoundio/src/soundio_internal.h",
		"libsoundio/src/soundio_private.h",
		"libsoundio/src/util.c",
		"libsoundio/src/util.h",
		"libsoundio/src/wasapi.c",
		"libsoundio/src/wasapi.h",
		"main.cpp",
	]

	cpp.cxxLanguageVersion: "c++14"

	cpp.includePaths: [
		".",
		"docopt",
		"opus",
		"libsoundio",
	]

	cpp.defines: [
		"SOUNDIO_STATIC_LIBRARY",
	]

	cpp.staticLibraries: [
		"ole32",
	]
}
