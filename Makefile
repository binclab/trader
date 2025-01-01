all: configure compile

configure:
	meson setup .build

compile:
	meson compile -C .build
