#include <stdio.h>
#include <stdlib.h>

#define BIT_NZ(word,bitN) ((word) & (1<<(bitN))) > 0

#define rev_end_16_bit(n) \
	( ((((unsigned int) n) << 0x08) & 0xFF00) | \
      ((((unsigned int) n) >> 0x08) & 0x00FF) )

#define rev_end_32_bit(n) \
	( ((((unsigned long) n) << 24) & 0xFF000000) |	\
	  ((((unsigned long) n) <<  8) & 0x00FF0000) |	\
	  ((((unsigned long) n) >>  8) & 0x0000FF00) |	\
	  ((((unsigned long) n) >> 24) & 0x000000FF) )

#define note_ms_length 3840.0

#define MIDI_NOTE_C 0
#define MIDI_NOTE_CSHARP 1
#define MIDI_NOTE_D 2
#define MIDI_NOTE_DSHARP 3
#define MIDI_NOTE_E 4
#define MIDI_NOTE_F 5
#define MIDI_NOTE_FSHARP 6
#define MIDI_NOTE_G 7
#define MIDI_NOTE_GSHARP 8
#define MIDI_NOTE_A 9
#define MIDI_NOTE_ASHARP 10
#define MIDI_NOTE_B 11

#define DICE() (rand()%6) + 1

#define MIDDLE_OCTAVE 5

#define OCTAVE_FACTOR(o) (12*(o))

#define WHOLE_NOTE 1
#define QUARTER_NOTE 0.25*WHOLE_NOTE
#define HALF_NOTE 0.50*WHOLE_NOTE
#define EIGHTH_NOTE 0.125*WHOLE_NOTE
#define SIXTEENTH_NOTE 0.0625*WHOLE_NOTE

#define CHOOSE if (rand() % 2)

float notedur[] = {WHOLE_NOTE, HALF_NOTE, QUARTER_NOTE, EIGHTH_NOTE, SIXTEENTH_NOTE};

typedef struct CHUNK_HDR
{
   char id[4];
   unsigned long octetLength;
} CHUNK_HDR;

struct MThd
{
    CHUNK_HDR hdr;
    unsigned short fmt, nTrks, div;
} MainThud;

int lastFtell = 0;


void out_mainThud(FILE * f)
{
    MainThud.hdr.id[0] = 'M';
    MainThud.hdr.id[1] = 'T';
    MainThud.hdr.id[2] = 'h';
    MainThud.hdr.id[3] = 'd';

    /* 3 shorts (2 octets each) */
    MainThud.hdr.octetLength = rev_end_32_bit(6);

    /* Single track, >=1 channels. */
    MainThud.fmt = 0;

    /* And yes, just one track! */
    MainThud.nTrks = rev_end_16_bit(1);

    /* Possibly we're asking for millisecond time-deltas. */
    MainThud.div = rev_end_16_bit(0x03c0);

    fwrite(MainThud.hdr.id, 1 , 4 , f );

    fwrite(&MainThud.hdr.octetLength, 1 , 4 , f );
    fwrite(&MainThud.fmt, 1 , 2 , f );
    fwrite(&MainThud.nTrks, 1 , 2 , f );
    fwrite(&MainThud.div, 1 , 2 , f );



}

void out_MTrk(FILE * f, unsigned long length)
{

    CHUNK_HDR trackHdr;

    trackHdr.id[0] = 'M';
    trackHdr.id[1] = 'T';
    trackHdr.id[2] = 'r';
    trackHdr.id[3] = 'k';

    trackHdr.octetLength = rev_end_32_bit(length);

    fwrite(trackHdr.id, 1 , 4 , f );

    fwrite(&trackHdr.octetLength, 1 , 4 , f );

    lastFtell = ftell(f);

}

/* via http://home.roadrunner.com/~jgglatt/tech/midifile.htm */
void WriteVarLen(FILE * f, unsigned long val)
{
   unsigned long buffer;
   buffer = val & 127;

   while ( (val >>= 7) )
   {
     buffer <<= 8;
     buffer |= ((val & 127) | 128);
   }

   while (1)
   {
      putc(buffer,f);
      if (buffer & 128)
          buffer >>= 8;
      else
          break;
   }
}

