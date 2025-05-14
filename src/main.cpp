#include <iostream>
#include <string>
#include <memory>
#include <signal.h>
#include <fluidsynth.h>

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
    fluid_mod_set_amount(mod.get(), FLUID_PEAK_ATTENUATION);
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

int main(int argc, char** argv) {
    // Print usage if no arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " /path/to/soundfont.sf2" << std::endl;
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

    // Configure ALSA settings
    fluid_settings_setstr(settings.get(), "audio.driver", "alsa");
    fluid_settings_setstr(settings.get(), "midi.driver", "alsa_seq");
    fluid_settings_setint(settings.get(), "midi.autoconnect", 1);
    fluid_settings_setnum(settings.get(), "synth.gain", 2.0);

    // Create the synthesizer
    synth = {new_fluid_synth(settings.get()), delete_fluid_synth};
    if (!synth) {
        std::cerr << "Failed to create FluidSynth synthesizer" << std::endl;
        return 1;
    }

    // Load SoundFont
    std::string soundfont_path = argv[1];
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
    std::cout << "MIDI controllers for ADSR and filter:" << std::endl;
    std::cout << " CC 71: Filter resonance" << std::endl;
    std::cout << " CC 72: Release time" << std::endl;
    std::cout << " CC 73: Attack time" << std::endl;
    std::cout << " CC 74: Filter cutoff" << std::endl;
    std::cout << " CC 75: Decay time" << std::endl;
    std::cout << " CC 79: Sustain" << std::endl;
    std::cout << "Press Ctrl+C to quit" << std::endl;

    // Main loop
    while (running) {
        sleep(1);
    }

    std::cout << "Shutting down FluidADSR..." << std::endl;

    return 0;
}