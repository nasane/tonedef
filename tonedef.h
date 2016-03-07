/*
 *  tonedef.h
 *
 *  Copyright (C) 2016  Nathan Bossart
 */

#ifndef TONEDEF_H
#define TONEDEF_H

/* lowest octave number accepted */
#define OCTAVE_MIN		0

/* highest octave number accepted */
#define OCTAVE_MAX		12

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

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288
#endif

/*
 * The half step enumeration defines the difference in semitones of the note
 * from the base note in the octave.  For example, the note E is 4 semitones
 * higher than the octave base note (i.e. C).
 */
enum semitone_t
{
	UNKNOWN_SEMITONE = -1, C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B
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
enum semitone_t	*get_chromatic_scale(enum semitone_t tonic);
float		*get_samples_from_file(char *filename, long samples);
float		*apply_hann_function(const float * const samples, long num_samples);
void		split_stereo_channels(const float * const samples, long num_samples, float **chan1, float **chan2);

#endif
