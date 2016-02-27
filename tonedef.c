#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "tonedef.h"

/* function prototypes for static functions */
static bool is_allowable_freq(double freq);

/*
 * Retrieves the semitone enumeration.
 */
enum semitone_t get_semitone(char * const semitone)
{
	if (NULL == semitone) {
		return UNKNOWN_SEMITONE;
	}

	for (int i = 0; '\0' != semitone[i]; ++i) {
		semitone[i] = tolower(semitone[i]);
	}

	if (0 == strcmp(semitone, "b#") || 0 == strcmp(semitone, "c")) {
		return C;
	}

	if (0 == strcmp(semitone, "c#") || 0 == strcmp(semitone, "db")) {
		return Db;
	}

	if (0 == strcmp(semitone, "d")) {
		return D;
	}

	if (0 == strcmp(semitone, "d#") || 0 == strcmp(semitone, "eb")) {
		return Eb;
	}

	if (0 == strcmp(semitone, "e") || 0 == strcmp(semitone, "fb")) {
		return E;
	}

	if (0 == strcmp(semitone, "e#") || 0 == strcmp(semitone, "f")) {
		return F;
	}

	if (0 == strcmp(semitone, "f#") || 0 == strcmp(semitone, "gb")) {
		return Gb;
	}

	if (0 == strcmp(semitone, "g")) {
		return G;
	}

	if (0 == strcmp(semitone, "g#") || 0 == strcmp(semitone, "ab")) {
		return Ab;
	}

	if (0 == strcmp(semitone, "a")) {
		return A;
	}

	if (0 == strcmp(semitone, "a#") || 0 == strcmp(semitone, "bb")) {
		return Bb;
	}

	if (0 == strcmp(semitone, "b") || 0 == strcmp(semitone, "cb")) {
		return B;
	}

	return UNKNOWN_SEMITONE;
}

/*
 * This function retrieves the ideal frequency of a note
 * given it's name and octave (e.g. G# 5).  The ideal
 * frequency is based upon the ISO 16:1975 standard
 * frequency of the A4 note (i.e. 440 Hz) in twelve-tone
 * equal temperament.
 *
 * Returns the frequency if everything works as expected.
 * Returns -1.0 for illegal arguments or internal error.
 */
double get_freq(const struct note * const note)
{
	int	note_num;
	double	ideal_freq;
	double	exact_freq;

	/* verify input looks good */
	if (note->semitone < C || note->semitone > B) {
		fprintf(stderr, "semitone '%d' does not exist\n", note->semitone);
		return -1.0;
	}

	if (note->octave < OCTAVE_MIN || note->octave > OCTAVE_MAX) {
		fprintf(stderr, "octave must be within %d to %d\n", OCTAVE_MIN, OCTAVE_MAX);
		return -1.0;
	}

	if (note->cents < -(SEMITONE_INTERVAL_CENTS / 2.0) || note->cents > SEMITONE_INTERVAL_CENTS / 2.0) {
		fprintf(stderr, "cents must be within %f to %f\n", -(SEMITONE_INTERVAL_CENTS / 2.0), SEMITONE_INTERVAL_CENTS / 2.0);
		return -1.0;
	}

	/*
	 * Determine the frequency of the note.
	 *
	 * The frequency of the note is first determined by finding the note
	 * number.  A0 has note number 1, A#0 has note number 2, etc.  Using
	 * the octave and the semitone enumeration, the following formula can
	 * be used to determine the note number.
	 *
	 * There is an octave number offset due to the conventional division
	 * of the 0th and 1st octaves between notes 3 and 4 (B0 and C1,
	 * respectively).  In order to retrieve the semitone of the octave,
	 * this offset must be subtracted from the product of the octave
	 * number and the number of notes per octave.
	 */
	note_num = (note->octave * SEMITONES_PER_OCTAVE)
		- OCTAVE_NOTE_NUM_OFFSET
		+ note->semitone;

	/*
	 * Once we have the note number, we can use the following formula to
	 * determine it's ideal frequency based on the standardized
	 * frequency of the the 49th key (A4) in twelve-tone equal
	 * temperament.  This value is defined in ISO 16:1975 as 440 Hz.
	 *
	 * f(n) = (sqrt[12]{2})^{n-49} * 440
	 */
	ideal_freq = pow(OCTAVE_JUST_INTERVAL,
		(note_num - NOTE_NUM_OF_A4) / (double) SEMITONES_PER_OCTAVE)
		* FREQ_OF_A4;

	/*
	 * Now, we just have to apply the cents to retrieve the exact
	 * frequency of the note.  This can be done using the following
	 * formula.
	 */
	exact_freq = ideal_freq
		* pow(OCTAVE_JUST_INTERVAL,
			note->cents / (SEMITONE_INTERVAL_CENTS * SEMITONES_PER_OCTAVE));

	return exact_freq;
}

