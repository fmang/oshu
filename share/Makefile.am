samplesdir = $(pkgdatadir)/samples
dist_samples_DATA = \
	samples/README.md \
	samples/drum-hitclap.wav \
	samples/drum-hitfinish.wav \
	samples/drum-hitnormal.wav \
	samples/drum-hitwhistle.wav \
	samples/drum-sliderslide.wav \
	samples/drum-slidertick.wav \
	samples/drum-sliderwhistle.wav \
	samples/normal-hitclap.wav \
	samples/normal-hitfinish.wav \
	samples/normal-hitnormal.wav \
	samples/normal-hitwhistle.wav \
	samples/normal-sliderslide.wav \
	samples/normal-slidertick.wav \
	samples/normal-sliderwhistle.wav \
	samples/soft-hitclap.wav \
	samples/soft-hitfinish.wav \
	samples/soft-hitnormal.wav \
	samples/soft-hitwhistle.wav \
	samples/soft-sliderslide.wav \
	samples/soft-slidertick.wav \
	samples/soft-sliderwhistle.wav

define CHECK_ERROR

ERROR:

The footprint of the downloaded samples archive is not what we expected.
For security reasons, we won't extract it. Extracting it could for instance
overwrite this Makefile and make you run arbitrary code.

You can either:

1. Update your version of oshu. A bug like this one won't remain unfixed for long.
2. Delete the archive if you think the file got corrupted, and run make to download it again.
3. Extract the archive yourself if you believe it's correct, at your own risks.
4. Look into this Makefile to see the files oshu needs, and copy them yourself.

endef
export CHECK_ERROR

samples/README.md:
	[ -e samples-v1.tar.bz2 ] || wget -c https://www.mg0.fr/oshu/samples-v1.tar.bz2
	if ! sha256sum --check <<< '3a7c47d0be9e820d2384adb77a0edc3ddd22b9a0689efe9c090f69246e0ea623  samples-v1.tar.bz2' ; then \
		echo "$$CHECK_ERROR" ; \
		false ; \
	fi
	tar xf samples-v1.tar.bz2
