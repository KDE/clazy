import os

class Test:
    def __init__(self, check):
        self.filename = ""
        self.minimum_qt_version = 500
        self.maximum_qt_version = 69999
        self.minimum_clang_version = 380
        self.minimum_clang_version_for_fixits = 380
        self.compare_everything = False
        self.check = check
        self.expects_failure = False
        self.skip_qtnamespaced = False
        self.qt_major_versions = [5, 6]
        self.env = os.environ
        self.checks = []
        self.flags = ""
        self.must_fail = False
        self.blacklist_platforms = []
        self.only_qt = False
        self.qt_developer = False
        self.header_filter = ""
        self.ignore_dirs = ""
        self.has_fixits = False
        self.should_run_fixits_test = False
        self.should_run_on_32bit = True
        self.cppStandards = ["c++14", "c++17"]
        self.extra_definitions = False
        self.qt_modules_includes = False
        self.fixed_file_base = None


    def relativeFilename(self):
        # example: "auto-unexpected-qstringbuilder/main.cpp"
        return self.check.name + "/" + self.filename

    def yamlFilename(self, is_standalone):
        # The name of the yaml file with fixits
        # example: "auto-unexpected-qstringbuilder/main.cpp.clazy.yaml"
        if is_standalone:
            return self.relativeFilename() + ".clazy-standalone.yaml"
        else:
            return self.relativeFilename() + ".clazy.yaml"

    def fixedFilename(self, is_standalone):
        if is_standalone:
            return self.relativeFilename() + ".clazy-standalone.fixed"
        else:
            return self.relativeFilename() + ".clazy.fixed"

    def expectedFixedFilename(self):
        return self.relativeFilename() + ".fixed.expected"

    def isScript(self):
        return self.filename.endswith(".sh")

    def setQtMajorVersions(self, major_versions):
        self.qt_major_versions = major_versions
        if 4 in major_versions:
            if self.minimum_qt_version >= 500:
                self.minimum_qt_version = 400

    def setEnv(self, e):
        self.env = os.environ.copy()
        for key in e:
            if type(key) is bytes:
                key = key.decode('utf-8')

            self.env[key] = e[key]

    def printableName(self, cppStandard, qt_major_version, is_standalone, is_tidy, is_fixits):
        name = self.check.name
        if len(self.check.tests) > 1:
            name += "/" + self.filename
        if len(cppStandard) > 0:
            name += " (" + cppStandard + ")"
        if qt_major_version > 0:
            name += " (Qt " + str(qt_major_version) + ")"
        if is_fixits and is_standalone:
            name += " (standalone, fixits)"
        elif is_standalone:
            name += " (standalone)"
        elif is_tidy:
            name += " (clang-tidy)"
        elif is_fixits:
            name += " (plugin, fixits)"
        else:
            name += " (plugin)"
        return name

    def removeYamlFiles(self):
        for f in [self.yamlFilename(False), self.yamlFilename(True)]:
            if os.path.exists(f):
                os.remove(f)
