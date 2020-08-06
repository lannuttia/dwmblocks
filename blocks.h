//Modify this file to change what commands output to your statusbar, and recompile using the make command.
static const Block blocks[] = {
	/*Icon*/	/*Command*/			/*Update Interval*/	/*Update Signal*/
    {"",		"moonphase-block",	18000,				3},
    {"",		"weather-block",	18000,				2},
    {"",		"internet-block",	5,					0},
    {"",		"clock-block",		60,					1},
};

//sets delimeter between status commands. NULL character ('\0') means no delimeter.
static char *delim = " | ";
