samplesdir = $(pkgdatadir)/samples
dist_samples_DATA = \
	samples/README.md \
	samples/soft-hitclap.wav \
	samples/soft-sliderslide.wav \
	samples/soft-hitfinish.wav \
	samples/drum-hitclap.wav \
	samples/normal-sliderslide.wav \
	samples/normal-hitclap.wav \
	samples/drum-hitnormal.wav \
	samples/drum-hitwhistle.wav \
	samples/normal-hitfinish.wav \
	samples/drum-sliderslide.wav \
	samples/normal-hitnormal.wav \
	samples/normal-sliderwhistle.wav \
	samples/soft-slidertick.wav \
	samples/soft-hitnormal.wav \
	samples/soft-sliderwhistle.wav \
	samples/drum-slidertick.wav \
	samples/normal-hitwhistle.wav \
	samples/soft-hitwhistle.wav \
	samples/drum-hitfinish.wav \
	samples/drum-sliderwhistle.wav \
	samples/normal-slidertick.wav

samples/README.md: samples-v1.tar.bz2
	tar xf samples-v1.tar.bz2

samples-v1.tar.bz2:
	wget https://www.mg0.fr/oshu/samples-v1.tar.bz2
	@sha256sum --check - <<< '3a7c47d0be9e820d2384adb77a0edc3ddd22b9a0689efe9c090f69246e0ea623  samples-v1.tar.bz2' || echo -e "\nWARNING:\nThe SHA-256 sum of the sample archive is not correct. This means it has been modified after the current code release. Use it at your own risk!\n"

EXTRA_DIST = CREDITS
