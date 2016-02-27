#include <stdio.h>
#include <stdlib.h>
#include "tonedef.h"

int main(int argc, const char *argv[]) {

	struct note exact_note;
	struct note approx_note;

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

	return 0;
}
