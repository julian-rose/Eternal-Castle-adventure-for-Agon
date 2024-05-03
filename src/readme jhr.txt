Eternal Castle dictionary works using 4-byte 'words', or 32-bit numbers.
But, eZ80 is only 24-bit data width.
So all ints or long ints need to become U32s in cygwin and AgDev.

