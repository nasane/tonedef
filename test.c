#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tonedef.h"

int main(int argc, const char *argv[]) {

	struct note test_note;
	char *semitone;
	enum semitone_t *scale;
	enum semitone_t c_major_scale[] = {C, D, E, F, G, A, B, UNKNOWN_SEMITONE};
	enum semitone_t gb_major_scale[] = {Gb, Ab, Bb, B, Db, Eb, F, UNKNOWN_SEMITONE};
	enum semitone_t b_major_scale[] = {B, Db, Eb, E, Gb, Ab, Bb, UNKNOWN_SEMITONE};
	enum semitone_t c_natural_minor_scale[] = {C, D, Eb, F, G, Ab, Bb, UNKNOWN_SEMITONE};
	enum semitone_t g_natural_minor_scale[] = {G, A, Bb, C, D, Eb, F, UNKNOWN_SEMITONE};
	enum semitone_t d_natural_minor_scale[] = {D, E, F, G, A, Bb, C, UNKNOWN_SEMITONE};
	enum semitone_t e_chromatic_scale[] = {E, F, Gb, G, Ab, A, Bb, B, C, Db, D, Eb, UNKNOWN_SEMITONE};

	test_note = get_exact_note(440.0);
	if (test_note.semitone != A || test_note.octave != 4 || test_note.cents != 0.0) {
		fprintf(
			stderr,
			"expected: %s %d %f, actual: %s %d %f\n",
			"A",
			4,
			0.0,
			get_semitone_str(test_note.semitone, false),
			test_note.octave,
			test_note.cents);
		return 1;
	}

	test_note = get_exact_note(45.21);
	if (test_note.semitone != Gb || test_note.octave != 1 || fabs(test_note.cents - -39.347641) > 0.00001) {
		fprintf(
			stderr,
			"expected: %s %d %f, actual: %s %d %f\n",
			"F#",
			1,
			-39.347641,
			get_semitone_str(test_note.semitone, false),
			test_note.octave,
			test_note.cents);
		return 1;
	}

	test_note = get_exact_note(2016.15);
	if (test_note.semitone != B || test_note.octave != 6 || fabs(test_note.cents - 35.233059) > 0.00001) {
		fprintf(
			stderr,
			"expected: %s %d %f, actual: %s %d %f\n",
			"B",
			6,
			35.233059,
			get_semitone_str(test_note.semitone, false),
			test_note.octave,
			test_note.cents);
		return 1;
	}

	test_note = get_exact_note(207.562);
	if (test_note.semitone != Ab || test_note.octave != 3 || fabs(test_note.cents - -0.753418) > 0.00001) {
		fprintf(
			stderr,
			"expected: %s %d %f, actual: %s %d %f\n",
			"G#",
			3,
			-0.753418,
			get_semitone_str(test_note.semitone, false),
			test_note.octave,
			test_note.cents);
		return 1;
	}

	test_note = get_approx_note(15.0);
	if (test_note.semitone != UNKNOWN_SEMITONE || test_note.octave != -1 || test_note.cents != -255.0) {
		fprintf(
			stderr,
			"expected: %d %d %f, actual: %d %d %f\n",
			UNKNOWN_SEMITONE,
			-1,
			-255.0,
			test_note.semitone,
			test_note.octave,
			test_note.cents);
		return 1;
	}

	if (UNKNOWN_SEMITONE != get_semitone(NULL)) {
		return 1;
	}

	if (UNKNOWN_SEMITONE != get_semitone("foo")) {
		return 1;
	}

	if (UNKNOWN_SEMITONE != get_semitone("h")) {
		return 1;
	}

	if (UNKNOWN_SEMITONE != get_semitone("a+")) {
		return 1;
	}

	if (C != get_semitone("c")) {
		return 1;
	}

	if (Gb != get_semitone("f#")) {
		return 1;
	}

	if (Ab != get_semitone("ab")) {
		return 1;
	}

	if (B != get_semitone("b")) {
		return 1;
	}

	if (0 != strcmp(get_semitone_str(2, true), "D")) {
		return 1;
	}

	if (0 != strcmp(get_semitone_str(6, true), "Gb")) {
		return 1;
	}

	if (0 != strcmp(get_semitone_str(9, false), "A")) {
		return 1;
	}

	if (0 != strcmp(get_semitone_str(10, false), "A#")) {
		return 1;
	}

	if (NULL != get_semitone_str(5555, true)) {
		return 1;
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

	if (UNKNOWN_SEMITONE != get_fifth(UNKNOWN_SEMITONE)) {
		return 1;
	}

	if (A != get_fifth(D)) {
		return 1;
	}

	if (UNKNOWN_SEMITONE != get_fourth(UNKNOWN_SEMITONE)) {
		return 1;
	}

	if (Ab != get_fourth(Eb)) {
		return 1;
	}

	scale = get_major_scale(C);
	for (int i = 0; i < sizeof(c_major_scale) / sizeof(enum semitone_t); ++i) {
		if (scale[i] != c_major_scale[i]) {
			return 1;
		}
	}
	free(scale);

	scale = get_major_scale(Gb);
	for (int i = 0; i < sizeof(gb_major_scale) / sizeof(enum semitone_t); ++i) {
		if (scale[i] != gb_major_scale[i]) {
			return 1;
		}
	}
	free(scale);

	scale = get_major_scale(B);
	for (int i = 0; i < sizeof(b_major_scale) / sizeof(enum semitone_t); ++i) {
		if (scale[i] != b_major_scale[i]) {
			return 1;
		}
	}
	free(scale);

	scale = get_major_scale(UNKNOWN_SEMITONE);
	if (NULL != scale) {
		return 1;
	}

	scale = get_natural_minor_scale(C);
	for (int i = 0; i < sizeof(c_natural_minor_scale) / sizeof(enum semitone_t); ++i) {
		if (scale[i] != c_natural_minor_scale[i]) {
			return 1;
		}
	}
	free(scale);

	scale = get_natural_minor_scale(G);
	for (int i = 0; i < sizeof(g_natural_minor_scale) / sizeof(enum semitone_t); ++i) {
		if (scale[i] != g_natural_minor_scale[i]) {
			return 1;
		}
	}
	free(scale);

	scale = get_natural_minor_scale(D);
	for (int i = 0; i < sizeof(d_natural_minor_scale) / sizeof(enum semitone_t); ++i) {
		if (scale[i] != d_natural_minor_scale[i]) {
			return 1;
		}
	}
	free(scale);

	scale = get_natural_minor_scale(UNKNOWN_SEMITONE);
	if (NULL != scale) {
                return 1;
	}

	scale = get_chromatic_scale(E);
	for (int i = 0; i < sizeof(e_chromatic_scale) / sizeof(enum semitone_t); ++i) {
		if (scale[i] != e_chromatic_scale[i]) {
			return 1;
		}
	}
	free(scale);

	return 0;
}
