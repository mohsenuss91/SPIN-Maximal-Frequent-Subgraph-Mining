#include <iostream>
#include <boost/geometry.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <string.h>
#include <dirent.h>
#include <vector>
using namespace std;
int main()
{
	/*
	   Basically all the other functions will be called from main.
	 */ 
	DIR *dir;
	struct dirent *ent;
    vector <string> dirContents;
	if ((dir = opendir ("Input")) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			if(std::strcmp(ent->d_name,".") ==0)
                ;
            else if(std::strcmp(ent->d_name,"..") == 0)
                ;
            else
                dirContents.push_back(ent->d_name);
		}
	closedir (dir);
    std::vector<string>::iterator i;
    for(i=dirContents.begin();i!=dirContents.end();i++)
        cout << *i << " " ;
    cout << endl;
	} 
	else {
		/* could not open directory */
		perror ("");
		return EXIT_FAILURE;
	}
	return 0;
}
