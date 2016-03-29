/*
 *  chord.c
 *
 *  Copyright (C) 2016  Nathan Bossart
 */

#include <assert.h>
#include "chord.h"
#include "common.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include "utils.h"

static void			get_semitones_in_chord(struct note_node *node, enum semitone_t *semitones);
static int			compare_semitones(const void *a, const void *b);
static void			get_chord_tonic_and_type(enum semitone_t *semitones, enum chord_t *chord, enum semitone_t *tonic);
static void			rotate_semitones(enum semitone_t *semitones);
static enum chord_t		get_chord_type(enum semitone_t *semitones);
static bool			semitone_arrays_equal(enum semitone_t *ary1, enum semitone_t *ary2);
static enum semitone_t		get_tonic(enum semitone_t *semitones, enum chord_t chord_type);
static enum semitone_t *	get_semitones_for_chord_with_given_tonic(enum semitone_t tonic, enum chord_t chord_type);
static enum semitone_t		get_bass_note_in_chord(struct note_node *node);
static void			build_chord(enum semitone_t **chord, enum semitone_t tonic, int count, ...);

/*
 * A basic comparator for using qsort on an array of enum semitone_t.
 */
static int compare_semitones(const void *a, const void *b)
{
	assert(NULL != a);
	assert(NULL != b);

	if (* (enum semitone_t *) a < * (enum semitone_t *) b) {
		return -1;
	}

	if (* (enum semitone_t *) a > * (enum semitone_t *) b) {
		return 1;
	}

	return 0;
}

static enum semitone_t get_bass_note_in_chord(struct note_node *node)
{
	double lowest_freq;
	enum semitone_t lowest_semitone;

	assert(NULL != node);

	lowest_freq = get_freq(&(node->note));
	lowest_semitone = node->note.semitone;
	node = node->next;

	while (NULL != node) {

		if (get_freq(&(node->note)) < lowest_freq) {
			lowest_freq = get_freq(&(node->note));
			lowest_semitone = node->note.semitone;
		}

		node = node->next;
	}

	return lowest_semitone;
}

/*
 * This function is used to retrieve the an array of semitones present in the
 * link list of note_node represented by the "node" agrument.  The "node"
 * argument should represent the head of this list.  The return array is stored
 * in the semitones argument, which must already be allocated to hold a maximum
 * of SEMITONES_PER_OCTAVE + 1 (effectively 13) of enum semitone_t.  The return
 * array will be sorted and will be terminated by an UNKNOWN_SEMITONE upon
 * successful completion of this function.
 */
static void get_semitones_in_chord(struct note_node *node, enum semitone_t *semitones)
{
	bool	already_added;
	int	semitones_index;
	int	i;

	assert(NULL != node);
	assert(NULL != semitones);

	semitones_index = 0;
	while (NULL != node) {

		/*
		 * Make sure we haven't already added this semitone to our
		 * return array.  This implementation doesn't care how many
		 * Bb's are in your chord.
		 */
		already_added = false;
		for (i = 0; i < semitones_index; ++i) {
			if (node->note.semitone == semitones[i]) {
				already_added = true;
				break;
			}
		}

		if (!already_added) {
			semitones[semitones_index++] = node->note.semitone;
		}

		node = node->next;  /* will be NULL after the last node */
	}

	assert(semitones_index < SEMITONES_PER_OCTAVE + 1);

	/* sort the semitones based upon the semitone enumeration */
	qsort(semitones, semitones_index, sizeof(enum semitone_t), compare_semitones);

	/* terminate the array with an UNKNOWN_SEMITONE, like we said we would */
	semitones[semitones_index] = UNKNOWN_SEMITONE;
}
/*
 * This function rotates all the semitones in the given semitone array
 * (terminated by an UNKNOWN_SEMITONE) forward by one.  For example, given the
 * following array:
 *
 *         {Db, E, Gb, A, UNKNOWN_SEMITONE}
 *
 * This function will alter the array to look like:
 *
 *         {D, F, A, Bb, UNKNOWN_SEMITONE}
 *
 * Note that while the input array does not necessarily need to be sorted, it
 * will be sorted based upon the semitone_t enumeration upon return, and it will
 * still be terminated by an UNKNOWN_SEMITONE.
 */
