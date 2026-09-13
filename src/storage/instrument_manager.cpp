#include "instrument_manager.h"
#include "../config.h"
#include "../debug.h"
#include "sample_loader.h"
#include "../midi/midi.h"
#include "../audio/audio_engine.h"
#include <math.h>

Instrument instruments[MAX_INSTRUMENTS];
int currentInstrument = 0;
int loadedInstruments = 0;

float calculatePitchRatio(float semitoneOffset) {
    // Calculate pitch ratio using 12-tone equal temperament
    // Each semitone is 2^(1/12) ratio
    return pow(2.0f, semitoneOffset / 12.0f);
}

KeySample* findBestKeySample(Instrument* instrument, uint8_t midiNote) {
    if (!instrument || !instrument->isLoaded) {
        return nullptr;
    }

    // First, look for a key sample that covers this note directly
    for (int i = 0; i < instrument->numKeySamples; i++) {
        KeySample* ks = &instrument->keySamples[i];
        if (ks->isLoaded && midiNote >= ks->minNote && midiNote <= ks->maxNote) {
            return ks;
        }
    }

    // If no direct match, find the closest key sample
    KeySample* closest = nullptr;
    int smallestDistance = 128;

    for (int i = 0; i < instrument->numKeySamples; i++) {
        KeySample* ks = &instrument->keySamples[i];
        if (ks->isLoaded) {
            int distance = abs((int)midiNote - (int)ks->rootNote);
            if (distance < smallestDistance) {
                smallestDistance = distance;
                closest = ks;
            }
        }
    }

    return closest;
}

bool loadKeySample(int instrumentIndex, const char* filename, uint8_t rootNote, uint8_t minNote, uint8_t maxNote) {
    if (instrumentIndex >= MAX_INSTRUMENTS || !instruments[instrumentIndex].isLoaded) {
        DEBUG("Invalid instrument index");
        return false;
    }

    Instrument* instrument = &instruments[instrumentIndex];

    if (instrument->numKeySamples >= MAX_SAMPLES) {
        DEBUGF("Instrument %s is full (max %d samples)\n", instrument->name.c_str(), MAX_SAMPLES);
        return false;
    }

    // Load the sample using the existing sample loader
    if (!loadSampleFromSD(filename, rootNote)) {
        DEBUGF("Failed to load sample %s\n", filename);
        return false;
    }

    // Find the sample that was just loaded
    Sample* sample = nullptr;
    for (int i = 0; i < loadedSamples; i++) {
        if (samples[i].midiNote == rootNote && samples[i].filename == filename) {
            sample = &samples[i];
            break;
        }
    }

    if (!sample) {
        DEBUG("Could not find loaded sample");
        return false;
    }

    // Add to instrument's key samples
    KeySample* ks = &instrument->keySamples[instrument->numKeySamples];
    ks->sample = sample;
    ks->rootNote = rootNote;
    ks->minNote = minNote;
    ks->maxNote = maxNote;
    ks->isLoaded = true;

    instrument->numKeySamples++;

    DEBUGF("Loaded key sample: %s -> root=%d, range=%d-%d\n", filename, rootNote, minNote, maxNote);

    return true;
}

int createInstrument(const char* name) {
    if (loadedInstruments >= MAX_INSTRUMENTS) {
        DEBUG("Cannot create more instruments");
        return -1;
    }

    Instrument* instrument = &instruments[loadedInstruments];
    instrument->name = name;
    instrument->numKeySamples = 0;
    instrument->isLoaded = true;
    instrument->pitchCorrectionSemitones = 0.0f;  // default: no correction

    // Initialize key samples
    for (int i = 0; i < MAX_SAMPLES; i++) {
        instrument->keySamples[i].isLoaded = false;
        instrument->keySamples[i].sample = nullptr;
    }

    DEBUGF("Created instrument: %s (index %d)\n", name, loadedInstruments);

    return loadedInstruments++;
}

void selectInstrument(int instrumentIndex) {
    if (instrumentIndex >= 0 && instrumentIndex < loadedInstruments) {
        currentInstrument = instrumentIndex;
        DEBUGF("Selected instrument: %s\n", instruments[currentInstrument].name.c_str());
    }
}

Instrument* getCurrentInstrument() {
    if (currentInstrument >= 0 && currentInstrument < loadedInstruments) {
        return &instruments[currentInstrument];
    }
    return nullptr;
}

void loadFlute() {
    int idx = createInstrument("Flute");
    if (idx == -1) return;

    // Single sample covering wide range
    loadKeySample(idx, "instruments/flute/fluteC5.wav", 60, 48, 96);
    DEBUGF("Loaded Flute with %d samples\n", instruments[idx].numKeySamples);
}

