#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tonedef.h"

static bool test_get_semitone(char *str, enum semitone_t expected);

int main(int argc, const char *argv[]) {

	struct note exact_note;
	struct note approx_note;
	struct note test_note;
	char *semitone;

	exact_note = get_exact_note(440.0);
	if (exact_note.semitone != A || exact_note.octave != 4 || exact_note.cents != 0.0) {
		fprintf(
			stderr,
			"expected: %s %d %f, actual: %s %d %f\n",
			"A",
			4,
			0.0,
			get_semitone_str(exact_note.semitone),
			exact_note.octave,
			exact_note.cents);
		return 1;
	}

	approx_note = get_approx_note(15.0);
	if (approx_note.semitone != UNKNOWN_SEMITONE || approx_note.octave != -1 || approx_note.cents != -255.0) {
		fprintf(
			stderr,
			"expected: %d %d %f, actual: %d %d %f\n",
			UNKNOWN_SEMITONE,
			-1,
			-255.0,
			approx_note.semitone,
			approx_note.octave,
			approx_note.cents);
		return 1;
	}

	printf("test invalid semitone\n");
	if (UNKNOWN_SEMITONE != get_semitone(NULL)) {
		return 1;
	}

	if (!test_get_semitone("foo", UNKNOWN_SEMITONE)) {
		return 1;
	}

	if (!test_get_semitone("c", C)) {
		return 1;
	}

	if (!test_get_semitone("c#", Db)) {
		return 1;
	}

	if (!test_get_semitone("d", D)) {
		return 1;
	}

	if (!test_get_semitone("eb", Eb)) {
		return 1;
	}

	if (!test_get_semitone("e", E)) {
		return 1;
	}

	if (!test_get_semitone("f", F)) {
		return 1;
	}

	if (!test_get_semitone("f#", Gb)) {
		return 1;
	}

	if (!test_get_semitone("g", G)) {
		return 1;
	}

	if (!test_get_semitone("ab", Ab)) {
		return 1;
	}

	if (!test_get_semitone("a", A)) {
		return 1;
	}

	if (!test_get_semitone("a#", Bb)) {
		return 1;
	}

	if (!test_get_semitone("b", B)) {
		return 1;
	}

	for (int i = 0; i < 13; ++i) {

		printf("test get_semitone_str for semitone %d - %s\n", i, get_semitone_str(i));

		if (0 == i && 0 != strcmp(get_semitone_str(i), "C")) {
			return 1;
		} else if (1 == i && 0 != strcmp(get_semitone_str(i), "Db")) {
			return 1;
		} else if (2 == i && 0 != strcmp(get_semitone_str(i), "D")) {
			return 1;
		} else if (3 == i && 0 != strcmp(get_semitone_str(i), "Eb")) {
			return 1;
		} else if (4 == i && 0 != strcmp(get_semitone_str(i), "E")) {
			return 1;
		} else if (5 == i && 0 != strcmp(get_semitone_str(i), "F")) {
			return 1;
		} else if (6 == i && 0 != strcmp(get_semitone_str(i), "Gb")) {
			return 1;
		} else if (7 == i && 0 != strcmp(get_semitone_str(i), "G")) {
			return 1;
		} else if (8 == i && 0 != strcmp(get_semitone_str(i), "Ab")) {
			return 1;
		} else if (9 == i && 0 != strcmp(get_semitone_str(i), "A")) {
			return 1;
		} else if (10 == i && 0 != strcmp(get_semitone_str(i), "Bb")) {
			return 1;
		} else if (11 == i && 0 != strcmp(get_semitone_str(i), "B")) {
			return 1;
		} else if (12 == i && NULL != get_semitone_str(i)) {
			return 1;
		}
	}

	test_note.semitone = UNKNOWN_SEMITONE;
	test_note.octave = 4;
	test_note.cents = 0;
	if (-1 != get_freq(&test_note)) {
		return 1;
	}

	test_note.semitone = B;
	test_note.octave = OCTAVE_MAX + 5;
	if (-1 != get_freq(&test_note)) {
		return 1;
	}

	test_note.octave = 3;
	test_note.cents = SEMITONE_INTERVAL_CENTS;
	if (-1 != get_freq(&test_note)) {
		return 1;
	}

	test_note = get_exact_note(5.0);
	if (UNKNOWN_SEMITONE != test_note.semitone || -1 != test_note.octave || -255.0 != test_note.cents) {
		return 1;
	}

	return 0;
}

static bool test_get_semitone(char *str, enum semitone_t expected)
{
	char semitone[5];

	printf("test %s\n", str);
        strcpy(semitone, str);
        if (expected != get_semitone(semitone)) {
                return false;
        }

	return true;
}
