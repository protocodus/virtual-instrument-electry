#include <clap/clap.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#if defined(_WIN32)
 #define NOMINMAX
 #include <windows.h>
#else
 #include <dlfcn.h>
#endif

namespace
{
void require (bool condition, const std::string& message)
{
    if (! condition)
        throw std::runtime_error (message);
}

struct Module
{
    explicit Module (const std::filesystem::path& path)
    {
       #if defined(_WIN32)
        handle = LoadLibraryW (path.c_str());
        require (handle != nullptr, "could not load CLAP binary (Windows error "
                                   + std::to_string (GetLastError()) + ")");
       #else
        handle = dlopen (path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (handle == nullptr)
            throw std::runtime_error (std::string ("could not load CLAP binary: ") + dlerror());
       #endif
    }

    ~Module()
    {
       #if defined(_WIN32)
        FreeLibrary (handle);
       #else
        dlclose (handle);
       #endif
    }

    const clap_plugin_entry_t* entry() const
    {
       #if defined(_WIN32)
        return reinterpret_cast<const clap_plugin_entry_t*> (GetProcAddress (handle, "clap_entry"));
       #else
        return static_cast<const clap_plugin_entry_t*> (dlsym (handle, "clap_entry"));
       #endif
    }

   #if defined(_WIN32)
    HMODULE handle {};
   #else
    void* handle {};
   #endif
};

struct EntryLifetime
{
    const clap_plugin_entry_t* entry;
    ~EntryLifetime() { entry->deinit(); }
};

struct PluginDeleter
{
    void operator() (const clap_plugin_t* plugin) const { plugin->destroy (plugin); }
};
using Plugin = std::unique_ptr<const clap_plugin_t, PluginDeleter>;

struct Host
{
    std::atomic<bool> callbackRequested { false };
    clap_host_t api {
        CLAP_VERSION, this, "Electry artifact test", "Electry Audio", "", "1.0",
        [] (const clap_host_t*, const char*) -> const void* { return nullptr; },
        [] (const clap_host_t*) {},
        [] (const clap_host_t*) {},
        [] (const clap_host_t* host)
        {
            static_cast<Host*> (host->host_data)->callbackRequested.store (true);
        }
    };

