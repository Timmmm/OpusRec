import qbs

CppApplication {
	name: "OpusRec"
	files: [
		"docopt/docopt.cpp",
		"docopt/docopt.h",
		"docopt/docopt_private.h",
		"docopt/docopt_util.h",
		"docopt/docopt_value.h",

		"libsoundio-1.1.0/soundio/*.h",

		"main.cpp",

	]

	cpp.cxxLanguageVersion: "c++14"

	cpp.includePaths: [
		"docopt",
		"opus",
		"libsoundio-1.1.0",
	]

	cpp.libraryPaths: [
		"libsoundio-1.1.0/i686",
	]

	cpp.staticLibraries: [
		"soundio",
	]

	cpp.defines: [
		"SOUNDIO_STATIC_LIBRARY",
	]
}