void out_generic_event(FILE* f, unsigned long timeDelta, char status, char * bytes, int nbytes)
{
    WriteVarLen(f, timeDelta);
    fwrite(&status, 1 , 1 , f );
    fwrite(bytes, 1 , nbytes , f );
}

void out_note_on(FILE * f, char noteNumber, char velocity, long ms)
{
    char bytes[] = {noteNumber,velocity};
    out_generic_event(f, ms, 0x90, bytes, 2);
}

void out_note_off(FILE * f, char noteNumber, char velocity, long ms)
{
    char bytes[] = {noteNumber,velocity};
    out_generic_event(f, ms, 0x80, bytes, 2);
}

void out_note(FILE * f, char noteNumber, char velocity, float fraction)
{
    int ms_length = note_ms_length * fraction;
    out_note_on(f, noteNumber, velocity, 0);
    out_note_off(f, noteNumber, velocity, ms_length);

    printf("\nPlacing a %d ms duration note with MIDI note number %d.", ms_length, noteNumber);
}


void out_end_of_track(FILE * f)
{
    char bytes[] = {0x2F,0x00};
    out_generic_event(f, 0, 0xFF, bytes, 2);
}

void randomChromaOctaveNotes(FILE * f, int n )
{
    int i;
    for (i = 0; i < n; i++)
    {
        out_note(f, (rand() % 35) + OCTAVE_FACTOR(MIDDLE_OCTAVE-1), 100, notedur[(rand() % 4)+1]);
    }


}

void vossOctaveNotes(FILE * f, int n)
{
    int noten = 1;

    int r,g,b;

    r = DICE();
    g = DICE();
    b = DICE();

    out_note(f, r + g + b+ OCTAVE_FACTOR(MIDDLE_OCTAVE-1), 100, notedur[(rand() % 4)+1]);

    int i;
    for (i = 0; i < n; i++)
    {

       if (BIT_NZ(noten,0))
            b = DICE();
       if (BIT_NZ(noten,1))
            g = DICE();
       if (BIT_NZ(noten,2))
            r = DICE();

       out_note(f, r + g + b+ OCTAVE_FACTOR(MIDDLE_OCTAVE-1), 100, notedur[(rand() % 4)+1]);

       noten++;

       if (noten > 7)
       noten = 0;

    }




}

void brownChromaOctaveNotes(FILE * f, int n )
{
    int i;
    int lastNote = (rand() % 35);
    for (i = 0; i < n; i++)
    {
        lastNote = lastNote + (rand() % 2) ? (rand() % 6) : -(rand() % 6);
        out_note(f, lastNote + OCTAVE_FACTOR(MIDDLE_OCTAVE-1), 100, notedur[(rand() % 4)+1]);
    }


}

