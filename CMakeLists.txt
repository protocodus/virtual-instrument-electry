cmake_minimum_required(VERSION 3.22)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

option(ELECTRY_BUILD_UNIVERSAL "Build arm64 and x86_64 slices on macOS" ON)

# These must be established before project() so CMake's compiler checks use the
# same deployment target and architectures as the final plug-in.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING
            "Minimum supported macOS version")
    endif()

    if(ELECTRY_BUILD_UNIVERSAL AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING
            "macOS architectures")
    endif()
endif()

# C is required in addition to C++: JUCE's macOS plug-in wrappers (AU, Standalone)
# compile C/Objective-C sources. Restricting this to CXX breaks non-Xcode
# generators (Ninja/Makefiles) with a missing CMAKE_C_COMPILE_OBJECT error.
project(Electry VERSION 1.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ELECTRY_BUILD_PLUGIN "Build the JUCE plug-in and standalone app" ON)
option(ELECTRY_BUILD_CLAP "Build the CLAP plug-in alongside the JUCE formats" ON)
option(ELECTRY_MEASURED_BODY_RESPONSE
    "Use the shipping matched-material modal body response" ON)
option(ELECTRY_DECOUPLED_PICK_RELEASE
    "Use the shipping force-decoupled plectrum release response" ON)
option(ELECTRY_MEASURED_MODERN_CABINET
    "Build the default-off measured-IR Modern cabinet candidate" OFF)
option(ELECTRY_LOW_STRING_LOSS_CORRECTION_ORDER2
    "Build the default-off fitted order-2 lowest-string loss correction" OFF)
option(ELECTRY_ENERGY_ATTACK_PITCH
    "Use the bounded energy-derived common attack-pitch relaxation" ON)
option(ELECTRY_POSITIONED_FRET_COLLISION
    "Build the default-off unilateral following-fret scatter experiment" OFF)
option(ELECTRY_MEASURED_PICKUP_FLUX
    "Build the default-off measurement-fitted pickup flux-linkage candidate" OFF)
option(ELECTRY_PASSIVE_REPICK_SPRING
    "Build the default-off passive dynamic repick-contact candidate" OFF)
set(ELECTRY_JUCE_PATH "" CACHE PATH
    "Optional path to a local JUCE 8.0.14 checkout (avoids downloading JUCE)")
set(ELECTRY_CLAP_JUCE_EXTENSIONS_PATH "" CACHE PATH
    "Optional local clap-juce-extensions checkout with initialized submodules")

include(CTest)

set(ELECTRY_DSP_SOURCES
    Source/DSP/ElectryEngine.cpp
    Source/DSP/ElectryFx.cpp
    Source/DSP/ElectryVisuals.cpp)
set(ELECTRY_DSP_HEADERS
    Source/DSP/DspMath.h
    Source/DSP/ElectryEngine.h
    Source/DSP/ElectryFx.h
    Source/DSP/ModernCabinetIR.h
    Source/DSP/ElectryVisuals.h)

add_library(ElectryDSP STATIC
    ${ELECTRY_DSP_SOURCES}
    ${ELECTRY_DSP_HEADERS})

