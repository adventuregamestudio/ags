//
// Allegro 4.4.2 and earlier do not have a variant of MIDI
// constructor that takes PACKFILE stream as parameter. Hence our own
// version. This should be removed when either Allegro 4 gets patched
// or we switch to another backend library.
//
// Much of the code is plain copied from load_midi Allegro's function.
//

#include <allegro.h>

#if (ALLEGRO_DATE < 20190303)

#include <allegro/internal/aintern.h>

/* load_midi_pf:
 *  Reads a standard MIDI file from the packfile given, returning a MIDI
 *  structure, or NULL on error.
 *
 *  If unsuccessful the offset into the file is unspecified, i.e. you must
 *  either reset the offset to some known place or close the packfile. The
 *  packfile is not closed by this function.
 */
MIDI *load_midi_pf(PACKFILE *fp)
{
   int c;
   char buf[4];
   long data;
   MIDI *midi;
   int num_tracks;
   ASSERT(fp);

   midi = _AL_MALLOC(sizeof(MIDI));              /* get some memory */
   if (!midi)
      return NULL;

   for (c=0; c<MIDI_TRACKS; c++) {
      midi->track[c].data = NULL;
      midi->track[c].len = 0;
   }

   pack_fread(buf, 4, fp); /* read midi header */

   /* Is the midi inside a .rmi file? */
   if (memcmp(buf, "RIFF", 4) == 0) { /* check for RIFF header */
      pack_mgetl(fp);

      while (!pack_feof(fp)) {
         pack_fread(buf, 4, fp); /* RMID chunk? */
         if (memcmp(buf, "RMID", 4) == 0) break;

         pack_fseek(fp, pack_igetl(fp)); /* skip to next chunk */
      }

      if (pack_feof(fp)) goto err;

      pack_mgetl(fp);
      pack_mgetl(fp);
      pack_fread(buf, 4, fp); /* read midi header */
   }

   if (memcmp(buf, "MThd", 4))
      goto err;

   pack_mgetl(fp);                           /* skip header chunk length */

   data = pack_mgetw(fp);                    /* MIDI file type */
   if ((data != 0) && (data != 1))
      goto err;

   num_tracks = pack_mgetw(fp);              /* number of tracks */
   if ((num_tracks < 1) || (num_tracks > MIDI_TRACKS))
      goto err;

   data = pack_mgetw(fp);                    /* beat divisions */
   midi->divisions = ABS(data);

   for (c=0; c<num_tracks; c++) {            /* read each track */
      pack_fread(buf, 4, fp);                /* read track header */
      if (memcmp(buf, "MTrk", 4))
	 goto err;

      data = pack_mgetl(fp);                 /* length of track chunk */
      midi->track[c].len = data;

      midi->track[c].data = _AL_MALLOC_ATOMIC(data); /* allocate memory */
      if (!midi->track[c].data)
	 goto err;
					     /* finally, read track data */
      if (pack_fread(midi->track[c].data, data, fp) != data)
	 goto err;
   }

   lock_midi(midi);
   return midi;

   /* oh dear... */
   err:
   destroy_midi(midi);
   return NULL;
}

#endif // (ALLEGRO_DATE < 20190303)