static bool is_allowable_freq(double freq)
{
	double		lowest_freq;
	double		highest_freq;
	struct note	lowest_note;
	struct note	highest_note;

	lowest_note.semitone	= C;  /* the "lowest" semitone enumeration */
	lowest_note.octave	= OCTAVE_MIN;
	lowest_note.cents	= -(SEMITONE_INTERVAL_CENTS / 2.0);  /* -50.0 */

	highest_note.semitone	= B;  /* the "highest" semitone enumeration */
	highest_note.octave	= OCTAVE_MAX;
	highest_note.cents	= SEMITONE_INTERVAL_CENTS / 2.0;  /* +50.0 */

	lowest_freq	= get_freq(&lowest_note);
	highest_freq	= get_freq(&highest_note);

	return (freq >= lowest_freq) && (freq <= highest_freq);
}

/* TODO: introduce prefer_sharp and prefer_flat */
char *get_semitone_str(enum semitone_t semitone)
{
	switch (semitone) {
	case 0:
		return "C";
	case 1:
		return "Db";
	case 2:
		return "D";
	case 3:
		return "Eb";
	case 4:
		return "E";
	case 5:
		return "F";
	case 6:
		return "Gb";
	case 7:
		return "G";
	case 8:
		return "Ab";
	case 9:
		return "A";
	case 10:
		return "Bb";
	case 11:
		return "B";
	default:
		return NULL;
	}
}

/*
 * This function retrieves the note closest to the given
 * frequency. The ideal frequency is based upon the ISO
 * 16:1975 standard frequency of the A4 note (i.e. 440 Hz)
 * in twelve-tone equal temperament.
 *
 * Returns the note if everything works as expected.
 * Returns an invalid note for illegal arguments or internal error.
 */
struct note get_approx_note(double freq)
{
	struct note	note;
	enum semitone_t	semitone;
	int		octave;
	int		note_num;

	/* check that the note exists between C0 and B24 */
	if (!is_allowable_freq(freq)) {
		fprintf(stderr, "frequency %f is not in the acceptable range\n", freq);
		note.semitone	= UNKNOWN_SEMITONE;
		note.octave	= -1;
		note.cents	= -255.0;
		return note;
	}

	/*
	 * Determine the approximate note number of the frequency.
	 *
	 * This function is simply the inverse of the function used to find the frequency
	 * of a note given the note number.
	 */
	note_num = round(SEMITONES_PER_OCTAVE * log2(freq / FREQ_OF_A4) + NOTE_NUM_OF_A4);

	/*
	 * Determine the octave of the note.
	 *
	 * There is an octave number offset due to the conventional division
         * of the 0th and 1st octaves between notes 3 and 4 (B0 and C1,
         * respectively).  In order to retrieve the correct octave number,
         * this offset must be added to the note number prior to dividing the
         * note number by the number of notes per octave.
	 */
	octave = (note_num + OCTAVE_NOTE_NUM_OFFSET) / SEMITONES_PER_OCTAVE;

	/*
	 * Deterine the "local" note.  This is simply the conventional letter
	 * and optional symbol identifier of the not without the octave number
	 * (e.g. B# 5).
	 */
	semitone = (note_num + OCTAVE_NOTE_NUM_OFFSET) % SEMITONES_PER_OCTAVE;

	note.semitone	= semitone;
	note.octave	= octave;
	note.cents	= 0.0;
	return note;
}

struct note get_exact_note(double freq)
{
	struct note	note;
	double		approx_note_freq;
	double		cents;

	/* use our handy-dandy get_approx_note() function to get a ballpark note */
	note = get_approx_note(freq);
	if (UNKNOWN_SEMITONE == note.semitone) {
		return note;
	}

	/* TODO: fix this */
	if (-1.0 == (approx_note_freq = get_freq(&note))) {
		return note;
	}

	cents = SEMITONE_INTERVAL_CENTS * SEMITONES_PER_OCTAVE * log2(freq / approx_note_freq);
	note.cents = cents;

	return note;
}
