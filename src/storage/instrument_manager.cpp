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

void loadSoundmaker01() {
    int idx = createInstrument("Soundmaker01");
    if (idx == -1) return;

    // Load the guitar C sample ONCE
    if (!loadSampleFromSD("instruments/soundmaker01/01SM_3_Guitar_Note_3_C.wav", 38)) {
        DEBUG("Failed to load guitar sample");
        return;
    }

    // Get pointer to the loaded guitar sample
    Sample* guitarSample = &samples[loadedSamples - 1];

    // Manually create 5 KeySamples that all point to the same guitar Sample
    // but with different rootNote values to achieve pitch shifting
    Instrument* inst = &instruments[idx];

    // Note 1: A (+9 semitones) - MIDI 36 plays at +9 from root
    inst->keySamples[0].sample = guitarSample;
    inst->keySamples[0].rootNote = 39;  // pitched to compensate for 22.k sample
    inst->keySamples[0].minNote = 36;
    inst->keySamples[0].maxNote = 36;
    inst->keySamples[0].isLoaded = true;

    // Note 2: F (+5 semitones)
    inst->keySamples[1].sample = guitarSample;
    inst->keySamples[1].rootNote = 44;  // pitched to compensate for 22.k sample
    inst->keySamples[1].minNote = 37;
    inst->keySamples[1].maxNote = 37;
    inst->keySamples[1].isLoaded = true;

    // Note 3: C (0 semitones - original pitch)
    inst->keySamples[2].sample = guitarSample;
    inst->keySamples[2].rootNote = 50;  // pitched to compensate for 22.k sample
    inst->keySamples[2].minNote = 38;
    inst->keySamples[2].maxNote = 38;
    inst->keySamples[2].isLoaded = true;

    // Note 4: Bb (-2 semitones)
    inst->keySamples[3].sample = guitarSample;
    inst->keySamples[3].rootNote = 53;  // pitched to compensate for 22.k sample
    inst->keySamples[3].minNote = 39;
    inst->keySamples[3].maxNote = 39;
    inst->keySamples[3].isLoaded = true;

    // Note 5: D (-10 semitones)
    inst->keySamples[4].sample = guitarSample;
    inst->keySamples[4].rootNote = 62;  // pitched to compensate for 22.k sample
    inst->keySamples[4].minNote = 40;
    inst->keySamples[4].maxNote = 40;
    inst->keySamples[4].isLoaded = true;

    inst->numKeySamples = 5;

    // Now load the 3 chime samples normally
    loadKeySample(idx, "instruments/soundmaker01/01SM_6_Bird_Bamboo_chimes.wav", 53, 41, 41);
    loadKeySample(idx, "instruments/soundmaker01/01SM_7_Chimes.wav", 54, 42, 42);
    loadKeySample(idx, "instruments/soundmaker01/01SM_8_Nada_Tropicana_Chime.wav", 55, 43, 43);

    DEBUGF("Loaded Soundmaker01 with %d samples (1 guitar + 3 chimes)\n", inst->numKeySamples);
}

void loadSoundmaker02() {
    int idx = createInstrument("Soundmaker02");
    if (idx == -1) return;

    // 8 sounds, MIDI 36-43, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/soundmaker02/02SM_1_Alarm.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/soundmaker02/02SM_2_Energy_Ray.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/soundmaker02/02SM_3_High_Signal.wav", 50, 38, 38);
    loadKeySample(idx, "instruments/soundmaker02/02SM_4_Lo_Pulse.wav", 51, 39, 39);
    loadKeySample(idx, "instruments/soundmaker02/02SM_5_Low_Signal.wav", 52, 40, 40);
    loadKeySample(idx, "instruments/soundmaker02/02SM_6_Scales_Up.wav", 53, 41, 41);
    loadKeySample(idx, "instruments/soundmaker02/02SM_7_Slide_Down.wav", 54, 42, 42);
    loadKeySample(idx, "instruments/soundmaker02/02SM_8_Whoosh_Up.wav", 55, 43, 43);

    DEBUGF("Loaded Soundmaker02 with %d samples\n", instruments[idx].numKeySamples);
}

