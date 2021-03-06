/*
 *  common.c
 *
 *  Copyright (C) 2016  Nathan Bossart
 */

#include <assert.h>
#include "common.h"
#include <ctype.h>
#include <fftw3.h>
#include <math.h>
#include <sndfile.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

/* function prototypes for static functions */
static bool			is_allowable_freq(double freq);
static enum semitone_t *	get_scale(enum semitone_t tonic, enum semitone_t *scale, int scale_length);
static void 			get_sample_rate_of_file(char *filename, int *sample_rate, int *num_channels);
static double *			combine_channels(double *samples, long num_samples, int num_channels);
static double *			get_fft_magnitudes(fftw_complex *fft_samples, long num_samples);
static void			get_sound_file_metadata(const char * const filename, int *sample_rate, int *num_channels);
//static double *		get_autocorrelation_function(const double * const samples, long num_samples, int sample_rate)
//static double *		get_avg_magnitude_diff_function(const double * const samples, long num_samples, int sample_rate);
//static double *		get_weighted_autocorrelation_function(double *acf, double *amdf, long num_samples);
static long			get_index_of_maximum(double *array, long array_len);

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
		fprintf(stderr, "semitone argument is null\n");
		return UNKNOWN_SEMITONE;
	}

	if (2 < strlen(semitone) || 1 > strlen(semitone)) {
		fprintf(stderr, "semitone argument can only be 1 or 2 characters\n");
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
			fprintf(stderr, "unknown semitone '%s'\n", semitone);
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
		fprintf(stderr, "invalid semitone '%d'\n", semitone);
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
	assert(INVALID_FREQUENCY != approx_note_freq);

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
		fprintf(stderr, "invalid semitone '%d'\n", semitone);
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
		fprintf(stderr, "invalid semitone '%d'\n", semitone);
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
		fprintf(stderr, "invalid tonic '%d'\n", tonic);
		return NULL;
	}

	enum semitone_t *ret = (enum semitone_t *) MALLOC_SAFELY(scale_length * sizeof(enum semitone_t));

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

/*
 * This function returns the harmonic minor scale of the given tonic note.  The
 * scale is an array of semitone_t terminated by an UNKNOWN_SEMITONE, and it is
 * allocated on the heap.  If the tonic is an illegal argument, NULL is
 * returned.
 */
enum semitone_t *get_harmonic_minor_scale(enum semitone_t tonic)
{
	enum semitone_t harmonic_minor_scale[] = {C, D, Eb, F, G, Ab, B, UNKNOWN_SEMITONE};

	return get_scale(tonic, harmonic_minor_scale,
		sizeof(harmonic_minor_scale) / sizeof(enum semitone_t));
}

/*
 * This function returns the melodic minor scale of the given tonic note.  The
 * scale is an array of semitone_t terminated by an UNKNOWN_SEMITONE, and it is
 * allocated on the heap.  If the tonic is an illegal argument, NULL is
 * returned.
 */
enum semitone_t *get_melodic_minor_scale(enum semitone_t tonic)
{
	enum semitone_t melodic_minor_scale[] = {C, D, Eb, F, G, A, B, UNKNOWN_SEMITONE};

	return get_scale(tonic, melodic_minor_scale,
		sizeof(melodic_minor_scale) / sizeof(enum semitone_t));
}

/*
 * This function returns the chromatic scale of the given tonic note.  The scale
 * is an array of semitone_t terminated by an UNKNOWN_SEMITONE, and it is
 * allocated on the heap.  If the tonic is an illegal argument, NULL is
 * returned.
 */
enum semitone_t *get_chromatic_scale(enum semitone_t tonic)
{
	enum semitone_t chromatic_scale[] = {C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B, UNKNOWN_SEMITONE};

	return get_scale(tonic, chromatic_scale,
		sizeof(chromatic_scale) / sizeof(enum semitone_t));
}

/*
 * This function retrieves the sample rate and number of channels in the audio
 * file, storing them in the respective arguments.
 *
 * If there are problems with the file, the sample rate and channel count are
 * set to -1.  Otherwise, these values should be set as expected.
 */
static void get_sound_file_metadata(const char * const filename, int *sample_rate, int *num_channels)
{
	SF_INFO	sfinfo;
	SNDFILE	*file;

	assert(NULL != sample_rate);
	assert(NULL != num_channels);
	assert(NULL != filename);

	/* initialize to invalid values in case we hit an error */
	*sample_rate = -1;
	*num_channels = -1;

	memset(&sfinfo, 0, sizeof(sfinfo));
	if (NULL == (file = sf_open(filename, SFM_READ, &sfinfo))) {
		return;
	}

	if (0 != sf_close(file)) {
		return;
	}

	*sample_rate = sfinfo.samplerate;
	*num_channels = sfinfo.channels;
}

