#!/usr/bin/make -f

LDFLAGS += -lboost_system
# XXX: can be removed when we delete deprecated DNS logger methods
CXXFLAGS += -Wno-deprecated-declarations

ifneq "$(shell id -u -r)" "0"
@echo "Note: This test will require elevated privileges."
PRE := sudo
endif

runtest-path := ../tests

# Overriding which files we use for One Definition Rule test
odr_library_header_files = $(extended_functionality_header_files)

include ../tests/makefile
