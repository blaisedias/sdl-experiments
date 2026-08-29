#!/usr/bin/env bash
./makemetajson2FRBS.py --src PurpleTastic/ --out PurpleTastic/ --name PurpleTastic
./makemetaindexedjson3.py --src PurpleTastic/ --out PurpleTastic/

./makemetajson2FRBS.py --src TubeD/ --out TubeD/ --name TubeD
./makemetaindexedjson3.py --src TubeD/ --out TubeD/

./makemetajson2FRBS.py --src TransparentWhite/ --out TransparentWhite/ --name TransparentWhite
./makemetaindexedjson3.py --src TransparentWhite/ --out TransparentWhite/

./makemetajson2FRBS.py --src SpeakerGreen/ --out SpeakerGreen/ --name SpeakerGreen
./makemetaindexedjson3.py --src SpeakerGreen/ --out SpeakerGreen/

./makemetajson2FRBS.py --src SpeakerGray/ --out SpeakerGray/ --name SpeakerGray
./makemetaindexedjson3.py --src SpeakerGray/ --out SpeakerGray/

./makemetajson2FRBS.py --src Chevrons/ --out Chevrons/ --name Chevrons
./makemetaindexedjson3.py --src Chevrons/ --out Chevrons/

./makemetajson2FRBS.py --src Kolossos/ --out Kolossos/ --name Kolossos
./makemetaindexedjson3.py --src Kolossos/ --out Kolossos/