/* TODO: documentation */
double *get_samples_from_file(const char * const filename, long frames_requested, long *frames_returned)
{
	SNDFILE	*file;
	SF_INFO	sfinfo;
	double	*buf;
	double	*ret;
	int	audio_channels;
	int	rd_cnt;
	long	frames_written;
	long	frames_to_copy;

	if (NULL == frames_returned) {
		fprintf(stderr, "frames_returned cannot be NULL\n");
		return NULL;
	}
	*frames_returned = -1;

	if (NULL == filename) {
		return NULL;
	}

	if (0 >= frames_requested) {
		return NULL;
	}

	memset(&sfinfo, 0, sizeof(sfinfo));
	if (NULL == (file = sf_open(filename, SFM_READ, &sfinfo))) {
		return NULL;
	}

	/* each frame contains a sample per audio channel */
	audio_channels = sfinfo.channels;
	ret = (double *) MALLOC_SAFELY(frames_requested * audio_channels * sizeof(double));

	/* we use a temporary buffer for pulling the samples into memory */
	buf = (double *) MALLOC_SAFELY(audio_channels * AUDIO_SAMPLE_BUFSIZE * sizeof(double));
	frames_written = 0;
	while (frames_written < frames_requested && 0 < (rd_cnt = sf_readf_double(file, buf, AUDIO_SAMPLE_BUFSIZE))) {

		/*
		 * We'll be able to copy either all of the bytes in our
		 * temporary buffer or only the ones that fit in our allocated
		 * memory, whichever is smaller.
		 */
		frames_to_copy = MIN(rd_cnt, frames_requested - frames_written);
		memcpy(ret + (frames_written * audio_channels), buf, frames_to_copy * audio_channels * sizeof(double));
		frames_written += frames_to_copy;
	}
	FREE_SAFELY(buf);

	if (0 != sf_close(file)) {
		FREE_SAFELY(ret);
		return NULL;
	}

	*frames_returned = frames_written;
	return ret;
}

/* TODO: documentation, perhaps make generic */
int split_stereo_channels(const double * const samples, long num_samples, double **chan1, double **chan2)
{
	long i;

	/*
	 * If one channel is NULL, make sure the other one is set to NULL when
	 * we error out.
	 */
	if (NULL == chan1) {
		if (chan2 != NULL) {
			*chan2 = NULL;
		}
		return SPLIT_STEREO_CHANNELS_FAILURE_CODE;
	}
	*chan1 = NULL;

	if (NULL == chan2) {
		return SPLIT_STEREO_CHANNELS_FAILURE_CODE;
	}
	*chan2 = NULL;

	if (NULL == samples) {
		return SPLIT_STEREO_CHANNELS_FAILURE_CODE;
	}

	if (0 > num_samples) {
		return SPLIT_STEREO_CHANNELS_FAILURE_CODE;
	}

	*chan1 = (double *) CALLOC_SAFELY(num_samples, sizeof(double));
	*chan2 = (double *) CALLOC_SAFELY(num_samples, sizeof(double));

	for (i = 0; i < num_samples * STEREO_NUM_CHANNELS; ++i) {
		if (0 == i % STEREO_NUM_CHANNELS) {
			(*chan1)[i / STEREO_NUM_CHANNELS] = samples[i];
		} else {
			(*chan2)[i / STEREO_NUM_CHANNELS] = samples[i];
		}
	}

	return SPLIT_STEREO_CHANNELS_SUCCESS_CODE;
}

/* TODO: documentation */
double *apply_hann_function(const double * const samples, long num_samples)
{
	long	i;
	double *ret;

	if (NULL == samples) {
		return NULL;
	}

	if (0 > num_samples) {
		return NULL;
	}

	ret = (double *) MALLOC_SAFELY(num_samples * sizeof(double));
	for (i = 0; i < num_samples; ++i) {
		ret[i] = samples[i]
			* 0.5 * (1 - cos((2 * M_PI * i) / (num_samples - 1)));
	}

	return ret;
}

/* TODO: documentation */
fftw_complex *get_fft(double *samples, long num_samples)
{
	fftw_complex *	ret;
	fftw_plan	plan;

	if (NULL == samples) {
		return NULL;
	}

	if (0 > num_samples) {
		return NULL;
	}

	ret = (fftw_complex *) MALLOC_SAFELY(num_samples * sizeof(fftw_complex));
	if (NULL == (plan = fftw_plan_dft_r2c_1d(num_samples, samples, ret, FFTW_ESTIMATE))) {
		return NULL;
	}

	fftw_execute(plan);
	fftw_destroy_plan(plan);

	return ret;
}