void loadSoundmaker03() {
    int idx = createInstrument("Soundmaker03");
    if (idx == -1) return;

    // 9 sounds, MIDI 36-44, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/soundmaker03/03SM_1_Cow.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/soundmaker03/03SM_2_Hen.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/soundmaker03/03SM_3_Owl_1.wav", 50, 38, 38);
    loadKeySample(idx, "instruments/soundmaker03/03SM_4_Owl_2.wav", 51, 39, 39);
    loadKeySample(idx, "instruments/soundmaker03/03SM_5_Owl_3.wav", 52, 40, 40);
    loadKeySample(idx, "instruments/soundmaker03/03SM_6_Robin.wav", 53, 41, 41);
    loadKeySample(idx, "instruments/soundmaker03/03SM_7_Sheep.wav", 54, 42, 42);
    loadKeySample(idx, "instruments/soundmaker03/03SM_8_SongThrush_1.wav", 55, 43, 43);
    loadKeySample(idx, "instruments/soundmaker03/03SM_9_SongThrush_2.wav", 56, 44, 44);

    DEBUGF("Loaded Soundmaker03 with %d samples\n", instruments[idx].numKeySamples);
}

void loadSoundmaker04() {
    int idx = createInstrument("Soundmaker04");
    if (idx == -1) return;

    // 12 sounds, MIDI 36-47, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/soundmaker04/04SM_1_Whale_Sound_01.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/soundmaker04/04SM_2_Whale_Sound_02.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/soundmaker04/04SM_3_Whale_Sound_03.wav", 50, 38, 38);
    loadKeySample(idx, "instruments/soundmaker04/04SM_4_Whale_Sound_04.wav", 51, 39, 39);
    loadKeySample(idx, "instruments/soundmaker04/04SM_5_Whale_Sound_05.wav", 52, 40, 40);
    loadKeySample(idx, "instruments/soundmaker04/04SM_6_Whale_Sound_06.wav", 53, 41, 41);
    loadKeySample(idx, "instruments/soundmaker04/04SM_7_Whale_Sound_07.wav", 54, 42, 42);
    loadKeySample(idx, "instruments/soundmaker04/04SM_8_Whale_Sound_08.wav", 55, 43, 43);
    loadKeySample(idx, "instruments/soundmaker04/04SM_9_Whale_Sound_09.wav", 56, 44, 44);
    loadKeySample(idx, "instruments/soundmaker04/04SM_10_Whale_Sound_10.wav", 57, 45, 45);
    loadKeySample(idx, "instruments/soundmaker04/04SM_11_Whale_Sound_11.wav", 58, 46, 46);
    loadKeySample(idx, "instruments/soundmaker04/04SM_12_Whale_Sound_12.wav", 59, 47, 47);

    DEBUGF("Loaded Soundmaker04 with %d samples\n", instruments[idx].numKeySamples);
}

void loadSoundmaker05() {
    int idx = createInstrument("Soundmaker05");
    if (idx == -1) return;

    // 2 sounds, MIDI 36-37, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/soundmaker05/05SM_1_Shooting_Star.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/soundmaker05/05SM_2_Cym_Swell.wav", 49, 37, 37);

    DEBUGF("Loaded Soundmaker05 with %d samples\n", instruments[idx].numKeySamples);
}

void loadSoundmaker06() {
    int idx = createInstrument("Soundmaker06");
    if (idx == -1) return;

    // 2 sounds, MIDI 36-37, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/soundmaker06/06SM_1_Bird_Call_1.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/soundmaker06/06SM_2_Bird_Call_2.wav", 49, 37, 37);

    DEBUGF("Loaded Soundmaker06 with %d samples\n", instruments[idx].numKeySamples);
}

