## Build notes

Generally, build exactly how you would build another commonlibSSE-ng SKSE plugin, see instructions at [CommonLibSSE NG Sample Plugin](https://gitlab.com/colorglass/commonlibsse-sample-plugin). This project follows common practices set there.

Note all building and CMake works only for msvc, don't build with clang. There is only a release-build, if you need logging use the ini settings and `EXTRALOG` function.

If you also understand the process for that commonlibSSE-ng SKSE sample plugin, you also understand the whole flow of this project as well.

### Using updated commonlib-ng version by alandtse

This plugin is built with VR support in mind, and uses alandtse's updated commonlibSSE-ng fork.

If you plan on using this fork of commonlibSSE-ng (since `CharmedBaryon`'s seems to be abandoned), follow similar instructions for the sample plugin but instead of managing `CommonibSSE NG` from vcpkg packages, you add the fork as a submodule.

git submodule add -b ng https://github.com/alandtse/CommonLibVR.git extern/CommonLibVR-ng
git submodule update --init --recursive

You then update the cmake file to reflect this by doing the following instead.

```

....
################################################################################
# CommonLibNG include, and options
################################################################################
include("${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibVR-ng/cmake/CommonLibSSE.cmake")
set(BUILD_TESTS OFF CACHE BOOL "Build unit tests for CommonLibVR." FORCE)
....

###############################################################################
# Find Dependencies
###############################################################################
find_package(CommonLibSSE CONFIG REQUIRED)
add_subdirectory(extern/CommonLibVR-ng)

.....

target_link_libraries(${PROJECT_NAME} PUBLIC
    CommonLibSSE::CommonLibSSE
.....
```

Everything can be the same as the sample plugin setup.

### Papyrus Dependencies

You can build with pyro, there is `Release.ppj` for main script build, and others like `OStim.ppj` are to add extra support.

You will notice all scripts of dependencies need to be placed under `dependencies`. Follow `INSTRUCTIONS.txt` to see where the originals are at.

### Licensing

`CC-BY-NC-SA-4.0`

Same as the precursors, that license is Share Alike, and this mod is likewise under the same license. If you create derivative works from this source code, they also must be under this license.

### Credits

See the following for extra information

- CharmedBaryon https://github.com/CharmedBaryon/CommonLibSSE-NG (CommonLibSSE-NG)
- alandtse CharmedBaryon https://github.com/alandtse/CommonLibVR/tree/ng (Latest CommonLibSSE-NG fork, with automatic VR ESL support)
- MetricExpansion's https://www.nexusmods.com/skyrimspecialedition/mods/42162 (Updated Skyrim Outfit System), [source](https://gitlab.com/metricexpansion/SkyrimOutfitSystemSE)
  - Note the version used as base was version v0.6 (See last 0.5+ commit [d57ba795186625bee879277f68d1a8f69a49bd93](https://gitlab.com/metricexpansion/SkyrimOutfitSystemSE/-/commit/d57ba795186625bee879277f68d1a8f69a49bd93)
- aers' [Skyrim SE port](https://github.com/aers/SkyrimOutfitSystemSE)  
- DavidJCobb's original Skyrim LE mod (https://github.com/DavidJCobb/skyrim-outfit-system).