static void rotate_semitones(enum semitone_t *semitones)
{
	int i;

	/* validate that the input array looks sane */
	assert(NULL != semitones);

	/* do the rotates */
	for (i = 0; semitones[i] != UNKNOWN_SEMITONE; ++i) {
		semitones[i] = (semitones[i] + 1) % SEMITONES_PER_OCTAVE;
	}

	/* put it back in order */
	qsort(semitones, i, sizeof(enum semitone_t), compare_semitones);
}

static bool semitone_arrays_equal(enum semitone_t *ary1, enum semitone_t *ary2)
{
	int i;

	assert(NULL != ary1);
	assert(NULL != ary2);

	for (i = 0; ary1[i] != UNKNOWN_SEMITONE; ++i) {
		if (ary1[i] != ary2[i]) {
			return false;
		}
	}

	if (UNKNOWN_SEMITONE == ary1[i] && UNKNOWN_SEMITONE == ary2[i]) {
		return true;
	}

	return false;
}

static enum chord_t get_chord_type(enum semitone_t *semitones)
{
	enum chord_t i;
	enum semitone_t *c_chord;

	assert(NULL != semitones);

	for (i = (enum chord_t) 0; i != UNKNOWN_CHORD_TYPE; i = (enum chord_t) ((int) i + 1)) {

		c_chord = get_semitones_for_chord_with_given_tonic(C, i);

		/* temp handling while all the chord enums aren't defined in get_c_chord(...) */
		//assert(NULL != c_chord);
		if (NULL == c_chord) {
			FREE_SAFELY(c_chord);
			continue;
		}

		if (semitone_arrays_equal(c_chord, semitones)) {
			FREE_SAFELY(c_chord);
			return i;
		}

		FREE_SAFELY(c_chord);
	}

	return UNKNOWN_CHORD_TYPE;
}

static void build_chord(enum semitone_t **chord, enum semitone_t tonic, int count, ...)
{
	va_list semitones;
	int i;

	assert(NULL != chord);
	assert(0 <= count);
	assert(C <= tonic && B >= tonic);

	va_start(semitones, count); /* Requires the last fixed parameter (to get the address) */
	for (i = 0; i < count; ++i) {
		*(*chord)++ = (va_arg(semitones, enum semitone_t) + (int) tonic) % SEMITONES_PER_OCTAVE;
	}
	va_end(semitones);
}

static enum semitone_t *get_semitones_for_chord_with_given_tonic(enum semitone_t tonic, enum chord_t chord_type)
{
	enum semitone_t *ret;
	enum semitone_t *i;

	/* TODO: make asserts more robust */
	assert(C <= tonic && B >= tonic);
	assert(UNKNOWN_CHORD_TYPE != chord_type);

	ret = (enum semitone_t *) MALLOC_SAFELY((SEMITONES_PER_OCTAVE + 1) * sizeof(enum semitone_t));
	i = ret;

