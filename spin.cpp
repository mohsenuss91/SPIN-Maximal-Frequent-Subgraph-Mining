#include <iostream>
#include <boost/geometry.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <string>
#include <dirent.h>
using namespace std;
int main()
{
	/*
	   Basically all the other functions will be called from main.
	 */ 
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir ("Input")) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			printf ("%s\n", ent->d_name);
		}
		closedir (dir);
	} 
	else {
		/* could not open directory */
		perror ("");
		return EXIT_FAILURE;
	}
	return 0;
}