/* For now, we'll comment out the rests and not consider them.*/
void pinkerton(FILE * f)
{
    int beats, choice;
    beats = 0;

    int starts = 0;

    BRANCH_A_ONE: beats++; starts++;
    out_note(f, MIDI_NOTE_C + OCTAVE_FACTOR(MIDDLE_OCTAVE), 100, notedur[(rand() % 4)+1]);
    CHOOSE goto BRANCH_B_TWO;

    BRANCH_A_TWO: beats++;
    out_note(f, MIDI_NOTE_C + OCTAVE_FACTOR(MIDDLE_OCTAVE), 100, notedur[(rand() % 4)+1]);
    CHOOSE goto BRANCH_B_THREE;

    BRANCH_A_THREE: beats++;
    out_note(f, MIDI_NOTE_C + OCTAVE_FACTOR(MIDDLE_OCTAVE),100, notedur[(rand() % 4)+1]);
    CHOOSE goto CENTRAL_G;

    BRANCH_A_FOUR: beats++;
    out_note(f, MIDI_NOTE_C + OCTAVE_FACTOR(MIDDLE_OCTAVE), 100, notedur[(rand() % 4)+1]);

    BRANCH_A_FIVE: //beats++;
    //out_note(f, 0, 0, notedur[(rand() % 4)+1]);

    IS_THIRD_BAR:

    if (starts > 2) goto END;

    CHOOSE goto BRANCH_B_TERMINAL;
    BRANCH_A_TERMINAL: beats++;
     out_note(f, MIDI_NOTE_D + OCTAVE_FACTOR(MIDDLE_OCTAVE), 100, notedur[(rand() % 4)+1]);

    goto BRANCH_A_ONE;

    BRANCH_B_TERMINAL: beats++;
        out_note(f, MIDI_NOTE_C + OCTAVE_FACTOR(MIDDLE_OCTAVE), 100, notedur[(rand() % 4)+1]);

    goto BRANCH_A_ONE;

    BRANCH_B_ONE: beats++; starts++;
        out_note(f, MIDI_NOTE_D + OCTAVE_FACTOR(MIDDLE_OCTAVE), 100, notedur[(rand() % 4)+1]);

    BRANCH_B_TWO: //beats++;
        //out_note(f, 0, 0, notedur[(rand() % 4)+1]);


    BRANCH_B_THREE: //beats++;
       // out_note(f, 0, 0, notedur[(rand() % 4)+1]);
    CHOOSE goto CENTRAL_G;


    BRANCH_B_FOUR: beats++;
        out_note(f, MIDI_NOTE_D + OCTAVE_FACTOR(MIDDLE_OCTAVE), 100, notedur[(rand() % 4)+1]);

    BRANCH_B_FIVE: //beats++;
        //out_note(f, 0,0, notedur[(rand() % 4)+1]);

    goto IS_THIRD_BAR;

    CENTRAL_G: beats++;
        out_note(f, MIDI_NOTE_G + OCTAVE_FACTOR(MIDDLE_OCTAVE), 100, notedur[(rand() % 4)+1]);
    CHOOSE goto BRANCH_B_FIVE;

    CENTRAL_E: beats++;
        out_note(f, MIDI_NOTE_E + OCTAVE_FACTOR(MIDDLE_OCTAVE), 100, notedur[(rand() % 4)+1]);
    goto IS_THIRD_BAR;



    END:
    out_note(f, MIDI_NOTE_D + OCTAVE_FACTOR(MIDDLE_OCTAVE), 100, notedur[(rand() % 4)+1]);
    out_note(f, MIDI_NOTE_C + OCTAVE_FACTOR(MIDDLE_OCTAVE), 100, notedur[(rand() % 4)+1]);


}

int main(int argc, char **argv)
{
   printf("\n\n\"Banal Tune\" Generator \n(Burton 2013 a la Pinkerton 1956, as described by Johnston 2009)\n");

   if (argc < 2)
   {
       printf("\n\nUsage: pinkerton banaltune.midi <r/b/v>\n");
       exit(EXIT_FAILURE);
   }

   srand(time(0));

   FILE * f = fopen(argv[1], "wb");

   if (f == NULL)
   {
       printf("\n\nCan't get a handle on: %s\n", argv[1]);
       exit(EXIT_FAILURE);
   }

   out_mainThud(f);
   out_MTrk(f,10);


   if (argc < 3)
   {
      pinkerton(f);
   }

   else
   {
       switch(argv[2][0])
       {
           case 'r':
              puts("WHITE/RANDOM");
              randomChromaOctaveNotes(f,13);
           break;
           case 'b':
              puts("BROWNIAN");
              brownChromaOctaveNotes(f, 13);
           break;
           case 'v':
              puts("VOSS");
              vossOctaveNotes(f,13);
           break;
           default:
                puts("PINKERTON");
                pinkerton(f);

       }

   }

   out_end_of_track(f);

   unsigned long trkDist = rev_end_32_bit(ftell(f) - lastFtell);

   fseek(f, lastFtell-4 ,SEEK_SET);
   fwrite(&trkDist, 1 , 4 , f );

   fclose(f);

   printf("\n\nMIDI file generated: %s\n", argv[1]);
   return 0;
}
