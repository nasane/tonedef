/*
 *  test.c
 *
 *  Copyright (C) 2016  Nathan Bossart
 */

#include <assert.h>
#include <fftw3.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tonedef.h"

#define HALF_SECOND_SAMPLE_COUNT		22050
#define FREE_SAFELY(a)				free(a); a = NULL
#define DOUBLE_EQUALS(a, b)			(fabs((a) - (b)) < 0.00001)
#define TEST_SCALE(expected, actual, scale)	{										\
							scale = actual;								\
							for (int i = 0; i < sizeof(expected) / sizeof(enum semitone_t); ++i) {	\
								assert(expected[i] == scale[i]);				\
							}									\
							FREE_SAFELY(scale);							\
						}
#define LOG(method_name)			printf("----- Testing %s(...) -----\n", method_name)
#define SET_NOTE(note, semi, oct, cent)		{				\
							note.semitone = semi;	\
							note.octave = oct;	\
							note.cents = cent;	\
						}

int main(int argc, const char *argv[])
{
	double bogus_samples[1];
	long samples_returned;
	double *wav_samples, *wav_samples_hannd, *wav_samples_left, *wav_samples_right;
	struct note test_note, note_from_file, n, test_note_2, test_note_3, test_note_4;
	char *semitone;
	enum semitone_t *scale;
	enum semitone_t c_major_scale[] = {C, D, E, F, G, A, B, UNKNOWN_SEMITONE};
	enum semitone_t gb_major_scale[] = {Gb, Ab, Bb, B, Db, Eb, F, UNKNOWN_SEMITONE};
	enum semitone_t b_major_scale[] = {B, Db, Eb, E, Gb, Ab, Bb, UNKNOWN_SEMITONE};
	enum semitone_t c_natural_minor_scale[] = {C, D, Eb, F, G, Ab, Bb, UNKNOWN_SEMITONE};
	enum semitone_t g_natural_minor_scale[] = {G, A, Bb, C, D, Eb, F, UNKNOWN_SEMITONE};
	enum semitone_t d_natural_minor_scale[] = {D, E, F, G, A, Bb, C, UNKNOWN_SEMITONE};
	enum semitone_t e_chromatic_scale[] = {E, F, Gb, G, Ab, A, Bb, B, C, Db, D, Eb, UNKNOWN_SEMITONE};
	enum semitone_t a_harmonic_minor_scale[] = {A, B, C, D, E, F, Ab, UNKNOWN_SEMITONE};
	enum semitone_t g_melodic_minor_scale[] = {G, A, Bb, C, D, E, Gb, UNKNOWN_SEMITONE};
	fftw_complex *fft_samples;
	struct chord chord;
	struct note_node node, node2, node3, node4;

	LOG("get_exact_note");

	test_note = get_exact_note(440.0);
	assert(test_note.semitone == A && test_note.octave == 4 && DOUBLE_EQUALS(test_note.cents, 0.0));

	test_note = get_exact_note(45.21);
	assert(test_note.semitone == Gb && test_note.octave == 1 && DOUBLE_EQUALS(test_note.cents, -39.347641));

	test_note = get_exact_note(2016.15);
	assert(test_note.semitone == B && test_note.octave == 6 && DOUBLE_EQUALS(test_note.cents, 35.233059));

	test_note = get_exact_note(207.562);
	assert(test_note.semitone == Ab && test_note.octave == 3 && DOUBLE_EQUALS(test_note.cents, -0.753418));

	test_note = get_exact_note(1.0);
	assert(UNKNOWN_SEMITONE == test_note.semitone && -1 == test_note.octave && DOUBLE_EQUALS(test_note.cents, -255.0));

	LOG("get_approx_note");

	test_note = get_approx_note(15.0);
	assert(test_note.semitone == UNKNOWN_SEMITONE && test_note.octave == -1 && DOUBLE_EQUALS(test_note.cents, -255.0));

	LOG("get_semitone");

	assert(UNKNOWN_SEMITONE == get_semitone(NULL));
	assert(UNKNOWN_SEMITONE == get_semitone("foo"));
	assert(UNKNOWN_SEMITONE == get_semitone("h"));
	assert(UNKNOWN_SEMITONE == get_semitone("a+"));
	assert(C == get_semitone("c"));
	assert(Gb == get_semitone("f#"));
	assert(Ab == get_semitone("ab"));
	assert(B == get_semitone("b"));

	LOG("get_semitone_str");

	assert(0 == strcmp(get_semitone_str(2, true), "D"));
	assert(0 == strcmp(get_semitone_str(6, true), "Gb"));
	assert(0 == strcmp(get_semitone_str(9, false), "A"));
	assert(0 == strcmp(get_semitone_str(10, false), "A#"));
	assert(NULL == get_semitone_str(5555, true));

	LOG("get_freq");

	SET_NOTE(test_note, UNKNOWN_SEMITONE, 4, 0.0);
	assert(-1 == get_freq(&test_note));

	SET_NOTE(test_note, B, OCTAVE_MAX + 5, 0.0);
	assert(-1 == get_freq(&test_note));

	SET_NOTE(test_note, B, 3, SEMITONE_INTERVAL_CENTS);
	assert(-1 == get_freq(&test_note));

	LOG("get_fifth");

	assert(UNKNOWN_SEMITONE == get_fifth(UNKNOWN_SEMITONE));
	assert(A == get_fifth(D));

	LOG("get_fourth");

	assert(UNKNOWN_SEMITONE == get_fourth(UNKNOWN_SEMITONE));
	assert(Ab == get_fourth(Eb));

	LOG("get_major_scale");

	TEST_SCALE(c_major_scale, get_major_scale(C), scale);
	TEST_SCALE(gb_major_scale, get_major_scale(Gb), scale);
	TEST_SCALE(b_major_scale, get_major_scale(B), scale);
	assert(NULL == get_major_scale(UNKNOWN_SEMITONE));

	LOG("get_natural_minor_scale");

	assert(NULL == get_natural_minor_scale(UNKNOWN_SEMITONE));
	TEST_SCALE(c_natural_minor_scale, get_natural_minor_scale(C), scale);
	TEST_SCALE(g_natural_minor_scale, get_natural_minor_scale(G), scale);
	TEST_SCALE(d_natural_minor_scale, get_natural_minor_scale(D), scale);

	LOG("get_chromatic_scale");

	TEST_SCALE(e_chromatic_scale, get_chromatic_scale(E), scale);

	LOG("get_harmonic_minor_scale");

	TEST_SCALE(a_harmonic_minor_scale, get_harmonic_minor_scale(A), scale);

	LOG("get_melodic_minor_scale");

	TEST_SCALE(g_melodic_minor_scale, get_melodic_minor_scale(G), scale);

	LOG("get_samples_from_file");

	wav_samples = get_samples_from_file("a4.wav", 12, &samples_returned);
	assert(12 == samples_returned);
	assert(DOUBLE_EQUALS(wav_samples[0], 0.0000000000));
	assert(DOUBLE_EQUALS(wav_samples[3], 0.0621588230));
	assert(DOUBLE_EQUALS(wav_samples[4], 0.1240735054));
	assert(DOUBLE_EQUALS(wav_samples[7], 0.1855007410));
	assert(DOUBLE_EQUALS(wav_samples[8], 0.2461992502));
	assert(DOUBLE_EQUALS(wav_samples[11], 0.3059304953));
	FREE_SAFELY(wav_samples);

	assert(NULL == get_samples_from_file(NULL, 12, &samples_returned) || samples_returned != -1);
	assert(NULL == get_samples_from_file("a4.wav", -23, &samples_returned) || samples_returned != -1);
	assert(NULL == get_samples_from_file("does_not_exist.wav", 1024, &samples_returned) || samples_returned != -1);
	assert(NULL == get_samples_from_file("a4.wav", 24, NULL));
	assert(NULL != (wav_samples = get_samples_from_file("a4.wav", 24, &samples_returned)) || samples_returned != 24);

	LOG("split_stereo_channels");

	assert(-1 == split_stereo_channels(NULL, 24, &wav_samples_left, &wav_samples_right) && wav_samples_left == NULL && wav_samples_right == NULL);
	assert(-1 == split_stereo_channels(wav_samples, -123, &wav_samples_left, &wav_samples_right) && wav_samples_left == NULL && wav_samples_right == NULL);
	assert(-1 == split_stereo_channels(wav_samples, 24, NULL, &wav_samples_right) && wav_samples_right == NULL);
	assert(-1 == split_stereo_channels(wav_samples, 24, &wav_samples_left, NULL) && wav_samples_left == NULL);
	assert(0 == split_stereo_channels(wav_samples, 24, &wav_samples_left, &wav_samples_right) && wav_samples_left != NULL && wav_samples_right != NULL);

	assert(DOUBLE_EQUALS(wav_samples_left[3], 0.1855007410));
	assert(DOUBLE_EQUALS(wav_samples_left[7], 0.4215573072));
	assert(DOUBLE_EQUALS(wav_samples_left[11], 0.6312451363));
	assert(DOUBLE_EQUALS(wav_samples_right[3], 0.1855007410));
	assert(DOUBLE_EQUALS(wav_samples_right[7], 0.4215573072));
	assert(DOUBLE_EQUALS(wav_samples_right[11], 0.6312451363));

	LOG("apply_hann_function");

	assert(NULL == apply_hann_function(wav_samples_left, -34));
	assert(NULL == apply_hann_function(NULL, 12));
	assert(NULL != (wav_samples_hannd = apply_hann_function(wav_samples_left, 24)));
	assert(DOUBLE_EQUALS(wav_samples_hannd[0], 0.0000000000));
	assert(DOUBLE_EQUALS(wav_samples_hannd[6], 0.1946656853));
	assert(DOUBLE_EQUALS(wav_samples_hannd[20], 0.1496363133));

	FREE_SAFELY(wav_samples_hannd);
	FREE_SAFELY(wav_samples_left);
	FREE_SAFELY(wav_samples_right);
	FREE_SAFELY(wav_samples);

	LOG("get_note_from_file");

	note_from_file = get_note_from_file(NULL, .75);
	assert(UNKNOWN_SEMITONE == note_from_file.semitone);

	note_from_file = get_note_from_file("a4.wav", -1.69);
	assert(UNKNOWN_SEMITONE == note_from_file.semitone);

	note_from_file = get_note_from_file("a4.wav", 100);
	assert(UNKNOWN_SEMITONE == note_from_file.semitone);

	note_from_file = get_note_from_file("does_not_exist.wav", 1.0);
	assert(UNKNOWN_SEMITONE == note_from_file.semitone);

	note_from_file = get_note_from_file("a4.wav", 0.5);
	assert(A == note_from_file.semitone && 4 == note_from_file.octave && DOUBLE_EQUALS(note_from_file.cents, 0.0000000000));

	note_from_file = get_note_from_file("g#5-piano.wav", 0.234);
	assert(Ab == note_from_file.semitone && 5 == note_from_file.octave && DOUBLE_EQUALS(note_from_file.cents, 5.6681983644));

	note_from_file = get_note_from_file("f4-piano.wav", 0.345);
	assert(F == note_from_file.semitone && 4 == note_from_file.octave && DOUBLE_EQUALS(note_from_file.cents, -6.9648619035));

	LOG("get_fft");

	assert(NULL == get_fft(NULL, 12345));
	assert(NULL == get_fft(bogus_samples, -123));

	LOG("get_chord");

	chord = get_chord(NULL);
	assert(chord.chord = UNKNOWN_CHORD_TYPE && chord.tonic == UNKNOWN_SEMITONE && chord.bass == UNKNOWN_SEMITONE);

	SET_NOTE(test_note, D, 2, 0.123);
	SET_NOTE(test_note_2, Gb, 4, 1.5);
	SET_NOTE(test_note_3, A, 4, 2.235);
	node.note = test_note;
	node.next = &node2;
	node2.note = test_note_2;
	node2.next = &node3;
	node3.note = test_note_3;
	node3.next = NULL;
	chord = get_chord(&node);
	assert(MAJOR_TRIAD == chord.chord && D == chord.tonic && D == chord.bass);

	SET_NOTE(test_note, F, 2, 0.123);
	SET_NOTE(test_note_2, Ab, 4, 1.5);
	SET_NOTE(test_note_3, C, 4, 2.235);
	node.note = test_note;
	node.next = &node2;
	node2.note = test_note_2;
	node2.next = &node3;
	node3.note = test_note_3;
	node3.next = NULL;
	chord = get_chord(&node);
	assert(MINOR_TRIAD == chord.chord && F == chord.tonic && F == chord.bass);

	SET_NOTE(test_note, G, 2, 0.123);
	SET_NOTE(test_note_2, B, 4, 1.5);
	SET_NOTE(test_note_3, D, 4, 2.235);
	SET_NOTE(test_note_4, Gb, 4, 0.00);
	node.note = test_note;
	node.next = &node2;
	node2.note = test_note_2;
	node2.next = &node3;
	node3.note = test_note_3;
	node3.next = &node4;
	node4.note = test_note_4;
	node4.next = NULL;
	chord = get_chord(&node);
	assert(MAJOR_SEVENTH == chord.chord && G == chord.tonic && G == chord.bass);

	SET_NOTE(test_note_4, Bb, 2, 0.123);
	SET_NOTE(test_note_2, D, 4, 1.5);
	SET_NOTE(test_note_3, F, 4, 2.235);
	SET_NOTE(test_note, F, 5, 0.00);
	node.note = test_note;
	node.next = &node2;
	node2.note = test_note_2;
	node2.next = &node3;
	node3.note = test_note_3;
	node3.next = &node4;
	node4.note = test_note_4;
	node4.next = NULL;
	chord = get_chord(&node);
	assert(MAJOR_TRIAD == chord.chord && Bb == chord.tonic && Bb == chord.bass);

	return 0;
}