void loadSoundmaker07() {
    int idx = createInstrument("Soundmaker07");
    if (idx == -1) return;

    // 8 sounds, MIDI 36-43, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/soundmaker07/07SM_1_note_1.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/soundmaker07/07SM_1_Active.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/soundmaker07/07SM_2_note_2.wav", 50, 38, 38);
    loadKeySample(idx, "instruments/soundmaker07/07SM_2_Cym_Swell.wav", 51, 39, 39);
    loadKeySample(idx, "instruments/soundmaker07/07SM_3_Tom_1.wav", 52, 40, 40);
    loadKeySample(idx, "instruments/soundmaker07/07SM_3_Synth_Drop.wav", 53, 41, 41);
    loadKeySample(idx, "instruments/soundmaker07/07SM_4_Tom_2.wav", 54, 42, 42);
    loadKeySample(idx, "instruments/soundmaker07/07SM_5_Tom_3.wav", 55, 43, 43);

    DEBUGF("Loaded Soundmaker07 with %d samples\n", instruments[idx].numKeySamples);
}

void loadSoundmaker08() {
    int idx = createInstrument("Soundmaker08");
    if (idx == -1) return;

    // 6 sounds, MIDI 36-41, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/soundmaker08/08SM_1_Thunder_01.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/soundmaker08/08SM_2_Thunder_02.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/soundmaker08/08SM_3_Thunder_03.wav", 50, 38, 38);
    loadKeySample(idx, "instruments/soundmaker08/08SM_4_Thunder_04.wav", 51, 39, 39);
    loadKeySample(idx, "instruments/soundmaker08/08SM_5_Thunder_05.wav", 52, 40, 40);
    loadKeySample(idx, "instruments/soundmaker08/08SM_6_Thunder_06.wav", 53, 41, 41);

    DEBUGF("Loaded Soundmaker08 with %d samples\n", instruments[idx].numKeySamples);
}

void loadSoundmaker09() {
    int idx = createInstrument("Soundmaker09");
    if (idx == -1) return;

    // 7 sounds, MIDI 36-42, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/soundmaker09/09SM_1_WindInLeaves.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/soundmaker09/09SM_2_HedgehogSnoring.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/soundmaker09/09SM_3_Bangers.wav", 50, 38, 38);
    loadKeySample(idx, "instruments/soundmaker09/09SM_4_ShriekingFireworks.wav", 51, 39, 39);
    loadKeySample(idx, "instruments/soundmaker09/09SM_5_CatherineWheels.wav", 52, 40, 40);
    loadKeySample(idx, "instruments/soundmaker09/09SM_6_Rockets.wav", 53, 41, 41);
    loadKeySample(idx, "instruments/soundmaker09/09SM_7_Sparklers.wav", 54, 42, 42);

    DEBUGF("Loaded Soundmaker09 with %d samples\n", instruments[idx].numKeySamples);
}

void loadSoundmaker10() {
    int idx = createInstrument("Soundmaker10");
    if (idx == -1) return;

    // 1 sound, MIDI 36, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/soundmaker10/10SM_01_Crowd_cheering.wav", 48, 36, 36);

    DEBUGF("Loaded Soundmaker10 with %d samples\n", instruments[idx].numKeySamples);
}

void loadPatternmaker03() {
    int idx = createInstrument("Patternmaker03");
    if (idx == -1) return;

    // 1 sound, MIDI 36, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/patternmaker03/03PM_1_Clock_Strikes_hour.wav", 48, 36, 36);

    DEBUGF("Loaded Patternmaker03 with %d samples\n", instruments[idx].numKeySamples);
}

