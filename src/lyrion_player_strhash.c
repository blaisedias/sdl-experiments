#include <stdio.h>
#include "lyrion_player.h"

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        printf("    %s = 0x%09lx,\n", argv[i], compute_player_hash(argv[i]));
    }
}
