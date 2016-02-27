#include <stdio.h>
#include <stdlib.h>
#include "tonedef.h"

int main(int argc, const char *argv[]) {

	struct note exact_note;
	struct note approx_note;

	exact_note = get_exact_note(strtod(argv[1], NULL));
	printf("%s %d %+f cents\n",
		get_semitone_str(exact_note.semitone),
		exact_note.octave,
		exact_note.cents);

	printf("-----\n");

	approx_note = get_approx_note(strtod(argv[1], NULL));
	printf("%s %d\n",
		get_semitone_str(approx_note.semitone),
		approx_note.octave);

	return 0;
}