void loadPatternmaker04() {
    int idx = createInstrument("Patternmaker04");
    if (idx == -1) return;

    // 3 sounds, MIDI 36-38, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/patternmaker04/04PM_1_Sonar_1.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/patternmaker04/04PM_2_Sonar_2.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/patternmaker04/04PM_3_Sonar_3.wav", 50, 38, 38);

    DEBUGF("Loaded Patternmaker04 with %d samples\n", instruments[idx].numKeySamples);
}

void loadPatternmaker05() {
    int idx = createInstrument("Patternmaker05");
    if (idx == -1) return;

    // 7 sounds, MIDI 36-42, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/patternmaker05/05PM_1_Dings_1.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/patternmaker05/05PM_2_Dings_2.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/patternmaker05/05PM_3_Dings_3.wav", 50, 38, 38);
    loadKeySample(idx, "instruments/patternmaker05/05PM_4_Dings_4.wav", 51, 39, 39);
    loadKeySample(idx, "instruments/patternmaker05/05PM_5_Dings_5.wav", 52, 40, 40);
    loadKeySample(idx, "instruments/patternmaker05/05PM_6_Dings_6.wav", 53, 41, 41);
    loadKeySample(idx, "instruments/patternmaker05/05PM_7_Dings_7.wav", 54, 42, 42);

    DEBUGF("Loaded Patternmaker05 with %d samples\n", instruments[idx].numKeySamples);
}

void loadPatternmaker06() {
    int idx = createInstrument("Patternmaker06");
    if (idx == -1) return;

    // 2 sounds, MIDI 36-37, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/patternmaker06/06PM_1_Clave_hit.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/patternmaker06/06PM_2_Hit.wav", 49, 37, 37);

    DEBUGF("Loaded Patternmaker06 with %d samples\n", instruments[idx].numKeySamples);
}

void loadPatternmaker08() {
    int idx = createInstrument("Patternmaker08");
    if (idx == -1) return;

    // 6 sounds, MIDI 36-41, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/patternmaker08/08PM_01_Water_Drop_01.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/patternmaker08/08PM_02_Water_Drop_02.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/patternmaker08/08PM_03_Water_Drop_03.wav", 50, 38, 38);
    loadKeySample(idx, "instruments/patternmaker08/08PM_04_Water_Drop_04.wav", 51, 39, 39);
    loadKeySample(idx, "instruments/patternmaker08/08PM_05_Water_Drop_05.wav", 52, 40, 40);
    loadKeySample(idx, "instruments/patternmaker08/08PM_06_Water_Drop_06.wav", 53, 41, 41);

    DEBUGF("Loaded Patternmaker08 with %d samples\n", instruments[idx].numKeySamples);
}

void loadPatternmaker09() {
    int idx = createInstrument("Patternmaker09");
    if (idx == -1) return;

    // 3 sounds, MIDI 36-38, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/patternmaker09/09PM_01_SwallowChirp.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/patternmaker09/09PM_02_Drum.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/patternmaker09/09PM_03_WoodBlock.wav", 50, 38, 38);

    DEBUGF("Loaded Patternmaker09 with %d samples\n", instruments[idx].numKeySamples);
}

void loadPatternmaker10() {
    int idx = createInstrument("Patternmaker10");
    if (idx == -1) return;

    // 3 sounds, MIDI 36-38, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/patternmaker10/10PM_01_Snare_drum.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/patternmaker10/10PM_02_Wood_block.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/patternmaker10/10PM_03_Tambourine.wav", 50, 38, 38);

    DEBUGF("Loaded Patternmaker10 with %d samples\n", instruments[idx].numKeySamples);
}

void loadMotifmaker06() {
    int idx = createInstrument("Motifmaker06");
    if (idx == -1) return;

    // 4 sounds, MIDI 36-39, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/motifmaker06/06MM_1_Pan_Pipe_Note_1.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/motifmaker06/06MM_2_Pan_Pipe_Note_2.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/motifmaker06/06MM_3_Pan_Pipe_Note_3.wav", 50, 38, 38);
    loadKeySample(idx, "instruments/motifmaker06/06MM_4_Pan_Pipe_Note_4.wav", 51, 39, 39);

    DEBUGF("Loaded Motifmaker06 with %d samples\n", instruments[idx].numKeySamples);
}

