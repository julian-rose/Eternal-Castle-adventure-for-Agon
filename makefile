# ----------------------------
# Makefile Options
# ----------------------------


DESCRIPTION = "Eternal Castle Adventure"
COMPRESSED = NO

_z80 = 0
_eZ80 = 1

ifeq ($(_z80),1)
	#The following CFLAGS will produce Z80 output, but not eZ80 Z80-mode
	NAME = Eternal80
	CFLAGS = -Wall -Wextra -Oz -std=c89 -v -fverbose-asm  --target=z80 -D__Z80__
else 
ifeq ($(_eZ80),1)
	#The following produces eZ80 ADL output (by default)
	NAME = Eternalez80
	CFLAGS = -Wall -Wextra -Oz -std=c89 -v -fverbose-asm -D__EZ80__
else
	#Original
	NAME = Eternal
	CFLAGS = -Wall -Wextra -Oz
endif
endif
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