target_include_directories(ElectryDSP
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_features(ElectryDSP PUBLIC cxx_std_20)
set_target_properties(ElectryDSP PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(ELECTRY_LOW_STRING_LOSS_CORRECTION_ORDER2)
    target_compile_definitions(ElectryDSP PUBLIC
        ELECTRY_LOW_STRING_LOSS_CORRECTION_ORDER2=1)
endif()

if(ELECTRY_ENERGY_ATTACK_PITCH)
    target_compile_definitions(ElectryDSP PUBLIC
        ELECTRY_ENERGY_ATTACK_PITCH=1)
endif()

if(ELECTRY_POSITIONED_FRET_COLLISION)
    target_compile_definitions(ElectryDSP PUBLIC
        ELECTRY_POSITIONED_FRET_COLLISION=1)
endif()

if(ELECTRY_MEASURED_PICKUP_FLUX)
    target_compile_definitions(ElectryDSP PUBLIC
        ELECTRY_MEASURED_PICKUP_FLUX=1)
endif()

if(ELECTRY_PASSIVE_REPICK_SPRING)
    target_compile_definitions(ElectryDSP PUBLIC
        ELECTRY_PASSIVE_REPICK_SPRING=1)
endif()

if(ELECTRY_MEASURED_MODERN_CABINET)
    target_compile_definitions(ElectryDSP PUBLIC
        ELECTRY_MEASURED_MODERN_CABINET=1)
endif()

if(ELECTRY_MEASURED_BODY_RESPONSE)
    target_compile_definitions(ElectryDSP PUBLIC
        ELECTRY_MEASURED_BODY_RESPONSE=1)
endif()

if(ELECTRY_DECOUPLED_PICK_RELEASE)
    target_compile_definitions(ElectryDSP PUBLIC
        ELECTRY_DECOUPLED_PICK_RELEASE=1)
endif()

if(MSVC)
    target_compile_options(ElectryDSP PRIVATE /W4 /permissive-)
else()
    target_compile_options(ElectryDSP PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ELECTRY_BUILD_PLUGIN)
    if(ELECTRY_JUCE_PATH)
        if(NOT EXISTS "${ELECTRY_JUCE_PATH}/CMakeLists.txt")
            message(FATAL_ERROR
                "ELECTRY_JUCE_PATH does not contain a JUCE CMakeLists.txt: ${ELECTRY_JUCE_PATH}")
        endif()
        add_subdirectory("${ELECTRY_JUCE_PATH}"
                         "${CMAKE_CURRENT_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(
            JUCE
            # JUCE 8.0.14, pinned to its immutable release commit and archive
            # checksum so automated release builds cannot follow a moved tag.
            URL https://github.com/juce-framework/JUCE/archive/2cdfca8feb300fb424002ba2c2751569e5bacb64.tar.gz
            URL_HASH SHA256=814019716960a16f8f850a1593eeb56e2f21910aff2d673f7d3513b4ebaf251d)
        FetchContent_MakeAvailable(JUCE)
    endif()

    if(ELECTRY_BUILD_CLAP)
        if(ELECTRY_CLAP_JUCE_EXTENSIONS_PATH)
            foreach(required_file CMakeLists.txt
                    clap-libs/clap/CMakeLists.txt
                    clap-libs/clap-helpers/CMakeLists.txt)
                if(NOT EXISTS "${ELECTRY_CLAP_JUCE_EXTENSIONS_PATH}/${required_file}")
                    message(FATAL_ERROR
                        "ELECTRY_CLAP_JUCE_EXTENSIONS_PATH needs a complete checkout with submodules: missing ${required_file}")
                endif()
            endforeach()
            add_subdirectory("${ELECTRY_CLAP_JUCE_EXTENSIONS_PATH}"
                             "${CMAKE_CURRENT_BINARY_DIR}/clap-juce-extensions"
                             EXCLUDE_FROM_ALL)
        else()
            include(FetchContent)
            FetchContent_Declare(
                clap_juce_extensions
                GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
                # This commit supports JUCE 8 and records immutable gitlinks for
                # CLAP 29ffcc273be7c7c651f6c9953b99e69700e2387a and
                # clap-helpers a61bcdf0ecc2c8db1e80bfe8bf9cb7e8d9fd2bbc.
                GIT_TAG c1a5ad025f95d01e03267857fa8276ebeed16500
                GIT_SUBMODULES clap-libs/clap clap-libs/clap-helpers
                GIT_SUBMODULES_RECURSE TRUE)
            FetchContent_MakeAvailable(clap_juce_extensions)
        endif()
    endif()

    if(APPLE)
        set(ELECTRY_PLUGIN_FORMATS VST3 AU Standalone)
    else()
        set(ELECTRY_PLUGIN_FORMATS VST3 Standalone)
    endif()

    juce_add_plugin(Electry
        VERSION ${PROJECT_VERSION}
        COMPANY_NAME "Electry Audio"
        BUNDLE_ID "audio.electry.synth"
        PLUGIN_MANUFACTURER_CODE Eltr
        PLUGIN_CODE Elc1
        PRODUCT_NAME "Electry"
        DESCRIPTION "Electry physically modeled dry electric guitar"
        FORMATS ${ELECTRY_PLUGIN_FORMATS}
        VST3_CATEGORIES Instrument Synth
        AU_MAIN_TYPE kAudioUnitType_MusicDevice
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        AU_SANDBOX_SAFE TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE)

    juce_generate_juce_header(Electry)

    target_sources(Electry PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h)

    target_link_libraries(Electry
        PRIVATE
            ElectryDSP
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    target_compile_definitions(Electry
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0)

    if(ELECTRY_BUILD_CLAP)
        clap_juce_extensions_plugin(TARGET Electry
            CLAP_ID "audio.electry.synth"
            CLAP_FEATURES instrument synthesizer stereo)
    endif()
endif()

# The committed demonstration WAVs under Docs/audio are rendered by this tool
# from the same JUCE-free path the plug-in uses, so they cannot drift away from
# what Electry actually produces. It is built by default, including on Linux CI,
# so it cannot rot unnoticed.
option(ELECTRY_BUILD_TOOLS "Build the demo/evaluation renderers and FX benchmark" ON)

if(ELECTRY_BUILD_TOOLS)
    add_executable(ElectryRenderDemos Tools/RenderDemos.cpp)
    target_link_libraries(ElectryRenderDemos PRIVATE ElectryDSP)
    target_compile_features(ElectryRenderDemos PRIVATE cxx_std_20)

    add_executable(ElectryRenderEvaluation Tools/RenderEvaluation.cpp)
    target_link_libraries(ElectryRenderEvaluation PRIVATE ElectryDSP)
    target_compile_features(ElectryRenderEvaluation PRIVATE cxx_std_20)
    target_compile_definitions(ElectryRenderEvaluation PRIVATE
        ELECTRY_EVALUATION_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}/evaluation"
        ELECTRY_EVALUATION_PROJECT_VERSION="${PROJECT_VERSION}")

    add_executable(ElectryBenchmarkFx Tools/BenchmarkFx.cpp)
    target_link_libraries(ElectryBenchmarkFx PRIVATE ElectryDSP)
    target_compile_features(ElectryBenchmarkFx PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(ElectryRenderDemos PRIVATE /W4 /permissive-)
        target_compile_options(ElectryRenderEvaluation PRIVATE /W4 /permissive-)
        target_compile_options(ElectryBenchmarkFx PRIVATE /W4 /permissive-)
    else()
        target_compile_options(ElectryRenderDemos PRIVATE
            -Wall -Wextra -Wpedantic)
        target_compile_options(ElectryRenderEvaluation PRIVATE
            -Wall -Wextra -Wpedantic)
        target_compile_options(ElectryBenchmarkFx PRIVATE
            -Wall -Wextra -Wpedantic)
    endif()
endif()

if(BUILD_TESTING)
    add_executable(ElectryRealismTimingTests Tests/RealismTimingTests.cpp)
    target_link_libraries(ElectryRealismTimingTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismTiming COMMAND ElectryRealismTimingTests)

    add_executable(ElectryRealismExcitationTests Tests/RealismExcitationTests.cpp)
    target_link_libraries(ElectryRealismExcitationTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismExcitation COMMAND ElectryRealismExcitationTests)

    add_executable(ElectryRealismContactTests Tests/RealismContactTests.cpp)
    target_link_libraries(ElectryRealismContactTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismContact COMMAND ElectryRealismContactTests)

    add_executable(ElectryRealismEverydayContactTests Tests/RealismEverydayContactTests.cpp)
    target_link_libraries(ElectryRealismEverydayContactTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismEverydayContact COMMAND ElectryRealismEverydayContactTests)

    add_executable(ElectryRealismRhythmTests Tests/RealismRhythmTests.cpp)
    target_link_libraries(ElectryRealismRhythmTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismRhythm COMMAND ElectryRealismRhythmTests)

    add_executable(ElectryRealismTransitionControlsTests Tests/RealismTransitionControlsTests.cpp)
    target_link_libraries(ElectryRealismTransitionControlsTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismTransitionControls COMMAND ElectryRealismTransitionControlsTests)

    add_executable(ElectryRealismTextureTests Tests/RealismTextureTests.cpp)
    target_link_libraries(ElectryRealismTextureTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismTexture COMMAND ElectryRealismTextureTests)

    add_executable(ElectryRealismHarmonicTests Tests/RealismHarmonicTests.cpp)
    target_link_libraries(ElectryRealismHarmonicTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismHarmonic COMMAND ElectryRealismHarmonicTests)

    add_executable(ElectryRealismHarmonicAttackTests Tests/RealismHarmonicAttackTests.cpp)
    target_link_libraries(ElectryRealismHarmonicAttackTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismHarmonicAttack COMMAND ElectryRealismHarmonicAttackTests)

    add_executable(ElectryRealismPickAttackTests Tests/RealismPickAttackTests.cpp)
    target_link_libraries(ElectryRealismPickAttackTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismPickAttack COMMAND ElectryRealismPickAttackTests)

    add_executable(ElectryRealismGestureTests Tests/RealismGestureTests.cpp)
    target_link_libraries(ElectryRealismGestureTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismGesture COMMAND ElectryRealismGestureTests)

    add_executable(ElectryRealismLossTests Tests/RealismLossTests.cpp)
    target_link_libraries(ElectryRealismLossTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismLoss COMMAND ElectryRealismLossTests)

    add_executable(ElectryRealismDecayTests Tests/RealismDecayTests.cpp)
    target_link_libraries(ElectryRealismDecayTests PRIVATE ElectryDSP)
    add_test(NAME Electry.RealismDecay COMMAND ElectryRealismDecayTests)

    add_executable(ElectryEngineTests Tests/ElectryEngineTests.cpp)
    target_link_libraries(ElectryEngineTests PRIVATE ElectryDSP)
    target_compile_features(ElectryEngineTests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(ElectryEngineTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(ElectryEngineTests PRIVATE
            -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME Electry.ElectryEngine COMMAND ElectryEngineTests)
    # Sanitizer instrumentation slows the multi-rate render matrix by roughly
    # an order of magnitude; keep the normal runaway guard tight while
    # retaining full coverage under instrumentation.
    # The unsanitized bound was 60 s. The picking-hand variation fixture added
    # in the 2026-08-07 pass renders two sets of twelve strokes separated by
    # twelve-second gaps, plus a rapid set, which put the suite at 54 s on a
    # loaded four-core Linux box and past 60 s on slower runners — so the guard
    # had started firing on ordinary growth rather than on the runaway it is
    # for. Raised to 300 s, keeping roughly the same margin over the real
    # runtime that the 60 s bound gave when it was chosen.
    if(CMAKE_CXX_FLAGS MATCHES "fsanitize")
        set(ELECTRY_ENGINE_TEST_TIMEOUT 600)
    else()
        set(ELECTRY_ENGINE_TEST_TIMEOUT 300)
    endif()
    set_tests_properties(Electry.ElectryEngine PROPERTIES
        TIMEOUT ${ELECTRY_ENGINE_TEST_TIMEOUT})

    add_executable(ElectryFxTests Tests/ElectryFxTests.cpp)
    target_link_libraries(ElectryFxTests PRIVATE ElectryDSP)
    target_compile_features(ElectryFxTests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(ElectryFxTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(ElectryFxTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME Electry.ElectryFx COMMAND ElectryFxTests)
    # This suite renders about 140 seconds of stereo audio across the normal
    # rate matrix and every nonlinear oversampling boundary. Debug and
    # instrumented builds need the wider guard; optimised builds still retain
    # a three-minute runaway bound.
    if(CMAKE_CXX_FLAGS MATCHES "fsanitize"
       OR CMAKE_BUILD_TYPE STREQUAL "Debug"
       OR NOT CMAKE_BUILD_TYPE)
        set(ELECTRY_FX_TEST_TIMEOUT 600)
    else()
        set(ELECTRY_FX_TEST_TIMEOUT 300)
    endif()
    set_tests_properties(Electry.ElectryFx PROPERTIES
        TIMEOUT ${ELECTRY_FX_TEST_TIMEOUT})

    if(ELECTRY_BUILD_TOOLS)
        # Guards the renderer that produces the committed demonstration WAVs:
        # it renders a short take to a temporary directory and checks that the
        # audio is finite, audible and written as a readable WAV.
        add_test(NAME Electry.RenderDemos
                 COMMAND ElectryRenderDemos --smoke
                         "${CMAKE_CURRENT_BINARY_DIR}/demo-smoke")
        set_tests_properties(Electry.RenderDemos PROPERTIES TIMEOUT 60)

        # Render, exercise mutation-tested signal oracles, read every written
        # float payload back, parse the JSON manifest with CMake's own reader,
        # and inspect every WAV header and length.
        add_test(NAME Electry.RenderEvaluation
                 COMMAND ${CMAKE_COMMAND}
                         -DELECTRY_EVALUATION_RENDERER=$<TARGET_FILE:ElectryRenderEvaluation>
                         -DELECTRY_EVALUATION_DIR=${CMAKE_CURRENT_BINARY_DIR}/evaluation-test
                         -DELECTRY_EXPECT_ANALYTIC_RELEASE_IC=OFF
                         -DELECTRY_EXPECT_DECOUPLED_PICK_RELEASE=${ELECTRY_DECOUPLED_PICK_RELEASE}
                         -DELECTRY_EXPECT_MEASURED_BODY_RESPONSE=${ELECTRY_MEASURED_BODY_RESPONSE}
                         -DELECTRY_EXPECT_LOW_STRING_LOSS_CORRECTION_ORDER2=${ELECTRY_LOW_STRING_LOSS_CORRECTION_ORDER2}
                         -DELECTRY_EXPECT_ENERGY_ATTACK_PITCH=${ELECTRY_ENERGY_ATTACK_PITCH}
                         -DELECTRY_EXPECT_POSITIONED_FRET_COLLISION=${ELECTRY_POSITIONED_FRET_COLLISION}
                         -DELECTRY_EXPECT_MEASURED_PICKUP_FLUX=${ELECTRY_MEASURED_PICKUP_FLUX}
                         -DELECTRY_EXPECT_PASSIVE_REPICK_SPRING=${ELECTRY_PASSIVE_REPICK_SPRING}
                         -DELECTRY_EXPECT_MEASURED_MODERN_CABINET=${ELECTRY_MEASURED_MODERN_CABINET}
                         -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateEvaluation.cmake)
        set_tests_properties(Electry.RenderEvaluation PROPERTIES TIMEOUT 60)
    endif()

    if(ELECTRY_BUILD_PLUGIN)
        add_executable(ElectryPluginProcessorTests Tests/PluginProcessorTests.cpp)
        target_link_libraries(ElectryPluginProcessorTests PRIVATE Electry)
        target_include_directories(ElectryPluginProcessorTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/Source"
            "$<TARGET_PROPERTY:Electry,JUCE_GENERATED_SOURCES_DIRECTORY>"
            "$<TARGET_PROPERTY:Electry,INCLUDE_DIRECTORIES>")
        target_compile_features(ElectryPluginProcessorTests PRIVATE cxx_std_20)
        if(ELECTRY_MEASURED_BODY_RESPONSE)
            target_compile_definitions(ElectryPluginProcessorTests PRIVATE
                ELECTRY_MEASURED_BODY_RESPONSE=1)
        endif()

        if(MSVC)
            target_compile_options(ElectryPluginProcessorTests PRIVATE /W4 /permissive-)
        else()
            target_compile_options(ElectryPluginProcessorTests PRIVATE
                -Wall -Wextra -Wpedantic)
        endif()

        add_test(NAME Electry.PluginProcessor COMMAND ElectryPluginProcessorTests)
        set_tests_properties(Electry.PluginProcessor PROPERTIES TIMEOUT 60)

        # Black-box the built product rather than linking the processor class:
        # this catches a broken VST3 bundle, wrapper or exported host contract.
        add_executable(ElectryVST3ArtifactSmokeTests
            Tests/VST3ArtifactSmokeTests.cpp)
        target_link_libraries(ElectryVST3ArtifactSmokeTests PRIVATE
            juce::juce_audio_processors_headless)
        target_compile_definitions(ElectryVST3ArtifactSmokeTests PRIVATE
            JUCE_PLUGINHOST_VST3=1
            ELECTRY_EXPECTED_VERSION="${PROJECT_VERSION}")
        target_compile_features(ElectryVST3ArtifactSmokeTests PRIVATE cxx_std_20)
        add_dependencies(ElectryVST3ArtifactSmokeTests Electry_VST3)

        if(MSVC)
            target_compile_options(ElectryVST3ArtifactSmokeTests PRIVATE
                /W4 /permissive-)
        else()
            target_compile_options(ElectryVST3ArtifactSmokeTests PRIVATE
                -Wall -Wextra -Wpedantic)
        endif()

        add_test(NAME Electry.VST3Artifact
                 COMMAND ElectryVST3ArtifactSmokeTests
                    "$<TARGET_GENEX_EVAL:Electry_VST3,$<TARGET_PROPERTY:Electry_VST3,JUCE_PLUGIN_ARTEFACT_FILE>>")
        set_tests_properties(Electry.VST3Artifact PROPERTIES
            TIMEOUT 60)

        if(ELECTRY_BUILD_CLAP)
            # Exercise the exported CLAP ABI from the actual binary, including
            # MIDI rendering and state restore, without linking the processor.
            add_executable(ElectryCLAPArtifactSmokeTests
                Tests/CLAPArtifactSmokeTests.cpp)
            target_link_libraries(ElectryCLAPArtifactSmokeTests PRIVATE
                clap ${CMAKE_DL_LIBS})
            target_compile_features(ElectryCLAPArtifactSmokeTests PRIVATE cxx_std_20)
            target_compile_definitions(ElectryCLAPArtifactSmokeTests PRIVATE
                ELECTRY_EXPECTED_VERSION="${PROJECT_VERSION}")
            add_dependencies(ElectryCLAPArtifactSmokeTests Electry_CLAP)
            if(MSVC)
                target_compile_options(ElectryCLAPArtifactSmokeTests PRIVATE
                    /W4 /permissive-)
            else()
                target_compile_options(ElectryCLAPArtifactSmokeTests PRIVATE
                    -Wall -Wextra -Wpedantic)
            endif()
            add_test(NAME Electry.CLAPArtifact
                     COMMAND ElectryCLAPArtifactSmokeTests
                         "$<TARGET_FILE:Electry_CLAP>")
            set_tests_properties(Electry.CLAPArtifact PROPERTIES TIMEOUT 60)
        endif()

        if(APPLE)
            # Load the exact AU binary through its exported factory and Core
            # Audio's process-local registration API. This avoids validating a
            # stale component discovered in the user's global AU directories.
            add_executable(ElectryAUArtifactSmokeTests
                Tests/ValidateAUArtifact.cpp)
            target_link_libraries(ElectryAUArtifactSmokeTests PRIVATE
                "-framework AudioToolbox"
                "-framework CoreFoundation")
            target_compile_features(ElectryAUArtifactSmokeTests PRIVATE cxx_std_20)
            target_compile_options(ElectryAUArtifactSmokeTests PRIVATE
                -Wall -Wextra -Wpedantic)
            add_dependencies(ElectryAUArtifactSmokeTests Electry_AU)

            add_test(NAME Electry.AUArtifact
                     COMMAND ElectryAUArtifactSmokeTests
                        "$<TARGET_FILE:Electry_AU>")
            set_tests_properties(Electry.AUArtifact PROPERTIES
                TIMEOUT 60)
        endif()
    endif()

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME Electry.RefreshGeneratedMedia
                 COMMAND "${Python3_EXECUTABLE}"
                         "${CMAKE_CURRENT_SOURCE_DIR}/Tests/RefreshGeneratedMediaTests.py")
        add_test(NAME Electry.PrepareBlindListening
                 COMMAND "${Python3_EXECUTABLE}"
                         "${CMAKE_CURRENT_SOURCE_DIR}/Tests/PrepareBlindListeningTests.py")
        add_test(NAME Electry.ScoreBlindListening
                 COMMAND "${Python3_EXECUTABLE}"
                         "${CMAKE_CURRENT_SOURCE_DIR}/Tests/ScoreBlindListeningTests.py")
        add_test(NAME Electry.ValidateReferenceFixture
                 COMMAND "${Python3_EXECUTABLE}"
                         "${CMAKE_CURRENT_SOURCE_DIR}/Tests/ValidateReferenceFixtureTests.py")
        add_test(NAME Electry.CompareReferenceFixture
                 COMMAND "${Python3_EXECUTABLE}"
                         "${CMAKE_CURRENT_SOURCE_DIR}/Tests/CompareReferenceFixtureTests.py")
        add_test(NAME Electry.FitDispersion
                 COMMAND "${Python3_EXECUTABLE}"
                         "${CMAKE_CURRENT_SOURCE_DIR}/Tests/FitDispersionTests.py")
        set_tests_properties(
            Electry.RefreshGeneratedMedia
            Electry.PrepareBlindListening Electry.ScoreBlindListening
            Electry.ValidateReferenceFixture Electry.CompareReferenceFixture
            Electry.FitDispersion
            PROPERTIES TIMEOUT 60)
    endif()
endif()
