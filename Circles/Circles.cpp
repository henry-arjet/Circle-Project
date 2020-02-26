#include <iostream>
#include <fstream>


#define WIDTH 100
#define HEIGHT 100

using std::ios;

int w = 0;
int h = 0;


int main()
{
	std::ofstream results;
	results.open("results.txt", std::ios::out | std::ios::trunc);

	char fb[WIDTH][HEIGHT];

	results << fb[0][0];
	
	results.close();


}