static double *combine_channels(double *samples, long num_samples, int num_channels)
{
	long	i;
	int	j;
	double	*ret;

	assert(NULL != samples);
	assert(0 < num_samples);
	assert(0 < num_channels);

	ret = (double *) MALLOC_SAFELY(num_samples * sizeof(double));

	for (i = 0; i < num_samples; ++i) {
		ret[i] = 0.0;
		for (j = 0; j < num_channels; ++j) {
			ret[i] += samples[i * num_channels + j] / num_channels;
		}
	}

	return ret;
}

static double *get_fft_magnitudes(fftw_complex *fft_samples, long num_samples)
{
	long i;
	double *ret;

	assert(NULL != fft_samples);
	assert(0 < num_samples);

	ret = (double *) MALLOC_SAFELY(num_samples * sizeof(double));

	for (i = 0; i < num_samples; ++i) {
		ret[i] = sqrt(fft_samples[i][0] * fft_samples[i][0] + fft_samples[i][1] * fft_samples[i][1]);
		//ret[i] = 10 * log10(ret[i]);
	}

	return ret;
}

/*
 * I've commented out the functions related to autocorrelation until they are
 * used for something.  Until then, there's no use counting them in the code
 * coverage.
 */
/*
static double *get_autocorrelation_function(const double * const samples, long num_samples, int sample_rate)
{
	double *ret;
	long tau;
	struct note lowest_note;
	struct note highest_note;
	long min_tau;
	long max_tau;
	long iterations;
	long i;

	assert(NULL != samples);
	assert(0 < num_samples);
	assert(0 < sample_rate);

	lowest_note.semitone	= C;  // the "lowest" semitone enumeration
	lowest_note.octave	= OCTAVE_MIN;
	lowest_note.cents	= -(SEMITONE_INTERVAL_CENTS / 2.0);  // -50.0
	max_tau = (1.0 / get_freq(&lowest_note)) * sample_rate;

	highest_note.semitone	= B;  // the "highest" semitone enumeration
	highest_note.octave	= OCTAVE_MAX;
	highest_note.cents	= SEMITONE_INTERVAL_CENTS / 2.0;  // +50.0
	min_tau = (1.0 / get_freq(&highest_note)) * sample_rate;

	max_tau = MIN(max_tau, num_samples);

	/// if this isn't true, then we have bigger problems
	assert(min_tau < max_tau);

	ret = (double *) CALLOC_SAFELY(num_samples, sizeof(double));

	for (tau = min_tau; tau < max_tau; ++tau) {

		iterations = num_samples - tau - 1;
		ret[tau] = 0.0;
		for (i = 0; i < iterations; ++i) {
			ret[tau] += samples[i] * samples[i + tau];
		}
		// ret[tau] *= 1.0 / iterations;
	}

	return ret;
}

static double *get_avg_magnitude_diff_function(const double * const samples, long num_samples, int sample_rate)
{
	double *	ret;
	struct note	lowest_note;
	struct note	highest_note;
	long		tau;
	long		min_tau;
	long		max_tau;
	long		iterations;
	long		i;

	assert(NULL != samples);
	assert(0 < num_samples);
	assert(0 < sample_rate);

	lowest_note.semitone	= C;  // the "lowest" semitone enumeration
	lowest_note.octave	= OCTAVE_MIN;
	lowest_note.cents	= -(SEMITONE_INTERVAL_CENTS / 2.0);  // -50.0
	max_tau = (1.0 / get_freq(&lowest_note)) * sample_rate;

	highest_note.semitone	= B;  // the "highest" semitone enumeration
	highest_note.octave	= OCTAVE_MAX;
	highest_note.cents	= SEMITONE_INTERVAL_CENTS / 2.0;  // +50.0
	min_tau = (1.0 / get_freq(&highest_note)) * sample_rate;

	max_tau = MIN(max_tau, num_samples);

	// if this isn't true, then we have bigger problems
	assert(min_tau < max_tau);

	ret = (double *) CALLOC_SAFELY(num_samples, sizeof(double));

	for (tau = min_tau; tau < MIN(max_tau, num_samples); ++tau) {

		iterations = num_samples - tau - 1;
		ret[tau] = 0.0;
		for (i = 0; i < iterations; ++i) {
			ret[tau] += samples[i] - samples[i + tau];
		}
		// ret[tau] *= 1.0 / iterations;
	}

	return ret;
}

static double *get_weighted_autocorrelation_function(double *acf, double *amdf, long num_samples)
{
	double *ret;
	long	i;

	assert(NULL != acf);
	assert(NULL != amdf);
	assert(0 < num_samples);

	ret = (double *) MALLOC_SAFELY(num_samples * sizeof(double));

	for (i = 0; i < num_samples; ++i) {
		ret[i] = acf[i] / (amdf[i] + 1);
	}

	return ret;
}
*/

