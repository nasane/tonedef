#ifndef TONEDEF_H
#define TONEDEF_H

#define OCTAVE_MIN		0   /* lowest octave number accepted by this library */
#define OCTAVE_MAX		12  /* highest octave number accepted by this library */
#define SEMITONES_PER_OCTAVE	12  /* num of notes per octave in twelve-tone equal temperament */
#define FREQ_OF_A4		440 /* standard frequency of the A4 note as defined in ISO 16:1975 */
#define NOTE_NUM_OF_A4		49  /* the note number of A4 on an 88-key piano */
#define OCTAVE_NOTE_NUM_OFFSET	8   /* the octave base note number offset from multiples of twelve */
#define OCTAVE_JUST_INTERVAL	2   /* the just interval ratio of perfect octaves in Western music */
#define SEMITONE_INTERVAL_CENTS	100

/*
 * The half step enumeration defines the difference
 * in semitones of the note from the base note in the
 * octave.  For example, the note E is 4 semitones
 * higher than the octave base note (i.e. C).
 */
enum semitone_t
{
	UNKNOWN_SEMITONE = -1,
	C,
	Db,
	D,
	Eb,
	E,
	F,
	Gb,
	G,
	Ab,
	A,
	Bb,
	B
};

/* TODO: documentation */
struct note
{
	enum semitone_t	semitone;
	int		octave;
	double		cents;
};

/* functions provided by this library */
double get_freq(const struct note * const note);
struct note get_approx_note(double freq);
struct note get_exact_note(double freq);
enum semitone_t get_semitone(char * const semitone);
char *get_semitone_str(enum semitone_t semitone);

#endif