void loadMotifmaker07() {
    int idx = createInstrument("Motifmaker07");
    if (idx == -1) return;

    // 5 sounds, MIDI 36-40, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/motifmaker07/07MM_1_Synth_note_1.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/motifmaker07/07MM_2_Synth_note_2.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/motifmaker07/07MM_3_Synth_note_3.wav", 50, 38, 38);
    loadKeySample(idx, "instruments/motifmaker07/07MM_4_Synth_note_4.wav", 51, 39, 39);
    loadKeySample(idx, "instruments/motifmaker07/07MM_5_Synth_note_5.wav", 52, 40, 40);

    DEBUGF("Loaded Motifmaker07 with %d samples\n", instruments[idx].numKeySamples);
}

void loadMotifmaker08() {
    int idx = createInstrument("Motifmaker08");
    if (idx == -1) return;

    // 2 sounds, MIDI 36-37, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/motifmaker08/08MM_01_Wood_Block.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/motifmaker08/08MM_02_Cowbell.wav", 49, 37, 37);

    DEBUGF("Loaded Motifmaker08 with %d samples\n", instruments[idx].numKeySamples);
}

void loadMotifmaker09() {
    int idx = createInstrument("Motifmaker09");
    if (idx == -1) return;

    // 2 sounds, MIDI 36-37, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/motifmaker09/09MM_01_Flute_A.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/motifmaker09/09MM_02_Flute_Fsharp.wav", 49, 37, 37);

    DEBUGF("Loaded Motifmaker09 with %d samples\n", instruments[idx].numKeySamples);
}

void loadMotifmaker10() {
    int idx = createInstrument("Motifmaker10");
    if (idx == -1) return;

    // 5 sounds, MIDI 36-40, +12 semitones for 22kHz compensation
    loadKeySample(idx, "instruments/motifmaker10/10MM_1_Horn_F.wav", 48, 36, 36);
    loadKeySample(idx, "instruments/motifmaker10/10MM_2_Horn_G.wav", 49, 37, 37);
    loadKeySample(idx, "instruments/motifmaker10/10MM_3_Horn_A.wav", 50, 38, 38);
    loadKeySample(idx, "instruments/motifmaker10/10MM_4_Horn_C.wav", 51, 39, 39);
    loadKeySample(idx, "instruments/motifmaker10/10MM_5_Horn_D.wav", 52, 40, 40);

    DEBUGF("Loaded Motifmaker10 with %d samples\n", instruments[idx].numKeySamples);
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

        // Sound Makers
        case 12: loadSoundmaker01(); break;
        case 13: loadSoundmaker02(); break;
        case 14: loadSoundmaker03(); break;
        case 15: loadSoundmaker04(); break;
        case 16: loadSoundmaker05(); break;
        case 17: loadSoundmaker06(); break;
        case 18: loadSoundmaker07(); break;
        case 19: loadSoundmaker08(); break;
        case 20: loadSoundmaker09(); break;
        case 21: loadSoundmaker10(); break;

        // Pattern Makers
        case 22: loadPatternmaker03(); break;
        case 23: loadPatternmaker04(); break;
        case 24: loadPatternmaker05(); break;
        case 25: loadPatternmaker06(); break;
        case 26: loadPatternmaker08(); break;
        case 27: loadPatternmaker09(); break;
        case 28: loadPatternmaker10(); break;

        // Motif Makers
        case 29: loadMotifmaker06(); break;
        case 30: loadMotifmaker07(); break;
        case 31: loadMotifmaker08(); break;
        case 32: loadMotifmaker09(); break;
        case 33: loadMotifmaker10(); break;

        default:
            DEBUG("Invalid type position");
            return;
    }

    selectInstrument(0);
}
