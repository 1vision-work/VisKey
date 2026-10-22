# -*- coding: utf-8 -*-
"""Default VisKey Linux version (keep in sync with project(VERSION) in CMakeLists.txt).

Release builds take the version from the git tag instead: scripts/build_linux_deb.sh rewrites
VERSION in the packaged copy of this file. scripts/viskey_version.sh reads the VERSION line below
for untagged builds, so keep it as a plain string literal.
"""
VERSION = "1.0.0"
