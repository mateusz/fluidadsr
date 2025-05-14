#include <iostream>
#include <string>
#include <memory>
#include <signal.h>
#include <fluidsynth.h>
#include <cstring>
#include <vector>
#include <getopt.h>

// FluidSynth object pointers and handlers
static std::unique_ptr<fluid_settings_t, decltype(&delete_fluid_settings)> settings{nullptr, delete_fluid_settings};
static std::unique_ptr<fluid_synth_t, decltype(&delete_fluid_synth)> synth{nullptr, delete_fluid_synth};
static std::unique_ptr<fluid_audio_driver_t, decltype(&delete_fluid_audio_driver)> adriver{nullptr, delete_fluid_audio_driver};
static std::unique_ptr<fluid_midi_driver_t, decltype(&delete_fluid_midi_driver)> mdriver{nullptr, delete_fluid_midi_driver};

// Control change constants
enum fluid_midi_control_change {
    SOUND_CTRL2 = 71,  // Filter resonance
    SOUND_CTRL3 = 72,  // Release time
    SOUND_CTRL4 = 73,  // Attack time
    SOUND_CTRL5 = 74,  // Filter cutoff
    SOUND_CTRL6 = 75,  // Decay time
    SOUND_CTRL10 = 79  // Sustain
};

// Global flag for handling program termination
static bool running = true;

// Signal handler for proper shutdown
void signal_handler(int sig) {
    std::cout << "\nCaught signal " << sig << ", exiting..." << std::endl;
    running = false;
}

// Setup ADSR and filter modulators
void setup_modulators(fluid_synth_t* synth) {
    // Set up envelope amount
    float env_amount = 20000.0f;

    // Filter resonance modulator
    std::unique_ptr<fluid_mod_t, decltype(&delete_fluid_mod)> mod{new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                         SOUND_CTRL2, // MIDI CC 71 Timbre/Harmonic Intensity (filter resonance)
                         FLUID_MOD_CC
                         | FLUID_MOD_UNIPOLAR
                         | FLUID_MOD_CONCAVE
                         | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_FILTERQ);
    fluid_mod_set_amount(mod.get(), 960.0f);
    fluid_synth_add_default_mod(synth, mod.get(), FLUID_SYNTH_ADD);

    // Release time modulator
    mod = {new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                         SOUND_CTRL3, // MIDI CC 72 Release time
                         FLUID_MOD_CC
                         | FLUID_MOD_UNIPOLAR
                         | FLUID_MOD_LINEAR
                         | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_VOLENVRELEASE);
    fluid_mod_set_amount(mod.get(), env_amount);
    fluid_synth_add_default_mod(synth, mod.get(), FLUID_SYNTH_ADD);

    // Attack time modulator
    mod = {new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                         SOUND_CTRL4, // MIDI CC 73 Attack time
                         FLUID_MOD_CC
                         | FLUID_MOD_UNIPOLAR
                         | FLUID_MOD_LINEAR
                         | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_VOLENVATTACK);
    fluid_mod_set_amount(mod.get(), env_amount);
    fluid_synth_add_default_mod(synth, mod.get(), FLUID_SYNTH_ADD);

    // Filter cutoff modulator
    mod = {new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                         SOUND_CTRL5, // MIDI CC 74 Brightness (cutoff frequency, FILTERFC)
                         FLUID_MOD_CC
                         | FLUID_MOD_LINEAR
                         | FLUID_MOD_UNIPOLAR
                         | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_FILTERFC);
    fluid_mod_set_amount(mod.get(), -2400.0f);
    fluid_synth_add_default_mod(synth, mod.get(), FLUID_SYNTH_ADD);

    // Decay time modulator
    mod = {new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                         SOUND_CTRL6, // MIDI CC 75 Decay Time
                         FLUID_MOD_CC
                         | FLUID_MOD_UNIPOLAR
                         | FLUID_MOD_LINEAR
                         | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_VOLENVDECAY);
    fluid_mod_set_amount(mod.get(), env_amount);
    fluid_synth_add_default_mod(synth, mod.get(), FLUID_SYNTH_ADD);

    // Sustain modulator
    mod = {new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                         SOUND_CTRL10, // MIDI CC 79 undefined
                         FLUID_MOD_CC
                         | FLUID_MOD_UNIPOLAR
                         | FLUID_MOD_CONCAVE
                         | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_VOLENVSUSTAIN);
    fluid_mod_set_amount(mod.get(), 1000.0f);
    fluid_synth_add_default_mod(synth, mod.get(), FLUID_SYNTH_ADD);
}

