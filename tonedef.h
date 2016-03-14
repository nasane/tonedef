/*
 *  tonedef.h
 *
 *  Copyright (C) 2016  Nathan Bossart
 */

#ifndef TONEDEF_H
#define TONEDEF_H

#include <fftw3.h>

/* lowest octave number accepted */
#define OCTAVE_MIN		0

/* highest octave number accepted */
#define OCTAVE_MAX		8

/* notes/octave in 12-tone equal temperament */
#define SEMITONES_PER_OCTAVE	12

/* standard freq of A4 as defined in ISO 16:1975 */
#define FREQ_OF_A4		440

/* note number of A4 on an 88-key piano */
#define NOTE_NUM_OF_A4		49

/* octave base note offset from multiples of 12 */
#define OCTAVE_NOTE_NUM_OFFSET	8

/* just interval ratio of perfect octaves */
#define OCTAVE_JUST_INTERVAL	2

/* number of cents in a half-step */
#define SEMITONE_INTERVAL_CENTS	100

/* return value to indicate an error in determining a frequency */
#define INVALID_FREQUENCY	-1.0

/* val for octave member of note struct to indicate an err determining a note */
#define INVALID_OCTAVE		-1

/* val for cents member of note struct to indicate an err determining a note */
#define INVALID_CENTS		-255.0

/* some platforms don't define the M_PI macro for whatever reason */
#ifndef M_PI
#define M_PI			3.14159265358979323846264338327950288
#endif

/* failure code to exit with if we hit a critical error */
#define EXIT_FAILURE_CODE	1

/* return codes for the split_stereo_channels() function */
#define SPLIT_STEREO_CHANNELS_SUCCESS_CODE	0
#define SPLIT_STEREO_CHANNELS_FAILURE_CODE	-1

/* number of channels in stereo audio */
#define STEREO_NUM_CHANNELS	2

/*
 * The half step enumeration defines the difference in semitones of the note
 * from the base note in the octave.  For example, the note E is 4 semitones
 * higher than the octave base note (i.e. C).
 */
enum semitone_t
{
	UNKNOWN_SEMITONE = -1, C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B
};

/* TODO: documentation, put in separate file */
enum chord_t
{
	MAJOR_TRIAD,
	MINOR_TRIAD,
	AUGMENTED_TRIAD,
	DIMINISHED_TRIAD,
	DIMINISHED_SEVENTH,
	HALF_DIMINISHED_SEVENTH,
	MINOR_SEVENTH,
	MINOR_MAJOR_SEVENTH,
	DOMINANT_SEVENTH,
	MAJOR_SEVENTH,
	AUGMENTED_SEVENTH,
	AUGMENTED_MAJOR_SEVENTH,
	DOMINANT_NINTH,
	DOMINANT_ELEVENTH,
	DOMINANT_THIRTEENTH,
	SEVENTH_AUGMENTED_FIFTH,
	SEVENTH_FLAT_NINTH,
	SEVENTH_SHARP_NINTH,
	SEVENTH_AUGMENTED_ELEVENTH,
	SEVENTH_FLAT_THIRTEENTH,
	ADD_NINE,
	ADD_FOURTH,
	ADD_SIXTH,
	SIX_NINE,
	MIXED_THIRD,
	SUS2,
	SUS4,
	JAZZ_SUS,
	UNKNOWN_CHORD_TYPE
	/* TOOD: this is not yet a complete list */
};

/*
 * A basic struct for a musical tone.
 *
 * semitone : the half-step of the note
 * octave   : the octave of the note
 * cents    : the cents away from the ideal tone of the note (-50.0 to +50.0)
 */
struct note
{
	enum semitone_t	semitone;
	int		octave;
	double		cents;
};

/*
 * A lightweight wrapper for building a singly-linked list of notes.
 *
 * note : the encapsulated note
 * next : the next node in the list (NULL if we're at the end)
 */
struct note_node
{
	struct note		note;
	struct note_node *	next;
};

/* TODO: documentation */
struct chord
{
	enum chord_t	chord;
	enum semitone_t	tonic;
	/* TODO: other fields, such as the "over" note for inverted chords */
};

/* functions provided by this library */
double		get_freq(const struct note * const note);
struct note	get_approx_note(double freq);
struct note	get_exact_note(double freq);
enum semitone_t	get_semitone(const char * const semitone);
char		*get_semitone_str(enum semitone_t semitone, bool prefer_flat);
enum semitone_t	get_fifth(enum semitone_t semitone);
enum semitone_t	get_fourth(enum semitone_t semitone);
enum semitone_t	*get_major_scale(enum semitone_t tonic);
enum semitone_t	*get_natural_minor_scale(enum semitone_t tonic);
enum semitone_t	*get_harmonic_minor_scale(enum semitone_t tonic);
enum semitone_t	*get_melodic_minor_scale(enum semitone_t tonic);
enum semitone_t	*get_chromatic_scale(enum semitone_t tonic);
double		*get_samples_from_file(const char * const filename, long samples_requested, long *samples_returned);
double		*apply_hann_function(const double * const samples, long num_samples);
int		split_stereo_channels(const double * const samples, long num_samples, double **chan1, double **chan2);
fftw_complex	*get_fft(double *samples, long num_samples);
struct note	get_note_from_file(const char * const filename, double secs_to_sample);
struct chord	get_chord(struct note_node *node);

#endif
