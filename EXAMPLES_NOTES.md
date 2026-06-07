some notes about the examples

composites/graph
not very pretty, can't we have a more classic/standard graph? something more compelling

composites/polygon
this is ugly can we have something cleaner and more regular without overlapping polygons
like a regular polygon, possibly with a hole, and something like a star or something
also could we show the underlying triangulation? how would that work?

features/annotation_readout
on macOS the text is way too small and far from the point (too much on the bottom right compared to the point)

features/axis_labels
wtf are all these segments? some axis aligned other tilted. seems very wrong and I don't understand what that is supposed to represent

features/colormap_scale
i'm a bit confused as to what this is supposed to illustrate mostly because the key feature seems to be hidden behind a helper no?

features/controller_fly
the left-right drag is very weird and possibly broken

features/legend_categorical
in the legend the gray square is bigger than the other squares

features/lighting
not sure if this is a good illustration of the lighting system, shouldn't we show various types of lights/materials/settings etc? wdyt?

features/marker_symbols
shouldn't the different types of symbols show clearly which is using which technique? for ex for bitmap using a clearly bitmap symbol with, perhaps, colors, that could not be easily made with other techniques (look at v0.3 marker visual example where we had used an actual red pin bitmap)

features/overlay_card
for that one i think i would prefer a slightly more realistic example that shows an actual use case of the overlay rather than an isolated overlay with nothing else, while keeping the spirit "1 feature = 1 example", what would you propose?

features/panel_background
i don't think this is working, i do not see a background

features/panzoom_attachment
why not just "panzoom"

features/pick*
these are mostly broken (notably y flip again, you should really add tight regressions for that), i would like a single unified picking example with 1 or several visual families where hovering changes the size, and selecting results in some visual change like color or border or something

features/selection
shouldn't that be merged with the new picking example? wdyt?

features/video_export
this one should run for a couple of seconds and save a video file and print on the terminal the path to the video file no?

features/visibility
we must add a comment that we use different visuals just for demo purposes only but in reality it is better to group visuals of the same family together to benefit from batching, however visibility applies to the level of an entire visual




showcases/brain_volume_mesh
this one should be called just "brain_volume"
also the initial view should be tilted in such a way that we see the slice and some of the brain volume behind

showcases/choropleth
title and subtitle are way too small on macOS retina

showcases/scientific_plotting
xaxis is clipped by panel bottom end

showcases/textured_planet
the dark side of the planet is way too dark we don't see anything
also i think we should put some limits to zoom in/out with this view, and perhaps some kind of log scale because the more we zoom the slower we want to go or something? right now on macOS the double finger zoom is way to sensitive

showcases/wind_field
the ellipsoid pointer is weird, what is it?




visuals/glyph
i don't understand this one, compared to marker? shouldn't these be font glyphs? what's the difference with marker?

visuals/segment
shouldn't we show different types of segment ends?
