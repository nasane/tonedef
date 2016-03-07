/*
 *  tonedef.c
 *
 *  Copyright (C) 2016  Nathan Bossart
 */

#include <assert.h>
#include <ctype.h>
#include <fftw3.h>
#include <math.h>
#include <sndfile.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tonedef.h"

#define MIN(a, b)		a < b ? a : b
#define AUDIO_SAMPLE_BUFSIZE	256

/* function prototypes for static functions */
static bool is_allowable_freq(double freq);
static enum semitone_t *get_scale(enum semitone_t tonic, enum semitone_t *scale, int scale_length);

/*
 * Retrieves the semitone enumeration representative of the string argument.
 * The letter portion of the string (the "step") may be either upper case or
 * lower case and must be the letters A-G.  The optional second character
 * indicates the exact semitone (or "half-step") of the note.  Any step may be
 * marked as either flat (represented by the letter 'b') or sharp (represented
 * by the character '#').  Please note that the semitone_t enumeration always
 * uses the "flat" notation for semitones in between full steps and always uses
 * the single-lettered notation otherwise.  For example, the argument "A#" will
 * yield the semitone_t Bb, and the argument "B#" will yield the semitone_t B.
 *
 * The first character of the string argument will be changed to its lowercase
 * counterpart.
 */
enum semitone_t get_semitone(const char * const semitone)
{
	char		*steps;
	char		step;
	enum semitone_t	ret;

	if (NULL == semitone) {
		return UNKNOWN_SEMITONE;
	}

	if (2 < strlen(semitone) || 1 > strlen(semitone)) {
		return UNKNOWN_SEMITONE;
	}

	step = tolower(semitone[0]);

	/*
	 * The position of the steps in this string is used to determine the
	 * integer value of the semitone_t enumeration.
	 */
	steps = "ccddeffggaab";

	for (int i = 0; '\0' != steps[i]; ++i) {

		if (steps[i] == step) {
			ret = i;
			break;
		}

		/*
		 * If we've reached the end of the steps string and we haven't
		 * broken from the loop, then the string is no note we know
		 * about!
		 */
		if ('\0' == steps[i + 1]) {
			return UNKNOWN_SEMITONE;
		}
	}

	if ('\0' == semitone[1]) {
		return ret;
	}

	if ('b' == semitone[1]) {
		return ret - 1;
	}

	if ('#' == semitone[1]) {
		return ret + 1;
	}

	return UNKNOWN_SEMITONE;
}

/*
 * This function retrieves a string representation of the provided semitone.
 * Return values will contain an uppercase letter in the range A-G followed by
 * an optional 'b' or '#' symbol.
 *
 * If the prefer_flat argument is true and the semitone falls between two
 * consecutive steps, the second character of the return value will be 'b'.  If
 * the prefer_flat argument is false and the semitone falls between two
 * consecutive steps, the second character of the return value will be '#'.
 */
char *get_semitone_str(enum semitone_t semitone, bool prefer_flat)
{
	char *semitone_strings_flat[]  = {"C",  "Db", "D",  "Eb", "E",  "F",
					  "Gb", "G",  "Ab", "A",  "Bb", "B"};
	char *semitone_strings_sharp[] = {"C",  "C#", "D",  "D#", "E",  "F",
					  "F#", "G",  "G#", "A",  "A#", "B"};

	if (semitone < C || semitone > B) {
		return NULL;
	}

	if (prefer_flat) {
		return semitone_strings_flat[semitone];
	}

	return semitone_strings_sharp[semitone];
}

/*
 * This function verifies that the provided frequency falls within the range of
 * notes this library can represent.  This range is from C0 -50.0 cents to B12
 * +50 cents.
 */
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

/*
 * This function retrieves the ideal frequency of a note given it's name and
 * octave (e.g. G# 5).  The ideal frequency is based upon the ISO 16:1975
 * standard frequency of the A4 note (i.e. 440 Hz) in twelve-tone equal
 * temperament.
 *
 * Returns the frequency if everything works as expected. Returns -1.0 for
 * illegal arguments.
 */
