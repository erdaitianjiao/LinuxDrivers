#!/bin/sh

# set catch volume

amixer cset name='Capture Volume' 90,90
# PCM
amixer sset 'PCM Playback' on
amixer sset 'Playback' 256
amixer sset 'Right Output Mixer PCM' on
amixer sset 'Left Output Mixer PCM' on

# ADC PCM
amixer sset 'ADC PCM' 200

# set speaker / headphone volume
# Turn on Headphone
amixer sset 'Headphone Playback ZC' on
# Set the volume of your headphones(98% volume，127 is the MaxVolume)
amixer sset Headphone 125,125
# Turn on the speaker
amixer sset 'Speaker Playback ZC' on
# Set the volume of your Speaker(98% volume，127 is the MaxVolume)
amixer sset Speaker 125,125
# Set the volume of your Speaker AC(80% volume，100 is the MaxVolume)
amixer sset 'Speaker AC' 4
# Set the volume of your Speaker AC(80% volume，5 is the MaxVolume)
amixer sset 'Speaker DC' 4

# voice in left
# Turn on Left Input Mixer Boost
amixer sset 'Left Input Mixer Boost' off
amixer sset 'Left Boost Mixer LINPUT1' off
amixer sset 'Left Input Boost Mixer LINPUT1' 0
amixer sset 'Left Boost Mixer LINPUT2' off
amixer sset 'Left Input Boost Mixer LINPUT2' 0
# Turn off Left Boost Mixer LINPUT3
amixer sset 'Left Boost Mixer LINPUT3' off
amixer sset 'Left Input Boost Mixer LINPUT3' 0

# voice in right all off
# Turn on Right Input Mixer Boost
amixer sset 'Right Input Mixer Boost' on
amixer sset 'Right Boost Mixer RINPUT1' off
amixer sset 'Right Input Boost Mixer RINPUT2' 0
amixer sset 'Right Boost Mixer RINPUT2' on
amixer sset 'Right Input Boost Mixer RINPUT2' 127
amixer sset 'Right Boost Mixer RINPUT3' off
amixer sset 'Right Input Boost Mixer RINPUT3' 0