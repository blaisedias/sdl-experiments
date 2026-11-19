#
# Copyright (C) 2025-2026  Blaise Dias
# 
# This file is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
# 
# It is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this file.  If not, see <http://www.gnu.org/licenses/>.
#

TARG_ARCH=$(shell uname -m)
TARG_CF =
TARG_DEFS =
LIBDIRS =
SDL2_CFLAGS := $(shell sdl2-config --cflags)
SDL2_LIBS := $(shell sdl2-config --libs)

PLATFORM=$(shell ./scripts/ispcp.sh)
ifeq ($(PLATFORM),piCorePlayer)
	INCLUDE_TSLIB = -I ./tcz-libts/libts-1.0/src
	TS_LIB_H = ./tcz-libts/libts-1.0/src/tslib.h
	TARG_DEFS = -D USE_TSLIB_FOR_TOUCH
else
	INCLUDE_TSLIB =
	TS_LIB_H = 
endif

DEFS = $(TARG_DEFS)
#DEFS = -D_TIME_BITS=64 -D_FILE_OFFSET_BITS=64 $(TARG_DEFS)
#		-DVOLUME_CALIB_LEVEL=1

SRC = src
GENERATED = generated
JSON-PARSER-SRC = json-parser
CITYHASH-SRC = cityhash-c
INCLUDES-JSON-PARSER = -I $(JSON-PARSER-SRC)
INCLUDES-CITYHASH = -I $(CITYHASH-SRC)
INCLUDES-GENERATED = -I $(GENERATED)
INCLUDES = -I $(SRC) $(INCLUDES-GENERATED) $(INCLUDES-JSON-PARSER) $(INCLUDES-CITYHASH) $(INCLUDE_TSLIB)

BIN_DIR = bin
OBJ_DIR = obj
DEP_DIR = dep
LIB_DIR = lib
LIBS = -lm $(SDL2_LIBS) -lSDL2_image -lSDL2_ttf -lts 

SANITIZE = 
#SANITIZE =  -fsanitize=safe-stack
#SANITIZE =  -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
#SANITIZE =  -fsanitize=memory -fno-omit-frame-pointer -fsanitize=undefined
#SANITIZE =   -fsanitize=undefined -fsanitize=address 
GLOBAL_DEPS = ./Makefile

# debug builds
CF_OPTIM = -fno-omit-frame-pointer -g -Og 
# release builds
#CF_OPTIM = -O2 

CF_WARNINGS_COMMON = -Wall -Wextra \
					 -Wpedantic \
					 -Wuninitialized \
					 -Wmissing-declarations \
					 -Wshadow \
					 -Wcast-align \
					 -Wswitch-default \
					 -Wpointer-arith \
					 -Wwrite-strings \
					 -Wundef \
					 -Wunreachable-code \
					 -Wstrict-prototypes \
					 -Werror



# commented out for pCP compiler version (14.x) which emits warnings without highlight "offending" code.
#					 -Wstrict-overflow=5 \
# commented out - too many warnings
#					 -Wconversion \

CF = $(CF_WARNINGS_COMMON) \
		-Wswitch-enum \
		-Wconversion \
		$(CF_OPTIM) $(TARG_CF) $(DEFS) $(SANITIZE) $(SDL2_CFLAGS) $(USER_CFLAGS) \

CF_JSON_PARSER = $(CF_WARNINGS_COMMON) \
		$(CF_OPTIM) $(TARG_CF) $(DEFS) $(SANITIZE) $(SDL2_CFLAGS) $(USER_CFLAGS)

CF_CITY_HASH = $(CF_WARNINGS_COMMON) \
		-Wswitch-enum \
		$(CF_OPTIM) $(TARG_CF) $(DEFS) $(SANITIZE) $(SDL2_CFLAGS) $(USER_CFLAGS)

CCP = g++
CC = gcc
CF_LIBS = $(CF) -fpic -shared

all: $(BIN_DIR)/jl2

JL2_OBJS =  \
		  $(OBJ_DIR)/application.o \
		  $(OBJ_DIR)/logging.o \
		  $(OBJ_DIR)/timing.o \
		  $(OBJ_DIR)/util.o \
		  $(OBJ_DIR)/event_queue.o \
		  $(OBJ_DIR)/touch_screen.o \
		  $(OBJ_DIR)/touch_screen_sdl2.o \
		  $(OBJ_DIR)/platform_linux.o \
		  $(OBJ_DIR)/texture_cache.o $(OBJ_DIR)/city.o \
		  $(OBJ_DIR)/lyrion_player.o \
		  $(OBJ_DIR)/timedate.o \
		  $(OBJ_DIR)/widgets.o \
		  $(OBJ_DIR)/actions.o \
		  $(OBJ_DIR)/json.o \
		  $(OBJ_DIR)/widgets_json.o \
		  $(OBJ_DIR)/visualizer.o \
		  $(OBJ_DIR)/vumeter.o \
		  $(OBJ_DIR)/vumeter_json.o \
		  $(OBJ_DIR)/vis_vumeter.o \
		  $(OBJ_DIR)/slider_widget.o \
		  $(OBJ_DIR)/vumeter_widget.o \
		  $(OBJ_DIR)/vumeter_enum.o \

