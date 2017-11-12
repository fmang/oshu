/**
 * \file beatmap/tokens.h
 * \ingroup beatmap
 *
 * \brief
 * List the special words in a beatmap.
 *
 * This file is a bit magical as it uses a special `TOKEN` macro defined by the
 * file that includes *tokens.h*.
 *
 * It is an absolute requirement that the tokens are sorted in alphabetical order!
 * This lets us perform a dichotomic search when parsing, which makes lexing
 * orders of magnitude faster.
 */

TOKEN(ApproachRate)
TOKEN(Artist)
TOKEN(ArtistUnicode)
TOKEN(AudioFilename)
TOKEN(AudioLeadIn)
TOKEN(BeatDivisor)
TOKEN(BeatmapID)
TOKEN(BeatmapSetID)
TOKEN(Bookmarks)
TOKEN(CircleSize)
TOKEN(Colours)
TOKEN(Combo)
TOKEN(Countdown)
TOKEN(Creator)
TOKEN(Difficulty)
TOKEN(DistanceSpacing)
TOKEN(Drum)
TOKEN(Editor)
TOKEN(EpilepsyWarning)
TOKEN(Events)
TOKEN(General)
TOKEN(GridSize)
TOKEN(HPDrainRate)
TOKEN(HitObjects)
TOKEN(LetterboxInBreaks)
TOKEN(Metadata)
TOKEN(Mode)
TOKEN(None)
TOKEN(Normal)
TOKEN(OverallDifficulty)
TOKEN(PreviewTime)
TOKEN(SampleSet)
TOKEN(SkinPreference)
TOKEN(SliderBody)
TOKEN(SliderBorder)
TOKEN(SliderMultiplier)
TOKEN(SliderTickRate)
TOKEN(SliderTrackOverride)
TOKEN(Soft)
TOKEN(Source)
TOKEN(StackLeniency)
TOKEN(StoryFireInFront)
TOKEN(Tags)
TOKEN(TimelineZoom)
TOKEN(TimingPoints)
TOKEN(Title)
TOKEN(TitleUnicode)
TOKEN(Version)
TOKEN(WidescreenStoryboard)