	switch(chord_type) {

	case(MAJOR_TRIAD):
		build_chord(&i, tonic, 3, C, E, G);
		break;

	case(MINOR_TRIAD):
		build_chord(&i, tonic, 3, C, Eb, G);
		break;

	/* TODO: the notes in the Caug chord are the same as Eaug and G#aug */
	case(AUGMENTED_TRIAD):
		build_chord(&i, tonic, 3, C, E, Ab);
		break;

	case(DIMINISHED_TRIAD):
		build_chord(&i, tonic, 3, C, Eb, Gb);
		break;

	case(DIMINISHED_SEVENTH):
		build_chord(&i, tonic, 4, C, Eb, Gb, A);
		break;

	case(HALF_DIMINISHED_SEVENTH):
		build_chord(&i, tonic, 4, C, Eb, Gb, Bb);
		break;

	/* TODO: may be a major-add-6th */
	case(MINOR_SEVENTH):
		build_chord(&i, tonic, 4, C, Eb, G, Bb);
		break;

	case(MINOR_MAJOR_SEVENTH):
		build_chord(&i, tonic, 4, C, Eb, G, B);
		break;

	case(DOMINANT_SEVENTH):
		build_chord(&i, tonic, 4, C, E, G, Bb);
		break;

	case(MAJOR_SEVENTH):
		build_chord(&i, tonic, 4, C, E, G, B);
		break;

	case(AUGMENTED_SEVENTH):
		build_chord(&i, tonic, 4, C, E, Ab, Bb);
		break;

	case(AUGMENTED_MAJOR_SEVENTH):
		build_chord(&i, tonic, 4, C, E, Ab, B);
		break;

	case(DOMINANT_NINTH):
		build_chord(&i, tonic, 5, C, E, G, Bb, D);
		break;

	/* TODO: the third is usually omitted for this one */
	case(DOMINANT_ELEVENTH):
		build_chord(&i, tonic, 6, C, E, G, Bb, D, F);
		break;

	case(DOMINANT_THIRTEENTH):
		build_chord(&i, tonic, 7, C, E, G, Bb, D, F, A);
		break;

	/* TODO: this is the same as AUGMENTED_SEVENTH */
	case(SEVENTH_AUGMENTED_FIFTH):
		build_chord(&i, tonic, 4, C, E, Ab, Bb);
		break;

	case(SEVENTH_FLAT_NINTH):
		FREE_SAFELY(ret);
		return NULL;

	case(SEVENTH_SHARP_NINTH):
		FREE_SAFELY(ret);
		return NULL;

	case(SEVENTH_AUGMENTED_ELEVENTH):
		FREE_SAFELY(ret);
		return NULL;

	case(SEVENTH_FLAT_THIRTEENTH):
		FREE_SAFELY(ret);
		return NULL;

	case(ADD_NINE):
		FREE_SAFELY(ret);
		return NULL;

	case(ADD_FOURTH):
		FREE_SAFELY(ret);
		return NULL;

	case(ADD_SIXTH):
		FREE_SAFELY(ret);
		return NULL;

	case(SIX_NINE):
		FREE_SAFELY(ret);
		return NULL;

	case(MIXED_THIRD):
		FREE_SAFELY(ret);
		return NULL;

	case(SUS2):
		FREE_SAFELY(ret);
		return NULL;

	case(SUS4):
		FREE_SAFELY(ret);
		return NULL;

	case(JAZZ_SUS):
		FREE_SAFELY(ret);
		return NULL;

	default:
		fprintf(stderr, "no rule for chord_type '%d'\n", chord_type);
		FREE_SAFELY(ret);
		return NULL;
	}

	qsort(ret, i - ret, sizeof(enum semitone_t), compare_semitones);
	*i = UNKNOWN_SEMITONE;

	return ret;
}

static enum semitone_t get_tonic(enum semitone_t *semitones, enum chord_t chord_type)
{
	enum semitone_t tonic;
	enum semitone_t *potential_chord_semitones;

	assert(NULL != semitones);
	assert((enum chord_t) 0 <= chord_type && UNKNOWN_CHORD_TYPE > chord_type);

	for (tonic = (enum semitone_t) 0; tonic <= B; tonic = (enum semitone_t) ((int) tonic + 1)) {

		potential_chord_semitones = get_semitones_for_chord_with_given_tonic(tonic, chord_type);
		assert(NULL != potential_chord_semitones);

		if (semitone_arrays_equal(potential_chord_semitones, semitones)) {
			FREE_SAFELY(potential_chord_semitones);
			return tonic;
		}

		FREE_SAFELY(potential_chord_semitones);
	}

	return UNKNOWN_SEMITONE;
}