void loadHorn() {
    int idx = createInstrument("Horn");
    if (idx == -1) return;

    // Horn sample is ~40 cents flat; correct it in software by sharpening 0.4 semitones
    instruments[idx].pitchCorrectionSemitones = 0.4f;

    loadKeySample(idx, "instruments/horn/hornC_sharp_3.wav", 36, 36, 84);
    DEBUGF("Loaded Horn with %d samples\n", instruments[idx].numKeySamples);
}

void loadBassoon() {
    int idx = createInstrument("Bassoon");
    if (idx == -1) return;

    instruments[idx].pitchCorrectionSemitones = -0.1f;

    loadKeySample(idx, "instruments/bassoon/bassoonE2.wav", 40, 24, 60);
    DEBUGF("Loaded Bassoon with %d samples\n", instruments[idx].numKeySamples);
}

void loadOrgan() {
    int idx = createInstrument("Organ");
    if (idx == -1) return;

    loadKeySample(idx, "instruments/organ/organF4.wav", 66, 48, 84);
    DEBUGF("Loaded Organ with %d samples\n", instruments[idx].numKeySamples);
}

void loadGuitar() {
    int idx = createInstrument("Guitar");
    if (idx == -1) return;

    loadKeySample(idx, "instruments/guitar/guitarE2.wav", 40, 24, 72);
    DEBUGF("Loaded Guitar with %d samples\n", instruments[idx].numKeySamples);
}

void loadBass() {
    int idx = createInstrument("Bass");
    if (idx == -1) return;

    loadKeySample(idx, "instruments/bass/bassA1.wav", 33, 24, 60);
    DEBUGF("Loaded Bass with %d samples\n", instruments[idx].numKeySamples);
}

void loadVibraphone() {
    int idx = createInstrument("Vibraphone");
    if (idx == -1) return;

    // Sample is vibesF4.wav (F4=65). rootNote is set BELOW the sample pitch by
    // an octave (F3=53) so the sample is pitched UP, making the instrument
    // sound an octave higher. (rootNote = sample pitch - 12 for octave up.)
    loadKeySample(idx, "instruments/vibraphone/vibesF4.wav", 53, 36, 72);
    DEBUGF("Loaded Vibraphone with %d samples\n", instruments[idx].numKeySamples);
}

void loadMarimba() {
    int idx = createInstrument("Marimba");
    if (idx == -1) return;

    loadKeySample(idx, "instruments/marimba/marimba-c7.wav", 84, 72, 108);
    DEBUGF("Loaded Marimba with %d samples\n", instruments[idx].numKeySamples);
}

void loadRhodes() {
    int idx = createInstrument("Fender Rhodes");
    if (idx == -1) return;

    loadKeySample(idx, "instruments/rhodes/rhodes-a2.wav", 46, 24, 72);
    DEBUGF("Loaded Rhodes with %d samples\n", instruments[idx].numKeySamples);
}

void loadLatinPercussion() {
    int idx = createInstrument("Latin Percussion");
    if (idx == -1) return;

    // Map all sounds across the MIDI range
    loadKeySample(idx, "instruments/latin_percussion/maraca-dry-hit-low.wav", 36, 36, 36);
    loadKeySample(idx, "instruments/latin_percussion/maraca-dry-hit-mid.wav", 38, 38, 38);
    loadKeySample(idx, "instruments/latin_percussion/maraca-dry-hit-intense.wav", 40, 40, 40);
    loadKeySample(idx, "instruments/latin_percussion/maraca-hit-1.wav", 42, 42, 42);
    loadKeySample(idx, "instruments/latin_percussion/maraca-hit-2.wav", 44, 44, 44);
    loadKeySample(idx, "instruments/latin_percussion/maraca-hit-strong.wav", 46, 46, 46);
    loadKeySample(idx, "instruments/latin_percussion/maraca-hit-strong-2.wav", 48, 48, 48);
    loadKeySample(idx, "instruments/latin_percussion/maraca-pattern.wav", 50, 50, 50);
    loadKeySample(idx, "instruments/latin_percussion/maraca-rolling.wav", 52, 52, 52);
    loadKeySample(idx, "instruments/latin_percussion/rainstick-hit-down.wav", 54, 54, 54);
    loadKeySample(idx, "instruments/latin_percussion/rainstick-hit-up.wav", 56, 56, 56);
    loadKeySample(idx, "instruments/latin_percussion/rainstick-vibration-fast.wav", 58, 58, 58);
    loadKeySample(idx, "instruments/latin_percussion/rainstick-vibration-slow.wav", 60, 60, 60);
    loadKeySample(idx, "instruments/latin_percussion/rainstick-fast.wav", 62, 62, 62);
    loadKeySample(idx, "instruments/latin_percussion/rainstick-slow.wav", 64, 64, 64);

    DEBUGF("Loaded Latin Percussion with %d samples\n", instruments[idx].numKeySamples);
}

