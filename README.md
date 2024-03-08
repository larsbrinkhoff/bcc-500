### Files from the BCC 500 computer at University of Hawaii

The tape file comes from http://www.bitsavers.org/bits/UniversityOfHawaii/BCC-500/

Each 24-bit word has been split in three 8-bit bytes.  To render the
data as somewhat readable ASCII text, each byte has been transformed
like this:

| Input range | Output range
| ---         | ---
| 000-137     | 040-177
| 140-177     | 000-037
| 200-377     | 200-377

(All numbers in the table are octal.)  The reason for this is that the
original character set is close to ASCII, but with all codes
subtracted with 32 (decimal).

The files in the `text` directory have been further processed to
expand spaces and use Unicode characters where appropriate.  This only
applies to the subset of files that can be considered to be text.