/* TODO: this should return a list of possibilities */
static void get_chord_tonic_and_type(enum semitone_t *semitones, enum chord_t *chord, enum semitone_t *tonic)
{
	int i;
	enum semitone_t *semitones_cpy;

	/* TODO: much more robust assertions */
	assert(NULL != semitones);
	assert(NULL != chord);
	assert(NULL != tonic);

	*chord = UNKNOWN_CHORD_TYPE;
	*tonic = UNKNOWN_SEMITONE;

	i = -1;
	while (semitones[++i] != UNKNOWN_SEMITONE);  /* count the elements */
	qsort(semitones, i, sizeof(enum semitone_t), compare_semitones);

	semitones_cpy = (enum semitone_t *) MALLOC_SAFELY((SEMITONES_PER_OCTAVE + 1) * sizeof(enum semitone_t));
	for (i = 0; semitones[i] != UNKNOWN_SEMITONE; ++i) {
		semitones_cpy[i] = semitones[i];
	}
	semitones_cpy[i] = UNKNOWN_SEMITONE;

	i = 0;
	while (i < SEMITONES_PER_OCTAVE && UNKNOWN_CHORD_TYPE == get_chord_type(semitones)) {
		rotate_semitones(semitones);
		++i;
	}

	if (UNKNOWN_CHORD_TYPE == get_chord_type(semitones)) {
		return;
	}

	assert(i < SEMITONES_PER_OCTAVE);

	*chord = get_chord_type(semitones);

	/* TODO: get tonic correctly */
	*tonic = get_tonic(semitones_cpy, *chord);
	assert(UNKNOWN_SEMITONE != *tonic);
}

/*
 * This function retrieves the chord represented by "node" (the head of a list
 * of note nodes).  If node is an illegal argument or the chord cannot be
 * determined, an invalid struct chord is returned.  This struct will contain
 * the UNKNOWN_* enumerations for each of its members.
 *
 * I try not to alter the node list passed in, but I make no promises unless
 * "const" is specified.
 */
struct chord get_chord(struct note_node *node)
{
	struct chord	ret;
	enum semitone_t	semitones[SEMITONES_PER_OCTAVE + 1];

	/* initialize return chord to error values in case something goes wrong */
	ret.chord = UNKNOWN_CHORD_TYPE;
	ret.tonic = UNKNOWN_SEMITONE;
	ret.bass  = UNKNOWN_SEMITONE;

	/* basic argument validation */
	if (NULL == node) {
		fprintf(stderr, "node cannot be NULL\n");
		return ret;
	}

	/*
	 * First, we put the semitones of the chord into a sorted array.  This
	 * array will be terminated with an UNKNOWN_SEMITONE, and can have a
	 * maximum of SEMITONES_PER_OCTAVE + 1 elements (hence the declaration
	 * at the beginning of this function.
	 */
	get_semitones_in_chord(node, semitones);

	/* TODO: document */
	ret.bass = get_bass_note_in_chord(node);

	/*
	 * Next, we do the real work (AKA finding the chord type and tonic).
	 * The chord type is discovered first by shifting all the semitones and
	 * checking for a match with a known C chord.  Once we know the chord
	 * type, we try all possible tonic notes (this is at most 12).
	 *
	 * If we are able to match a chord, then we _should_ be able to match a
	 * tonic note, since they both use
	 * get_semitones_for_chord_with_given_tonic(...)!
	 */
	get_chord_tonic_and_type(semitones, &(ret.chord), &(ret.tonic));

	/*
	 * If once of our members is still UNKNOWN_* somehow, we must make them
	 * all UNKNOWN_*.
	 */
	 if (UNKNOWN_CHORD_TYPE == ret.chord || UNKNOWN_SEMITONE == ret.tonic || UNKNOWN_SEMITONE == ret.bass) {
		 ret.chord = UNKNOWN_CHORD_TYPE;
		 ret.tonic = UNKNOWN_SEMITONE;
		 ret.bass  = UNKNOWN_SEMITONE;
	 }

	return ret;
}