// Initialize all controllers with value 0
void init_controllers(fluid_synth_t* synth, int channel) {
    fluid_synth_cc(synth, channel, SOUND_CTRL2, 0);  // Filter resonance
    fluid_synth_cc(synth, channel, SOUND_CTRL3, 0);  // Release
    fluid_synth_cc(synth, channel, SOUND_CTRL4, 0);  // Attack
    fluid_synth_cc(synth, channel, SOUND_CTRL5, 0);  // Filter cutoff
    fluid_synth_cc(synth, channel, SOUND_CTRL6, 0);  // Decay
    fluid_synth_cc(synth, channel, SOUND_CTRL10, 0); // Sustain
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options] /path/to/soundfont.sf2\n"
              << "Options:\n"
              << "  -a, --audio-driver=DRIVER       Audio driver [alsa, coreaudio, etc.]\n"
              << "  -m, --midi-driver=DRIVER        MIDI driver [alsa_seq, coremidi, etc.]\n"
              << "  -o name=value                   Define a setting\n"
              << "  -c, --audio-bufcount=COUNT      Number of audio buffers\n"
              << "  -z, --audio-bufsize=SIZE        Size of each audio buffer\n"
              << "  -G, --audio-groups=NUM          Number of audio groups\n"
              << "  -j, --connect-jack-outputs      Connect jack outputs to physical ports\n"
              << "  -s, --server                    Start as a server process\n"
              << "  -r, --sample-rate=RATE          Set the sample rate\n"
              << "  -g, --gain=GAIN                 Set the gain [0 < gain < 10, default = 0.2]\n"
              << "  -h, --help                      Show this help\n"
              << std::endl;
}