    void serviceCallbacks (const clap_plugin_t* plugin)
    {
        for (int attempt = 0; attempt < 8; ++attempt)
            if (callbackRequested.exchange (false))
                plugin->on_main_thread (plugin);
            else
                return;
        require (! callbackRequested.load(), "CLAP main-thread callback did not settle");
    }
};

struct Events
{
    const clap_event_header_t* event = nullptr;
    clap_input_events_t input {
        this,
        [] (const clap_input_events_t* list) -> uint32_t
        {
            return static_cast<const Events*> (list->ctx)->event != nullptr ? 1u : 0u;
        },
        [] (const clap_input_events_t* list, uint32_t index) -> const clap_event_header_t*
        {
            return index == 0 ? static_cast<const Events*> (list->ctx)->event : nullptr;
        }
    };
    clap_output_events_t output {
        nullptr,
        [] (const clap_output_events_t*, const clap_event_header_t*) { return true; }
    };
};

struct State
{
    std::vector<uint8_t> bytes;
    std::size_t position = 0;
    clap_ostream_t output {
        this,
        [] (const clap_ostream_t* stream, const void* buffer, uint64_t size) -> int64_t
        {
            auto& state = *static_cast<State*> (stream->ctx);
            if (size > 16 * 1024 * 1024 || state.bytes.size() + size > 16 * 1024 * 1024)
                return -1;
            try
            {
                const auto* begin = static_cast<const uint8_t*> (buffer);
                state.bytes.insert (state.bytes.end(), begin, begin + size);
                return static_cast<int64_t> (size);
            }
            catch (...)
            {
                return -1;
            }
        }
    };
    clap_istream_t input {
        this,
        [] (const clap_istream_t* stream, void* buffer, uint64_t size) -> int64_t
        {
            auto& state = *static_cast<State*> (stream->ctx);
            const auto count = std::min<uint64_t> (size, state.bytes.size() - state.position);
            std::memcpy (buffer, state.bytes.data() + state.position, static_cast<std::size_t> (count));
            state.position += static_cast<std::size_t> (count);
            return static_cast<int64_t> (count);
        }
    };
};

template <typename Extension>
const Extension* extension (const clap_plugin_t* plugin, const char* id)
{
    const auto* result = static_cast<const Extension*> (plugin->get_extension (plugin, id));
    require (result != nullptr, std::string ("missing CLAP extension: ") + id);
    return result;
}

struct ProcessingLifetime
{
    const clap_plugin_t* plugin;
    bool started = false;
    ~ProcessingLifetime()
    {
        if (started)
            plugin->stop_processing (plugin);
        plugin->deactivate (plugin);
    }
};

void testArtifact (const std::filesystem::path& path)
{
    Module module (path);
    const auto* entry = module.entry();
    require (entry != nullptr, "CLAP binary does not export clap_entry");
    require (clap_version_is_compatible (entry->clap_version), "incompatible CLAP entry version");
    auto pluginPath = path;
   #if defined(__APPLE__)
    pluginPath = path.parent_path().parent_path().parent_path();
   #endif
    require (entry->init (pluginPath.string().c_str()), "CLAP entry initialization failed");
    EntryLifetime entryLifetime { entry };

    const auto* factory = static_cast<const clap_plugin_factory_t*> (
        entry->get_factory (CLAP_PLUGIN_FACTORY_ID));
    require (factory != nullptr && factory->get_plugin_count (factory) == 1,
             "CLAP factory must expose exactly one plug-in");
    const auto* descriptor = factory->get_plugin_descriptor (factory, 0);
    require (descriptor != nullptr && descriptor->id != nullptr
             && std::strcmp (descriptor->id, "audio.electry.synth") == 0,
             "wrong CLAP plug-in ID");
    require (descriptor->name != nullptr && std::strcmp (descriptor->name, "Electry") == 0,
             "wrong CLAP product name");
    require (descriptor->version != nullptr
             && std::strcmp (descriptor->version, ELECTRY_EXPECTED_VERSION) == 0,
             "wrong CLAP product version");

    Host host;
    Plugin plugin (factory->create_plugin (factory, &host.api, descriptor->id));
    require (plugin != nullptr && plugin->init (plugin.get()), "CLAP instance initialization failed");
    host.serviceCallbacks (plugin.get());

    const auto* ports = extension<clap_plugin_audio_ports_t> (plugin.get(), CLAP_EXT_AUDIO_PORTS);
    clap_audio_port_info_t port {};
    require (ports->count (plugin.get(), true) == 0
             && ports->count (plugin.get(), false) == 1
             && ports->get (plugin.get(), 0, false, &port) && port.channel_count == 2,
             "CLAP must expose no audio input and one stereo output");
    const auto* notes = extension<clap_plugin_note_ports_t> (plugin.get(), CLAP_EXT_NOTE_PORTS);
    clap_note_port_info_t notePort {};
    require (notes->count (plugin.get(), true) == 1
             && notes->get (plugin.get(), 0, true, &notePort)
             && (notePort.supported_dialects & CLAP_NOTE_DIALECT_MIDI) != 0,
             "CLAP does not advertise MIDI input");

    const auto* params = extension<clap_plugin_params_t> (plugin.get(), CLAP_EXT_PARAMS);
    require (params->count (plugin.get()) == 28, "CLAP must expose 28 product parameters");
    std::vector<clap_param_info_t> parameterInfo;
    std::unordered_set<clap_id> parameterIDs;
    clap_param_info_t outputLevel {};
    bool foundOutputLevel = false;
    for (uint32_t index = 0; index < params->count (plugin.get()); ++index)
    {
        clap_param_info_t info {};
        require (params->get_info (plugin.get(), index, &info), "could not read CLAP parameter info");
        require (info.id != CLAP_INVALID_ID && parameterIDs.insert (info.id).second,
                 "CLAP parameter IDs must be valid and unique");
        parameterInfo.push_back (info);
        if (std::strcmp (info.name, "Output level") == 0)
        {
            outputLevel = info;
            foundOutputLevel = true;
        }
    }
    require (foundOutputLevel, "CLAP does not expose Output level");
    const auto setOutputLevel = [&] (double fraction)
    {
        clap_event_param_value_t change {};
        change.header = { sizeof (change), 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0 };
        change.param_id = outputLevel.id;
        change.cookie = outputLevel.cookie;
        change.note_id = -1;
        change.port_index = change.channel = change.key = -1;
        change.value = outputLevel.min_value + fraction * (outputLevel.max_value - outputLevel.min_value);
        Events events;
        events.event = &change.header;
        params->flush (plugin.get(), &events.input, &events.output);
        host.serviceCallbacks (plugin.get());
        double actual = 0;
        require (params->get_value (plugin.get(), outputLevel.id, &actual)
                 && std::abs (actual - change.value) < 1.0e-5,
                 "CLAP parameter event did not set Output level");
    };
    setOutputLevel (0.65);
    std::vector<double> savedValues;
    for (const auto& info : parameterInfo)
    {
        double value = 0;
        require (params->get_value (plugin.get(), info.id, &value), "could not read CLAP parameter value");
        savedValues.push_back (value);
    }
    const auto* stateExtension = extension<clap_plugin_state_t> (plugin.get(), CLAP_EXT_STATE);
    State state;
    require (stateExtension->save (plugin.get(), &state.output) && ! state.bytes.empty(),
             "CLAP state save failed");
    setOutputLevel (0.15);
    require (stateExtension->load (plugin.get(), &state.input), "CLAP state restore failed");
    host.serviceCallbacks (plugin.get());
    for (std::size_t index = 0; index < parameterInfo.size(); ++index)
    {
        double value = 0;
        require (params->get_value (plugin.get(), parameterInfo[index].id, &value)
                 && std::abs (value - savedValues[index]) < 1.0e-5,
                 std::string ("CLAP state did not restore parameter: ") + parameterInfo[index].name);
    }

    require (plugin->activate (plugin.get(), 48000.0, 256, 256), "CLAP activation failed");
    ProcessingLifetime processing { plugin.get() };
    processing.started = plugin->start_processing (plugin.get());
    require (processing.started, "CLAP start_processing failed");
    std::array<std::array<float, 256>, 2> samples {};
    std::array<float*, 2> channels { samples[0].data(), samples[1].data() };
    clap_audio_buffer_t audio { channels.data(), nullptr, 2, 0, 0 };
    Events events;
    clap_process_t process {};
    process.frames_count = 256;
    process.audio_outputs = &audio;
    process.audio_outputs_count = 1;
    process.in_events = &events.input;
    process.out_events = &events.output;
    double energy = 0;
    float peak = 0;
    for (int block = 0; block < 96; ++block)
    {
        for (auto& channel : samples)
            channel.fill (std::numeric_limits<float>::quiet_NaN());
        clap_event_midi_t midi {};
        midi.header = { sizeof (midi), 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_MIDI, 0 };
        midi.data[0] = block == 0 ? 0x90 : 0x80;
        midi.data[1] = 40;
        midi.data[2] = block == 0 ? 114 : 0;
        events.event = block == 0 || block == 72 ? &midi.header : nullptr;
        process.steady_time = static_cast<int64_t> (block) * process.frames_count;
        require (plugin->process (plugin.get(), &process) != CLAP_PROCESS_ERROR,
                 "CLAP audio processing failed");
        for (const auto& channel : samples)
            for (const auto value : channel)
            {
                require (std::isfinite (value), "CLAP render produced a non-finite or unwritten sample");
                peak = std::max (peak, std::abs (value));
                energy += static_cast<double> (value) * value;
            }
        host.serviceCallbacks (plugin.get());
    }
    require (peak > 1.0e-6f && energy > 1.0e-8, "CLAP MIDI render was silent");
}
} // namespace

int main (int argc, char** argv)
{
    try
    {
        require (argc == 2, "usage: ElectryCLAPArtifactSmokeTests <CLAP binary>");
        testArtifact (std::filesystem::absolute (argv[1]));
        std::cout << "Electry built CLAP artifact smoke test passed (28 parameters, state restore, MIDI audio)\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
