#include <iostream>
#include <string>
#include <unordered_map>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

/*----MAIN_LOOP----*/
void loop();

/*---PARSING_FUNCTIONS---*/
char* sh_readline();
char** sh_splitline(char*);
int sh_execute(char**);