int main(int argc, char** argv) {
    std::string audio_driver = "alsa";
    std::string midi_driver = "alsa_seq";
    std::vector<std::pair<std::string, std::string>> option_settings;
    int audio_bufcount = 0;
    int audio_bufsize = 0;
    int audio_groups = 0;
    bool connect_jack = false;
    bool server_mode = false;
    float sample_rate = 0;
    float gain = 0.2f;
    std::string soundfont_path;

    // Define long options
    static struct option long_options[] = {
        {"audio-driver", required_argument, nullptr, 'a'},
        {"midi-driver", required_argument, nullptr, 'm'},
        {"audio-bufcount", required_argument, nullptr, 'c'},
        {"audio-bufsize", required_argument, nullptr, 'z'},
        {"audio-groups", required_argument, nullptr, 'G'},
        {"connect-jack-outputs", no_argument, nullptr, 'j'},
        {"server", no_argument, nullptr, 's'},
        {"sample-rate", required_argument, nullptr, 'r'},
        {"gain", required_argument, nullptr, 'g'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "a:m:o:c:z:G:jsr:g:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'a':
                audio_driver = optarg;
                break;
            case 'm':
                midi_driver = optarg;
                break;
            case 'o': {
                // Parse key=value setting
                std::string arg = optarg;
                size_t pos = arg.find('=');
                if (pos != std::string::npos) {
                    std::string key = arg.substr(0, pos);
                    std::string value = arg.substr(pos + 1);
                    option_settings.push_back({key, value});
                } else {
                    std::cerr << "Invalid setting format: " << arg << std::endl;
                    return 1;
                }
                break;
            }
            case 'c':
                audio_bufcount = std::stoi(optarg);
                break;
            case 'z':
                audio_bufsize = std::stoi(optarg);
                break;
            case 'G':
                audio_groups = std::stoi(optarg);
                break;
            case 'j':
                connect_jack = true;
                break;
            case 's':
                server_mode = true;
                break;
            case 'r':
                sample_rate = std::stof(optarg);
                break;
            case 'g':
                gain = std::stof(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // Get soundfont path from remaining arguments
    if (optind < argc) {
        soundfont_path = argv[optind];
    } else {
        std::cerr << "No SoundFont specified" << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    // Register signal handlers for clean shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create and configure FluidSynth settings
    settings = {new_fluid_settings(), delete_fluid_settings};
    if (!settings) {
        std::cerr << "Failed to create FluidSynth settings" << std::endl;
        return 1;
    }

    // Configure audio and MIDI drivers
    fluid_settings_setstr(settings.get(), "audio.driver", audio_driver.c_str());
    fluid_settings_setstr(settings.get(), "midi.driver", midi_driver.c_str());
    fluid_settings_setint(settings.get(), "midi.autoconnect", 1);

    // Configure additional settings from -o options
    for (const auto& setting : option_settings) {
        const std::string& name = setting.first;
        const std::string& value = setting.second;

        // Determine setting type and apply appropriate setting function
        int type = fluid_settings_get_type(settings.get(), name.c_str());
        switch (type) {
            case FLUID_NUM_TYPE:
                fluid_settings_setnum(settings.get(), name.c_str(), std::stod(value));
                break;
            case FLUID_INT_TYPE:
                fluid_settings_setint(settings.get(), name.c_str(), std::stoi(value));
                break;
            case FLUID_STR_TYPE:
                fluid_settings_setstr(settings.get(), name.c_str(), value.c_str());
                break;
            default:
                std::cerr << "Unknown setting type for: " << name << std::endl;
                break;
        }
    }

    // Apply command-line options to settings
    if (audio_bufcount > 0) {
        fluid_settings_setint(settings.get(), "audio.periods", audio_bufcount);
    }

    if (audio_bufsize > 0) {
        fluid_settings_setint(settings.get(), "audio.period-size", audio_bufsize);
    }

    if (audio_groups > 0) {
        fluid_settings_setint(settings.get(), "synth.audio-groups", audio_groups);
    }

    if (connect_jack) {
        fluid_settings_setint(settings.get(), "audio.jack.autoconnect", 1);
    }

    if (sample_rate > 0) {
        fluid_settings_setnum(settings.get(), "synth.sample-rate", sample_rate);
    }

    fluid_settings_setnum(settings.get(), "synth.gain", gain);

    // Create the synthesizer
    synth = {new_fluid_synth(settings.get()), delete_fluid_synth};
    if (!synth) {
        std::cerr << "Failed to create FluidSynth synthesizer" << std::endl;
        return 1;
    }

    // Load SoundFont
    int sfont_id = fluid_synth_sfload(synth.get(), soundfont_path.c_str(), 1);
    if (sfont_id == -1) {
        std::cerr << "Failed to load SoundFont: " << soundfont_path << std::endl;
        return 1;
    }

    // Set up ADSR and filter modulators
    setup_modulators(synth.get());

    // Initialize controller values for channel 0
    int channel = 0;
    init_controllers(synth.get(), channel);

    // Create audio driver
    adriver = {new_fluid_audio_driver(settings.get(), synth.get()), delete_fluid_audio_driver};
    if (!adriver) {
        std::cerr << "Failed to create audio driver" << std::endl;
        return 1;
    }

    // Create MIDI driver
    mdriver = {new_fluid_midi_driver(settings.get(), fluid_synth_handle_midi_event, synth.get()), delete_fluid_midi_driver};
    if (!mdriver) {
        std::cerr << "Failed to create MIDI driver" << std::endl;
        return 1;
    }

    std::cout << "FluidADSR started with SoundFont: " << soundfont_path << std::endl;
    std::cout << "Press Ctrl+C to quit" << std::endl;

    // Main loop
    while (running) {
        sleep(1);
    }

    std::cout << "Shutting down FluidADSR..." << std::endl;

    return 0;
}