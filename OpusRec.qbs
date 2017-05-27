import qbs

CppApplication {
	name: "OpusRec"
	files: "main.cpp"

	Properties {
		condition: qbs.targetOS.contains("windows")
		cpp.includePaths: ["libsoundio-1.1.0"]
	}
}