void loadAfricanPercussion() {
    int idx = createInstrument("African Percussion");
    if (idx == -1) return;

    // Map all doumbek sounds across the MIDI range
    loadKeySample(idx, "instruments/african_percussion/doumbek_doum1.wav", 36, 36, 36);
    loadKeySample(idx, "instruments/african_percussion/doumbek_doum2.wav", 38, 38, 38);
    loadKeySample(idx, "instruments/african_percussion/doumbek_doum3.wav", 40, 40, 40);
    loadKeySample(idx, "instruments/african_percussion/doumbek_tek1.wav", 42, 42, 42);
    loadKeySample(idx, "instruments/african_percussion/doumbek_tek2.wav", 44, 44, 44);
    loadKeySample(idx, "instruments/african_percussion/doumbek_tek3.wav", 46, 46, 46);
    loadKeySample(idx, "instruments/african_percussion/doumbek_ka1.wav", 48, 48, 48);
    loadKeySample(idx, "instruments/african_percussion/doumbek_ka2.wav", 50, 50, 50);
    loadKeySample(idx, "instruments/african_percussion/doumbek_ka3.wav", 52, 52, 52);
    loadKeySample(idx, "instruments/african_percussion/doumbek_slap1.wav", 54, 54, 54);
    loadKeySample(idx, "instruments/african_percussion/doumbek_slap2.wav", 56, 56, 56);
    loadKeySample(idx, "instruments/african_percussion/doumbek_mute1.wav", 58, 58, 58);
    loadKeySample(idx, "instruments/african_percussion/doumbek_mute2.wav", 60, 60, 60);
    loadKeySample(idx, "instruments/african_percussion/doumbek_snap.wav", 62, 62, 62);
    loadKeySample(idx, "instruments/african_percussion/doumbek_snap2.wav", 64, 64, 64);

    DEBUGF("Loaded African Percussion with %d samples\n", instruments[idx].numKeySamples);
}

// Update your existing drum kit function to use the new path
void loadBasicDrumKit() {
    int drumIndex = createInstrument("Basic Drums");
    if (drumIndex == -1) return;

    loadKeySample(drumIndex, "instruments/drums/kick.wav", 36, 36, 36);
    loadKeySample(drumIndex, "instruments/drums/snare.wav", 38, 38, 38);
    loadKeySample(drumIndex, "instruments/drums/hihat_closed.wav", 42, 42, 42);
    loadKeySample(drumIndex, "instruments/drums/hihat_open.wav", 46, 46, 46);
    loadKeySample(drumIndex, "instruments/drums/crash.wav", 49, 49, 49);
    loadKeySample(drumIndex, "instruments/drums/ride.wav", 51, 51, 51);

    DEBUGF("Loaded Basic Drums with %d samples\n", instruments[drumIndex].numKeySamples);
}

void unloadCurrentInstrument() {
    if (currentInstrument >= 0 && currentInstrument < loadedInstruments) {
        Instrument* inst = &instruments[currentInstrument];

        // Free all samples in this instrument
        for (int i = 0; i < inst->numKeySamples; i++) {
            if (inst->keySamples[i].sample && inst->keySamples[i].sample->data) {
                free(inst->keySamples[i].sample->data);
                inst->keySamples[i].sample->data = nullptr;
                inst->keySamples[i].sample->isLoaded = false;
            }
        }

        inst->numKeySamples = 0;
        inst->isLoaded = false;

        DEBUGF("Unloaded instrument: %s\n", inst->name.c_str());
    }

    // Reset the global sample counter if needed
    loadedSamples = 0;
}

void loadInstrumentForType(int typePosition) {
    // Stop all playing notes
    allNotesOff();
    stopAllVoices();  // Actually stop the audio voices

    // Small delay to let audio task finish current buffer
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // Unload current instrument to free memory
    unloadCurrentInstrument();

    // Reset instrument counter to reuse slot 0
    loadedInstruments = 0;
    currentInstrument = 0;

    // Load new instrument based on switch position
    switch(typePosition) {
        case 0:  loadFlute(); break;
        case 1:  loadHorn(); break;
        case 2:  loadBassoon(); break;
        case 3:  loadOrgan(); break;
        case 4:  loadGuitar(); break;
        case 5:  loadBass(); break;
        case 6:  loadVibraphone(); break;
        case 7:  loadMarimba(); break;
        case 8:  loadRhodes(); break;
        case 9:  loadLatinPercussion(); break;
        case 10: loadAfricanPercussion(); break;
        case 11: loadBasicDrumKit(); break;

        default:
            DEBUG("Invalid type position");
            return;
    }

    selectInstrument(0);
}
