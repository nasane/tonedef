#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tonedef.h"

static bool test_get_semitone(char *str, enum semitone_t expected);

int main(int argc, const char *argv[]) {

	struct note exact_note;
	struct note approx_note;
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
