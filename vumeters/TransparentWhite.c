// This file is generated, do not modify


#include <stddef.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include "vumeterdef.h"


// Resources enumeration
enum resources_enum {
    RSRC_NULL,
    RSRC_BG,
    RSRC_needle,
    RSRC_COUNT,
};


static const char* resource_names[] = {
   NULL,
   "background.png",
   "needle.png",
};


static texture_id_t textures[RSRC_COUNT];

enum placements_enum {
    PLCMNT_NULL,
    PLCMNT_BG,
    PLCMNT_N00,
    PLCMNT_N01,
    PLCMNT_N02,
    PLCMNT_N03,
    PLCMNT_N04,
    PLCMNT_N05,
    PLCMNT_N06,
    PLCMNT_N07,
    PLCMNT_N08,
    PLCMNT_N09,
    PLCMNT_N10,
    PLCMNT_N11,
    PLCMNT_N12,
    PLCMNT_N13,
    PLCMNT_N14,
    PLCMNT_N15,
    PLCMNT_N16,
    PLCMNT_N17,
    PLCMNT_N18,
    PLCMNT_N19,
    PLCMNT_N20,
    PLCMNT_N21,
    PLCMNT_N22,
    PLCMNT_N23,
    PLCMNT_N24,
    PLCMNT_N25,
    PLCMNT_N26,
    PLCMNT_N27,
    PLCMNT_N28,
    PLCMNT_N29,
    PLCMNT_N30,
    PLCMNT_N31,
    PLCMNT_N32,
    PLCMNT_N33,
    PLCMNT_N34,
    PLCMNT_N35,
    PLCMNT_N36,
    PLCMNT_N37,
    PLCMNT_N38,
    PLCMNT_N39,
    PLCMNT_N40,
    PLCMNT_N41,
    PLCMNT_N42,
    PLCMNT_N43,
    PLCMNT_N44,
    PLCMNT_N45,
    PLCMNT_N46,
    PLCMNT_N47,
    PLCMNT_N48,
    PLCMNT_COUNT,
};


static vu_placement_t placements[] = {
   { .texture_index=RSRC_NULL, .rect={ 0, 0, 0, 0}, },
   { .texture_index=RSRC_BG, .rect={ 0, 0, 1024, 600}, .flip=0, .angle=0, .center={.x=0, .y=0}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=6.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=9.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=13.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=16.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=20.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=23.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=27.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=30.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=34.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=37.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=41.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=44.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=48.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=51.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=55.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=58.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=62.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=65.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=69.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=72.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=76.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=79.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=83.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=86.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=90.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=93.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=97.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=100.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=104.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=107.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=111.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=114.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=118.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=121.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=125.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=128.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=132.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=135.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=139.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=142.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=146.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=149.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=153.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=156.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=160.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=163.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=167.0, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=170.5, .center={.x=367, .y=21}, },
   { .texture_index=RSRC_needle, .rect={ 145, 529, 389, 43}, .flip=0, .angle=174.0, .center={.x=367, .y=21}, },
};


//Background
// background placements
static const vu_background_t bg_left_tsp_white = {
    .placement_count=1,
    .placements={
        PLCMNT_BG,
    },
};

static const vu_background_t bg_right_tsp_white = {
    .placement_count=1,
    .placements={
        PLCMNT_BG,
    },
};

//Levels
static vu_component_t levels_1_left[] = {
    {
        .render=SINGLE, .peak=DECAY,
        .placements={
            PLCMNT_N00,
            PLCMNT_N01,
            PLCMNT_N02,
            PLCMNT_N03,
            PLCMNT_N04,
            PLCMNT_N05,
            PLCMNT_N06,
            PLCMNT_N07,
            PLCMNT_N08,
            PLCMNT_N09,
            PLCMNT_N10,
            PLCMNT_N11,
            PLCMNT_N12,
            PLCMNT_N13,
            PLCMNT_N14,
            PLCMNT_N15,
            PLCMNT_N16,
            PLCMNT_N17,
            PLCMNT_N18,
            PLCMNT_N19,
            PLCMNT_N20,
            PLCMNT_N21,
            PLCMNT_N22,
            PLCMNT_N23,
            PLCMNT_N24,
            PLCMNT_N25,
            PLCMNT_N26,
            PLCMNT_N27,
            PLCMNT_N28,
            PLCMNT_N29,
            PLCMNT_N30,
            PLCMNT_N31,
            PLCMNT_N32,
            PLCMNT_N33,
            PLCMNT_N34,
            PLCMNT_N35,
            PLCMNT_N36,
            PLCMNT_N37,
            PLCMNT_N38,
            PLCMNT_N39,
            PLCMNT_N40,
            PLCMNT_N41,
            PLCMNT_N42,
            PLCMNT_N43,
            PLCMNT_N44,
            PLCMNT_N45,
            PLCMNT_N46,
            PLCMNT_N47,
            PLCMNT_N48,
        },
    },
};

static vu_channel_t channel_levels_1_left = { .component_count=1,.components=levels_1_left,};


static vu_component_t levels_1_right[] = {
    {
        .render=SINGLE, .peak=DECAY,
        .placements={
            PLCMNT_N00,
            PLCMNT_N01,
            PLCMNT_N02,
            PLCMNT_N03,
            PLCMNT_N04,
            PLCMNT_N05,
            PLCMNT_N06,
            PLCMNT_N07,
            PLCMNT_N08,
            PLCMNT_N09,
            PLCMNT_N10,
            PLCMNT_N11,
            PLCMNT_N12,
            PLCMNT_N13,
            PLCMNT_N14,
            PLCMNT_N15,
            PLCMNT_N16,
            PLCMNT_N17,
            PLCMNT_N18,
            PLCMNT_N19,
            PLCMNT_N20,
            PLCMNT_N21,
            PLCMNT_N22,
            PLCMNT_N23,
            PLCMNT_N24,
            PLCMNT_N25,
            PLCMNT_N26,
            PLCMNT_N27,
            PLCMNT_N28,
            PLCMNT_N29,
            PLCMNT_N30,
            PLCMNT_N31,
            PLCMNT_N32,
            PLCMNT_N33,
            PLCMNT_N34,
            PLCMNT_N35,
            PLCMNT_N36,
            PLCMNT_N37,
            PLCMNT_N38,
            PLCMNT_N39,
            PLCMNT_N40,
            PLCMNT_N41,
            PLCMNT_N42,
            PLCMNT_N43,
            PLCMNT_N44,
            PLCMNT_N45,
            PLCMNT_N46,
            PLCMNT_N47,
            PLCMNT_N48,
        },
    },
};

static vu_channel_t channel_levels_1_right = { .component_count=1,.components=levels_1_right,};


static vumeter_t vumeters[] = {
    {
        .name="Transparent White",
        .backgrounds={ &bg_left_tsp_white, &bg_right_tsp_white, },
        .channels={ &channel_levels_1_left, &channel_levels_1_right, }
    },
};


// VU Meter properties
vumeter_properties_t VuProperties = {
    .name="TransparentWhite",
    .volume_levels=49, .w=2144, .h=600,
    .vumeter_count=1, .vumeters=vumeters,
    .layout={ .w=2144,.h=600,
        .rects={
            { .x=0,  .y=0,  .w=0,  .h=0 }, 
            { .x=32,  .y=0,  .w=1024,  .h=600 }, 
            { .x=1088,  .y=0,  .w=1024,  .h=600 }, 
        },
    },
    .resources={.count=RSRC_COUNT, .names=resource_names, .textures=textures },
    .placements={.count=PLCMNT_COUNT,.elements=placements },
};


