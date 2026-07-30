#!/usr/bin/env bash
#
# Makes sure JUCE is present before anything calls cmake. Sourced by the other
# scripts rather than copied into them, so a fresh clone behaves the same whichever
# one you happen to run first.
#
# JUCE/ is gitignored, so a fresh clone has nothing there at all and
# add_subdirectory(JUCE) would abort configuration.

zs_fetch_juce() {
    # The directory may be a symlink to a sibling checkout to save a gigabyte on
    # disk; if that checkout has since moved, the link is dangling and git would
    # refuse to clone over it.
    if [[ -L JUCE && ! -e JUCE ]]; then
        echo "the JUCE symlink is dangling — replacing it with a fresh checkout"
        rm JUCE
    fi

    if [[ ! -d JUCE ]]; then
        echo "JUCE is missing — fetching it..."
        git clone --depth 1 --branch master https://github.com/juce-framework/JUCE.git JUCE
    fi
}