$(BIN_DIR)/jl2 : $(OBJ_DIR)/jl2.o $(JL2_OBJS) | $(BIN_DIR)
	$(CC) $(CF) $(INCLUDES-CITYHASH)  -o $(@) $^ $(LIBDIRS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJ_DIR)/*
	rm -f $(BIN_DIR)/*
	rm -f $(LIB_DIR)/*
	rm -f $(DEP_DIR)/*
	rm -f $(GENERATED)/*

generated: $(GENERATED)/vumeter_enum.c $(GENERATED)/vumeter_enum.h

#== { Dependencies =============== 
sources := $(wildcard $(SRC)/*.c)
sources_json_parser := $(wildcard $(JSON-PARSER-SRC)/*.c)
sources_city_hash := $(wildcard $(CITYHASH-SRC)/*.c)
objs := $(sources:$(SRC)/%.c=$(OBJ_DIR)/%.o)
objs_json_parser := $(sources_json_parser:$(JSON-PARSER-SRC)/%.c=$(OBJ_DIR)/%.o)
objs_cityhash := $(sources_cityhash:$(CITYHASH-SRC)/%.c=$(OBJ_DIR)/%.o)
deps := $(sources:$(SRC)/%.c=$(DEP_DIR)/%.d)
deps_json_parser := $(sources_json_parser:$(JSON-PARSER-SRC)/%.c=$(DEP_DIR)/%.d)
deps_cityhash := $(sources_cityhash:$(CITYHASH-SRC)/%.c=$(DEP_DIR)/%.d)

SOURCES := $(sources) $(sources_json_parser) $(sources_cityhash)
OBJS := $(objs) $(objs_json_parser) $(objs_cityhash)
DEPS := $(deps) $(deps_json_parser) $(deps_cityhash)

$(DEP_DIR)/%.d: $(SRC)/%.c | $(DEP_DIR) generated 
	@set -e; rm -f $@; \
	$(CC) -MM $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OBJ_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(DEP_DIR)/%.d: $(CITYHASH-SRC)/%.c | $(DEP_DIR)
	@set -e; rm -f $@; \
	$(CC) -MM $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OBJ_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(DEP_DIR)/%.d: $(JSON-PARSER-SRC)/%.c | $(DEP_DIR)
	@set -e; rm -f $@; \
	$(CC) -MM $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OBJ_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

include $(DEPS)
#== }

$(OBJ_DIR)/%.o: $(SRC)/%.cpp | $(OBJ_DIR)
	$(CCP) $(CF) -c -o $(@) $< $(INCLUDES)

$(OBJ_DIR)/%.o: $(SRC)/%.c | $(OBJ_DIR)
	$(CC) $(CF) -c -o $(@) $< $(INCLUDES)

$(OBJ_DIR)/%.o: $(JSON-PARSER-SRC)/%.cpp | $(OBJ_DIR)
	$(CCP) $(CF_JSON_PARSER) -c -o $(@) $< $(INCLUDES-JSON-PARSER)

$(OBJ_DIR)/%.o: $(JSON-PARSER-SRC)/%.c | $(OBJ_DIR)
	$(CC) $(CF_JSON_PARSER) -c -o $(@) $< $(INCLUDES-JSON-PARSER)

$(OBJ_DIR)/%.o: $(CITYHASH-SRC)/%.cpp | $(OBJ_DIR)
	$(CCP) $(CF_CITY_HASH) -c -o $(@) $< $(INCLUDES-CITYHASH)

$(OBJ_DIR)/%.o: $(CITYHASH-SRC)/%.c | $(OBJ_DIR)
	$(CC) $(CF_CITY_HASH) -DHAVE_BUILTIN_EXPECT=0 -c -o $(@) $< $(INCLUDES-CITYHASH)

$(OBJ_DIR)/%.o: $(GENERATED)/%.c | $(OBJ_DIR)
	$(CC) $(CF) -c -o $(@) $< $(INCLUDES)

$(LIB_DIR)/%.so: $(OBJ_DIR)/%.o | $(LIB_DIR)
	$(CC) $(CF_LIBS) $(INCLUDES) $< -o $@

# generated code
$(GENERATED)/vumeter_enum.c: src/vumeter_enums.json
	./scripts/generate_enums_with_strings.py --src src/vumeter_enums.json --destdir generated/ --name vumeter_enum

$(GENERATED)/vumeter_enum.h: src/vumeter_enums.json
	./scripts/generate_enums_with_strings.py --src src/vumeter_enums.json --destdir generated/ --name vumeter_enum

## { test executables ===================
# # 1. texture cache
# $(BIN_DIR)/test_tcache : $(OBJ_DIR)/test_tcache.o $(OBJ_DIR)/texture_cache.o $(OBJ_DIR)/logging.o $(OBJ_DIR)/city.o $(OBJ_DIR)/timing.o $(OBJ_DIR)/event_queue.o | $(BIN_DIR)
# 	$(CC) $(CF) -o $(@) $^ $(LIBDIRS) $(LIBS)
# 
# # 2. json parsing
# TEST_WIDGETS_JSON_OBJS =  \
# 	$(OBJ_DIR)/widgets_json.o \
# 	$(OBJ_DIR)/json.o \
# 	$(OBJ_DIR)/logging.o \
# 	$(OBJ_DIR)/widgets.o \
# 	$(OBJ_DIR)/slider_widget.o \
# 	$(OBJ_DIR)/vumeter_widget.o \
# 	$(OBJ_DIR)/actions.o \
# 	$(OBJ_DIR)/util.o \
# 	$(OBJ_DIR)/vumeter.o \
# 	$(OBJ_DIR)/vumeter_json.o \
# 	$(OBJ_DIR)/visualizer.o \
# 	$(OBJ_DIR)/vis_vumeter.o \
# 	$(OBJ_DIR)/city.o $(OBJ_DIR)/texture_cache.o \
# 	$(OBJ_DIR)/timing.o $(OBJ_DIR)/event_queue.o \
# 	$(OBJ_DIR)/lyrion_player.o \
# 	$(OBJ_DIR)/timedate.o \
# 	$(OBJ_DIR)/platform_linux.o
# 
# $(BIN_DIR)/test_widgets_json : $(OBJ_DIR)/test_widgets_json.o $(TEST_WIDGETS_JSON_OBJS) | $(BIN_DIR)
# 	$(CC) $(CF) -o $(@) $^ $(LIBDIRS) $(LIBS)
# 
# # 3. touch screen
# $(BIN_DIR)/test_touch: $(OBJ_DIR)/test_touch.o $(OBJ_DIR)/touch_screen.o $(OBJ_DIR)/timing.o
# 	$(CC) $(CF) -o $(@) $^ $(LIBDIRS) $(LIBS)
# 
# # 4. lyrion player test
# $(BIN_DIR)/local_player_test: $(SRC)/local_player_test.c $(SRC)/lyrion_player.c $(SRC)/logging.c $(SRC)/timing.c
# 	$(CC) $(CF) -fsanitize=address -fsanitize=undefined -fsanitize=null -fsanitize=alignment -fsanitize=float-cast-overflow \
# 		-o $(@) $^ $(LIBDIRS) $(LIBS)

# 5. test vumeter json load
VUMETER_TEST_OBJS = \
	$(OBJ_DIR)/vumeter_json.o \
	$(OBJ_DIR)/json.o \
	$(OBJ_DIR)/vumeter.o \
	$(OBJ_DIR)/logging.o \
	$(OBJ_DIR)/util.o \
	$(OBJ_DIR)/texture_cache.o  $(OBJ_DIR)/city.o \
	$(OBJ_DIR)/timing.o \
	$(OBJ_DIR)/visualizer.o \
	$(OBJ_DIR)/vis_vumeter.o \
	$(OBJ_DIR)/platform_linux.o \


$(BIN_DIR)/test_vumeter_json_load: $(SRC)/test_vumeter_json_load.c $(GENERATED)/vumeter_enum.c $(VUMETER_TEST_OBJS) 
	$(CC) $(CF) -fsanitize=address -fsanitize=undefined -fsanitize=null -fsanitize=alignment -fsanitize=float-cast-overflow \
		-Og -g -o $(@) $^ $(INCLUDES) $(LIBDIRS) $(LIBS)

#test: $(BIN_DIR)/test_tcache $(BIN_DIR)/test_widgets_json $(BIN_DIR)/local_player_test $(BIN_DIR)/test_vumeter_json_load
test: $(BIN_DIR)/test_vumeter_json_load
## } 

## { utilities / tools ===================
$(BIN_DIR)/lyrion_player_strhash: $(SRC)/lyrion_player_strhash.c $(SRC)/lyrion_player.c $(SRC)/logging.c $(SRC)/timing.c
	$(CC) $(CF) -fsanitize=address -fsanitize=undefined -fsanitize=null -fsanitize=alignment -fsanitize=float-cast-overflow \
		-o $(@) $^ $(LIBDIRS) $(LIBS)

util: $(BIN_DIR)/lyrion_player_strhash
## } 

## { sub directories ===================
$(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

$(DEP_DIR):
	mkdir -p $@

## }
