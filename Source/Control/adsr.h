/*
Copyright (c) 2020 Electrosmith, Corp, Paul Batchelor

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/

#pragma once
#ifndef DSY_ADSR_H
#define DSY_ADSR_H

#include <stdint.h>
#ifdef __cplusplus

namespace daisysp
{
/** Distinct stages that the phase of the envelope can be located in.
- IDLE   = located at phase location 0, and not currently running
- ATTACK  = First segment of envelope where phase moves from 0 to 1
- DECAY   = Second segment of envelope where phase moves from 1 to SUSTAIN value
- RELEASE =     Fourth segment of envelop where phase moves from SUSTAIN to 0
*/
enum
{
    ADSR_SEG_IDLE    = 0,
    ADSR_SEG_ATTACK  = 1,
    ADSR_SEG_DECAY   = 2,
    ADSR_SEG_RELEASE = 4
};


/** adsr envelope module

Original author(s) : Paul Batchelor

Ported from Soundpipe by Ben Sergentanis, May 2020
 
Remake by Steffan DIedrichsen, May 2021
*/
class Adsr
{
  public:
    Adsr() {}
    ~Adsr() {}
    /** Initializes the Adsr module.
        \param sample_rate - The sample rate of the audio engine being run. 
    */
    void Init(float sample_rate, int blockSize = 1);
    /**
     \function Retrigger forces the envelope back to attack phase
     \param hard  resets the history to zero, results in a click.
     */
    void Retrigger(bool hard);
    /**
     \function ForceRelease forces the envelope directly into the
     release phase from wherever its own level currently sits, bypassing
     Process()'s own gate-edge detection -- same reasoning Retrigger()
     already has for forcing attack: a caller that explicitly knows a
     note just ended (not just inferring it from a sampled gate boolean)
     needs a way to say so directly. Confirmed as a real, reproducible
     bug without this: if a NoteOn is immediately followed by a NoteOff
     before Process() is ever called with gate=true in between (a real
     risk with a fast arpeggiator's short note durations, or just two
     control-rate MIDI events landing in the same inter-audio-block
     gap), Process()'s own `if(!gate && gate_)` falling-edge check can
     never fire -- gate_ (this envelope's own last-seen gate) never
     recorded the rising edge either, so it stays false throughout,
     and the envelope keeps climbing through attack into decay and
     holds at its sustain level forever (decay's own target is a
     positive sustain level, so its own `out < 0.f` idle check never
     fires) -- a genuinely stuck note, with nothing externally wrong at
     all (the caller's own held-note tracking is already correctly
     cleared; the problem is purely this envelope's own internal gate_
     memory never having observed the moment it needed to).
     */
    void ForceRelease();
    /** Processes one sample through the filter and returns one sample.
        \param gate - trigger the envelope, hold it to sustain 
    */
    float Process(bool gate);
    /** Sets time
        Set time per segment in seconds
    */
    void SetTime(int seg, float time);
    void SetAttackTime(float timeInS, float shape = 0.0f);
    void SetDecayTime(float timeInS);
    void SetReleaseTime(float timeInS);

  private:
    void SetTimeConstant(float timeInS, float& time, float& coeff);

  public:
    /** Sustain level
        \param sus_level - sets sustain level, 0...1.0
    */
    inline void SetSustainLevel(float sus_level)
    {
        sus_level = (sus_level <= 0.f) ? -0.01f // forces envelope into idle
                                       : (sus_level > 1.f) ? 1.f : sus_level;
        sus_level_ = sus_level;
    }
    /** get the current envelope segment
        \return the segment of the envelope that the phase is currently located in.
    */
    inline uint8_t GetCurrentSegment() { return mode_; }
    /** Tells whether envelope is active
        \return true if the envelope is currently in any stage apart from idle.
    */
    inline bool IsRunning() const { return mode_ != ADSR_SEG_IDLE; }

  private:
    float   sus_level_{0.f};
    float   x_{0.f};
    float   attackShape_{-1.f};
    float   attackTarget_{0.0f};
    float   attackTime_{-1.0f};
    float   decayTime_{-1.0f};
    float   releaseTime_{-1.0f};
    float   attackD0_{0.f};
    float   decayD0_{0.f};
    float   releaseD0_{0.f};
    int     sample_rate_;
    uint8_t mode_{ADSR_SEG_IDLE};
    bool    gate_{false};
};
} // namespace daisysp
#endif
#endif