/*
 * A simple convenience function for getting the index of the maximum double in
 * an array.  Since it is a static function, either the index of the maximum
 * value should be returned or an assertion should fail (causing the program to
 * exit with an error message).
 */
static long get_index_of_maximum(double *array, long array_len)
{
	long	maximum_index;
	long	i;
	double	maximum;

	assert(NULL != array);
	assert(0 < array_len);

	/*
	 * We prep the return value by starting with the first element in the
	 * array.
	 */
	maximum_index = 0;
	maximum = array[0];

	/* now iterate over the rest, updating each time we find a new max */
	for (i = 1; i < array_len; ++i) {
		if (array[i] > maximum) {
			maximum = array[i];
			maximum_index = i;
		}
	}

	return maximum_index;
}

/* TODO: write version to pick up all prominent notes */
struct note get_note_from_file(const char * const filename, double secs_to_sample)
{
	int		sample_rate;
	int		num_channels;
	long		sample_num_of_highest_magnitude;
	long		samples_returned;
	long		num_samples;
	double *	samples;
	double *	mono_samples;
	double *	hannd_samples;
	double *	fft_magnitudes;
	struct note	invalid_note;
	fftw_complex *	fft_samples;

	/* initialize as invalid note for error checking purposes */
	invalid_note.semitone	= UNKNOWN_SEMITONE;
	invalid_note.octave	= INVALID_OCTAVE;
	invalid_note.cents	= INVALID_CENTS	;

	if (NULL == filename) {
		fprintf(stderr, "filename is null\n");
		return invalid_note;
	}

	if (0.0 >= secs_to_sample) {
		fprintf(stderr, "secs_to_sample is less than zero\n");
		return invalid_note;
	}

	/* get the sample rate and number of channels in the file */
	get_sound_file_metadata(filename, &sample_rate, &num_channels);
	if (-1 == sample_rate || -1 == num_channels) {
		fprintf(stderr, "could not retrieve sound file metadata; does the file exist?\n");
		return invalid_note;
	}

	/* get the number of samples we'll be working with */
	num_samples = secs_to_sample * sample_rate;

	/*
	 * Get the samples from the file.
	 *
	 * If the the file is gone or we don't get the requested number of
	 * samples back, we return an invalid note.
	 */
	samples = get_samples_from_file(filename, num_samples, &samples_returned);
	if (NULL == samples || num_samples != samples_returned) {
		fprintf(stderr, "could not access file or retrieve requested number of samples from file\n");
		FREE_SAFELY(samples);
		return invalid_note;
	}

	/* combine all the channels into one */
	mono_samples = combine_channels(samples, num_samples, num_channels);
	FREE_SAFELY(samples);

	/* apply the Hanning function to our window of samples */
	if (NULL == (hannd_samples = apply_hann_function(mono_samples, num_samples))) {
		fprintf(stderr, "could not apply hanning function; chances are there is a bigger problem\n");
		FREE_SAFELY(mono_samples);
		FREE_SAFELY(hannd_samples);
		return invalid_note;
	}
	FREE_SAFELY(mono_samples);

	/* get the Fast Fourier Transform of our samples */
	if (NULL == (fft_samples = get_fft(hannd_samples, num_samples))) {
		fprintf(stderr, "could not calculate fft\n");
		FREE_SAFELY(hannd_samples);
		FREE_SAFELY(fft_samples);
		return invalid_note;
	}
	FREE_SAFELY(hannd_samples);

	/*
	 * The FFT output array is all complex numbers.  We need to calculate
	 * the magnitude of each output value in order to reveal the frequencies
	 * that we care about.
	 */
	fft_magnitudes = get_fft_magnitudes(fft_samples, num_samples);
	fftw_free(fft_samples);
	fft_samples = NULL;

	/* get the FFT sample index that has the highest magnitude */
	sample_num_of_highest_magnitude = get_index_of_maximum(fft_magnitudes, num_samples);
	FREE_SAFELY(fft_magnitudes);

	/*
	 * Finally, get the note.
	 *
	 * The frequency is simply the sample number in our FFT divided by the
	 * number of seconds that we sampled.  This is because we have
	 * num_samples samples in our FFT and secs_to_sample = num_samples /
	 * sample_rate.  It also makes sense that the denominator here would be
	 * in seconds since hertz = seconds^-1.
	 */
	return get_exact_note(sample_num_of_highest_magnitude / secs_to_sample);
}
