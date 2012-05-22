#include <iostream>
#include <string>

#include "Texture.h"


using namespace std;

int main(int argc, char *argv[])
{
#ifdef __APPLE__
	string path(argv[0]);
	path = path.substr(0,path.find_last_of("/")+1);
	printf("executable path: %s\n",path.c_str());
	
	chdir( (path + "../Resources/").c_str() );
#endif
	char buf[512];
	getcwd(buf, 512);
	printf("current dir: %s\n",buf);

        gl::Texture tex;


    return 0;
}
