#!/usr/bin/make -f
include ../tests/makefile

LDFLAGS += -lboost_system

ifneq "$(shell id -u -r)" "0"
PRE :=sudo 
endif