double get_freq(const struct note * const note)
{
	int	note_num;
	double	ideal_freq;
	double	exact_freq;

	if (note->semitone < C || note->semitone > B) {
		fprintf(stderr, "invalid semitone '%d'\n", note->semitone);
		return INVALID_FREQUENCY;
	}

	if (note->octave < OCTAVE_MIN || note->octave > OCTAVE_MAX) {
		fprintf(stderr, "octave must be within %d to %d\n",
			OCTAVE_MIN, OCTAVE_MAX);
		return INVALID_FREQUENCY;
	}

	if (note->cents < -(SEMITONE_INTERVAL_CENTS / 2.0) ||
	    note->cents >   SEMITONE_INTERVAL_CENTS / 2.0) {
		fprintf(stderr, "cents must be within %f to %f\n",
			-(SEMITONE_INTERVAL_CENTS / 2.0),
			  SEMITONE_INTERVAL_CENTS / 2.0);
		return INVALID_FREQUENCY;
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
	 * determine it's ideal frequency based on the standardized frequency of
	 * the the 49th key (A4) in twelve-tone equal temperament.  This value
	 * is defined in ISO 16:1975 as 440 Hz.
	 *
	 * f(n) = (sqrt[12]{2})^{n-49} * 440
	 */
	ideal_freq = pow(OCTAVE_JUST_INTERVAL,
		(note_num - NOTE_NUM_OF_A4) / (double) SEMITONES_PER_OCTAVE)
		* FREQ_OF_A4;

	/*
	 * Now, we just have to apply the cents to retrieve the exact frequency
	 * of the note.  This can be done using the following formula.
	 */
	exact_freq = ideal_freq
		* pow(OCTAVE_JUST_INTERVAL,
			note->cents / (SEMITONE_INTERVAL_CENTS
				* SEMITONES_PER_OCTAVE));

	return exact_freq;
}

/*
 * This function retrieves the note closest to the given frequency. The ideal
 * frequency is based upon the ISO 16:1975 standard frequency of the A4 note
 * (i.e. 440 Hz) in twelve-tone equal temperament.
 *
 * Returns the note if everything works as expected.  Returns an invalid note
 * for illegal arguments or internal error.
 */
struct note get_approx_note(double freq)
{
	struct note	note;
	int		note_num;

	/* check that the note exists between C0 and B24 */
	if (!is_allowable_freq(freq)) {
		fprintf(stderr, "frequency %f not in acceptable range\n", freq);
		note.semitone	= UNKNOWN_SEMITONE;
		note.octave	= INVALID_OCTAVE;
		note.cents	= INVALID_CENTS	;
		return note;
	}

	/*
	 * Determine the approximate note number of the frequency.
	 *
	 * This function is simply the inverse of the function used to find the
	 * frequency of a note given the note number.
	 */
	note_num = round(SEMITONES_PER_OCTAVE * log2(freq / FREQ_OF_A4)
			+ NOTE_NUM_OF_A4);

	/*
	 * Determine the octave of the note.
	 *
	 * There is an octave number offset due to the conventional division
         * of the 0th and 1st octaves between notes 3 and 4 (B0 and C1,
         * respectively).  In order to retrieve the correct octave number,
         * this offset must be added to the note number prior to dividing the
	 * note number by the number of notes per octave.
	 */
	note.octave = (note_num + OCTAVE_NOTE_NUM_OFFSET)
			/ SEMITONES_PER_OCTAVE;

	/*
	 * Deterine the "local" note.  This is simply the conventional letter
	 * and optional symbol identifier of the not without the octave number
	 * (e.g. B# 5).
	 */
	note.semitone = (note_num + OCTAVE_NOTE_NUM_OFFSET)
			% SEMITONES_PER_OCTAVE;

	note.cents = 0.0;  /* since this is approximate, this is always 0 */

	return note;
}

/*
 * This function retrieves the exact note for the given frequency. The ideal
 * frequency is based upon the ISO 16:1975 standard frequency of the A4 note
 * (i.e. 440 Hz) in twelve-tone equal temperament.  The cents member of the
 * note struct will be given a value in the (inclusive) range -50.0 to +50.0.
 *
 * Returns the note if everything works as expected.  Returns an invalid note
 * for illegal arguments or internal error.
 */
struct note get_exact_note(double freq)
{
	struct note	note;
	double		approx_note_freq;

	/* use our handy-dandy get_approx_note() fxn to get a ballpark note */
	note = get_approx_note(freq);
	if (UNKNOWN_SEMITONE == note.semitone) {
		return note;
	}

	/*
	 * In order to calculate the cents value for the exact note, we first
	 * must know the ideal frequency of the approximate note.
	 *
	 * We assume that get_freq() is called without any illegal arguments,
	 * since we have successfully gotten the approximate note if we have
	 * made it this far.
	 */
	approx_note_freq = get_freq(&note);

	/*
	 * Calculate the number of cents from the ideal frequency of the
	 * approximate note.  This is done by multiplying the number of cents
	 * per octave by the logarithm base two of the quotient of the
	 * frequencies.
	 */
	note.cents = SEMITONE_INTERVAL_CENTS * SEMITONES_PER_OCTAVE
		* log2(freq / approx_note_freq);

	return note;
}

/*
 * This function returns the semitone obtained by ascending by the interval of
 * an equal tempered fifth.
 */
enum semitone_t get_fifth(enum semitone_t semitone)
{
	if (C > semitone || B < semitone) {
		return UNKNOWN_SEMITONE;
	}

	/*
	 * I was originally going to write a really cool circle of fifths thing
	 * for this function, but then I realized it is much simpler to use
	 * basic arithmetic to go up seven semitones.
	 */
	return (semitone + 7) % SEMITONES_PER_OCTAVE;
}

/*
 * This function returns the semitone obtained by ascending by the interval of
 * an equal tempered fourth.
 */
 enum semitone_t get_fourth(enum semitone_t semitone)
 {
	if (C > semitone || B < semitone) {
		return UNKNOWN_SEMITONE;
	}

	/* All we have to do is go up five semitones. */
	return (semitone + 5) % SEMITONES_PER_OCTAVE;
 }

/*
 * Static function for getting several different types of scales.  This function
 * returns the generated scale on the heap.  The 'scale' parameter must be the
 * version of the scale with 'C' as the tonic note and it must end with
 * UNKNOWN_SEMITONE.
 *
 * Returns NULL for illegal arguments.
 */
static enum semitone_t *get_scale(enum semitone_t tonic, enum semitone_t *scale, int scale_length)
{
	assert(NULL != scale);
	assert(0 < scale_length);
	assert(UNKNOWN_SEMITONE == scale[scale_length - 1]);

	if (C > tonic || B < tonic) {
		return NULL;
	}

	enum semitone_t *ret = (enum semitone_t *) malloc(scale_length * sizeof(enum semitone_t));

	for (int i = 0; i < scale_length; ++i) {

		if (UNKNOWN_SEMITONE == scale[i]) {
			ret[i] = UNKNOWN_SEMITONE;
			continue;
		}

		ret[i] = (scale[i] + tonic) % SEMITONES_PER_OCTAVE;
	}

	return ret;
}

/*
 * This function returns the major scale of the given tonic note.  The scale is
 * an array of semitone_t terminated by an UNKNOWN_SEMITONE, and it is allocated
 * on the heap.  If the tonic is an illegal argument, NULL is returned.
 */
enum semitone_t *get_major_scale(enum semitone_t tonic)
{
	enum semitone_t major_scale[] = {C, D, E, F, G, A, B, UNKNOWN_SEMITONE};

	return get_scale(tonic, major_scale,
		sizeof(major_scale) / sizeof(enum semitone_t));
}

/*
 * This function returns the natural minor scale of the given tonic note.  The
 * scale is an array of semitone_t terminated by an UNKNOWN_SEMITONE, and it is
 * allocated on the heap.  If the tonic is an illegal argument, NULL is
 * returned.
 */
enum semitone_t *get_natural_minor_scale(enum semitone_t tonic)
{
	enum semitone_t natural_minor_scale[] = {C, D, Eb, F, G, Ab, Bb, UNKNOWN_SEMITONE};

	return get_scale(tonic, natural_minor_scale,
		sizeof(natural_minor_scale) / sizeof(enum semitone_t));
}

enum semitone_t *get_chromatic_scale(enum semitone_t tonic)
{
	enum semitone_t chromatic_scale[] = {C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B, UNKNOWN_SEMITONE};

	return get_scale(tonic, chromatic_scale,
		sizeof(chromatic_scale) / sizeof(enum semitone_t));
}

float *get_samples_from_file(char *filename, long samples)
{
	SNDFILE	*file;
	SF_INFO	sfinfo;
	float	*buf;
	float	*ret;
	int	audio_channels;
	int	rd_cnt;
	long	i;
	long	ret_bytes;
	long	bytes_to_cpy;


	if (NULL == filename) {
		return NULL;
	}

	if (samples < 0) {
		return NULL;
	}

	memset(&sfinfo, 0, sizeof(sfinfo));

	if (NULL == (file = sf_open(filename, SFM_READ, &sfinfo))) {
		/* TODO: print error */
		return NULL;
	}

	audio_channels = sfinfo.channels;
	ret_bytes = audio_channels * samples * sizeof(float);
	ret = (float *) malloc(ret_bytes);
	buf = (float *) malloc((audio_channels * AUDIO_SAMPLE_BUFSIZE) * sizeof(float));
	i = 0;
	while (i < ret_bytes && 0 < (rd_cnt = sf_readf_float(file, buf, AUDIO_SAMPLE_BUFSIZE))) {
		bytes_to_cpy = MIN(rd_cnt * audio_channels * sizeof(float), ret_bytes - i);
		memcpy(ret + (i / sizeof(float)), buf, bytes_to_cpy);
		i += bytes_to_cpy;
	}

	sf_close(file);
	return ret;
}

int split_stereo_channels(const float * const samples, long num_samples, float **chan1, float **chan2)
{
	long num_samples_per_chan;
	long i;

	if (NULL == samples) {
		return -1;
	}

	if (0 > num_samples) {
		return -1;
	}

	if (NULL == chan1) {
		return -1;
	}

	if (NULL == chan2) {
		return -1;
	}

	num_samples_per_chan = num_samples / 2 + 1;

	*chan1 = (float *) calloc(num_samples_per_chan, sizeof(float));
	*chan2 = (float *) calloc(num_samples_per_chan, sizeof(float));

	for (i = 0; i < num_samples; ++i) {
		if (0 == i % 2) {
			(*chan1)[i / 2] = samples[i];
		} else {
			(*chan2)[i / 2] = samples[i];
		}
	}

	return 0;
}

float *apply_hann_function(const float * const samples, long num_samples)
{
	long i;
	float *ret;

	if (NULL == samples) {
		return NULL;
	}

	if (0 > num_samples) {
		return NULL;
	}

	ret = (float *) malloc(num_samples * sizeof(float));
	for (i = 0; i < num_samples; ++i) {
		ret[i] = samples[i]
			* 0.5 * (1 - cos((2 * M_PI * i) / (num_samples - 1)));
	}

	return ret;
}
