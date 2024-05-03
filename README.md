# Eternal-Castle-adventure-for-Agon
Eternal Castle text adventure for Agon Light<p>


About:<br>
Eternal Castle is a text adventure in the style of Scott Adams. <br>
Compared to those earlier games, it has an extended dictionary so you can type in multi-word phrases (the limit is five words) rather than the usual verb-noun pairing.<br>
And each word is unique to 4 characters (rather than the usual 3) of those earlier games.<p>

Game objectives:<br>
Save the virtual world.<p>

Story:<br>
The story is in the style of a fantasy novel - like sci-fi, but set in an idealised past instead of the future.<br>
Scott Adams adventures can typically be completed in a sequence of 30-40 moves.<br>
Whereas Eternal Castle is probably over 100 moves.<p>
I can't remember much detail about the world or the mission specifically, including how to win.
So you'll have to waste time finding out...<p>

History:<br>
Eternal Castle was written in the summer of 2015, targetting cygwin in C.<br>
A year or so later it was built as a Java native back-end; and given a graphical front-end running on Android. <br>
Now in 2024, it serves as an exercise in porting 32-bit cygwin applications to 24-bit EZ80 ADL mode.<br>

How to run (on Agon):<br>
1. Copy the binary file Eternalez80.bin to your SD-card. This is found in your AgDev/user/Eternal-Castle-adventure-for-Agon/bin/ sub-directory.<br>
2. Insert your SD-card into your Agon Light, then power on.<br>
3. Issue the following commands from the MOS prompt:<br>
    'load Eternalez80.bin'<br>
    'run'<p>

How to build from source (using AgDev):<br>
The Eternal Castle source software is arranged to follow the AgDev structure, under the .../user sub-directory. 
This includes a Makefile.<br>
1. Clone the repository to a location under your AgDev/user/Eternal-Castle-adventure-for-Agon/<br>
2. cd to the repository, .../user/Eternal-Castle-adventure-for-Agon/<br>
3. type 'make'<br>
                        Note you will get a number of warnings: mostly to do with the disabled debugf, comma operator having no effect, and unused parameters; and -Wsign-compare on loop variants. Ignore these.<br>
                        The built image will be found under .../user/Eternal-Castle-adventure-for-Agon/bin/Eternalez80.bin<p>
