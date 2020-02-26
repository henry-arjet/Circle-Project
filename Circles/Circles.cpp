#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <string>

#define WIDTH 100 //framebuffer size
#define HEIGHT 100
#define PI 3.14159265

using std::ios;
using std::vector;
using std::cout;
using std::endl;
using std::string;
using std::to_string;

class Vertex {
public:
	Vertex(short ix = 0, short  iy = 0) {
		x = ix, y = iy;
	}
	short x;
	short y;
	string str() { 
		string r = ("(" + to_string(x) +", " + to_string(y) + ")");
		return r;
	}
};

Vertex vertices[] = {
	{50, 50},
	{60, 50 }

};


int main()
{
	std::ofstream results;
	results.open("results.txt", std::ios::out | std::ios::trunc); //I'm not going to bother with drawing to a window,
		//just a janky .txt to hold the rasterization for now

	char fb[WIDTH][HEIGHT]; //frame buffer

	for (int i = 0; i < WIDTH; i++) {
		for (int j = 0; j < HEIGHT; j++) {
			fb[j][i] = ' ';
		}
	}


	int rad = abs(vertices[0].x - vertices[1].x) + abs(vertices[0].y - vertices[1].y);
	int octLen = std::ceilf(std::sinf(PI / 4) * rad); // gets the number of pixles in an octet
	vector<Vertex> octant(octLen);
	octant[0] = Vertex(rad, 0); //place the first point at rad,0 (no midpoint shifting for now, just drawing a very basic circle)
	for (short i = 1; i < octLen; i++) {
		Vertex p;
		//useing midpoint algorithm
		//octant from 0 - pi/4
		p.y = octant[i - 1].y + 1;
		p.x = octant[i - 1].x;
		float d = powf(p.y, 2) + powf(p.x - 0.5, 2); //decision variable
		if (d > powf(rad, 2)) { // midpoint is outside circle
			p.x -= 1; //shift x point left
		}
		octant[i] = p;
	}

	vector<Vertex> circle(octLen * 8); //Now lets copy the octant eight times
	for (short i = 0; i < octLen; i++){
		short ci = i * 8;//index for the circle, output of this loop
		circle[ci].x = octant[i].x; //oct 1 (x, y)
		circle[ci].y = octant[i].y;

		circle[ci + 1].x = octant[i].y; //oct 2 (y, x)
		circle[ci + 1].y = octant[i].x;

		circle[ci + 2].x = octant[i].y*-1; //oct 3 (y, -x)
		circle[ci + 2].y = octant[i].x;


		circle[ci + 3].x = octant[i].x*-1; //oct 4 (-x, y)
		circle[ci + 3].y = octant[i].y;

		circle[ci + 4].x = octant[i].x*-1; //oct 5 (-x, -y)
		circle[ci + 4].y = octant[i].y*-1;

		circle[ci + 5].x = octant[i].y*-1; //oct 6 (-y, -x)
		circle[ci + 5].y = octant[i].x*-1;

		circle[ci + 6].x = octant[i].y; //oct 7 (-y, x)
		circle[ci + 6].y = octant[i].x*-1;

		circle[ci + 7].x = octant[i].x; //oct 8 (x, -y)
		circle[ci + 7].y = octant[i].y*-1;
	}

	for (short i = 0; i < circle.size(); i++) { // go ahead and place it in normalized coordinates


		cout << circle[i].x << endl;
		circle[i].x += vertices[0].x;
		circle[i].y += vertices[0].y;
	}

	for (Vertex v : circle) { // draw to framebuffer

		fb[v.x][v.y] = 'c';
	}
	for (short i = HEIGHT - 1; i > -1; i--) { //print framebuffer
		for (short j = 0; j < WIDTH; j++) {
			results << fb[j][i];
		}
		results << endl;
	}

	results.close();

}
