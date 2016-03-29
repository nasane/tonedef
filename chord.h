/*
 *  chord.h
 *
 *  Copyright (C) 2016  Nathan Bossart
 */

#ifndef CHORD_H
#define CHORD_H

#include "common.h"
#include <stdbool.h>

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
	ADD_NINE,  /* TODO: the "ADD" chords aren't fully named */
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

/* TODO: documentation */
struct chord
{
	enum chord_t	chord;
	enum semitone_t	tonic;
	enum semitone_t bass;
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

struct chord get_chord(struct note_node *node);

#endif
