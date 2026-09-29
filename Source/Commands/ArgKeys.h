#pragma once

#include <juce_core/juce_core.h>

/** The keys of the args Commands receive, shared by every Command file and the
    *Args functions that build them. */
namespace resamper::ArgKeys
{
    inline const juce::Identifier argument ("argument"), attack ("attack"), bpm ("bpm"), bus ("bus"),
                                  busTrackId ("busTrackId"), bypassed ("bypassed"), chain ("chain"), clipId ("clipId"),
                                  continuesGesture ("continuesGesture"), count ("count"), countIn ("countIn"),
                                  deltaPitch ("deltaPitch"), deltaSeconds ("deltaSeconds"), denominator ("denominator"),
                                  depth ("depth"), dest ("dest"), end ("end"), file ("file"), folder ("folder"),
                                  grid ("grid"), hold ("hold"), index ("index"), input ("input"), length ("length"),
                                  lengthBeats ("lengthBeats"), mode ("mode"), muted ("muted"), name ("name"),
                                  noteId ("noteId"), noteIds ("noteIds"), numerator ("numerator"),
                                  parameter ("parameter"), parameterId ("parameterId"), pitch ("pitch"),
                                  plugin ("plugin"), pluginId ("pluginId"), position ("position"), release ("release"),
                                  scene ("scene"), sendId ("sendId"), shape ("shape"), shaperId ("shaperId"), start ("start"),
                                  take ("take"), templateFolder ("template"), thresholdDb ("thresholdDb"),
                                  time ("time"), toTrackId ("toTrackId"), trackId ("trackId"), value ("value"),
                                  velocity ("velocity");
}
