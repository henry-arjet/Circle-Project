#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <string>
#include <BMP.h>
#include <glm/glm.hpp>

#define WIDTH 100 //framebuffer size.    depreciated.
#define HEIGHT 100
#define PI 3.14159265

using std::ios;
using std::vector;
using std::cout;
using std::endl;
using std::string;
using std::to_string;
using glm::vec2;
using glm::normalize;
using glm::dot;

struct Vertex {
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
	{256, 256}, // midpoint
	{367, 320}, // right side
	{211, 334}, // top

};

struct BMPpix {
	uint8_t b;
	uint8_t g;
	uint8_t r;
	bool isEqual(BMPpix other) {
		if (b == other.b && g == other.g && r == other.r) return true;
		else return false;
	}
};

class Rasterizer {
public:
	uint16_t width;
	uint16_t height;
	BMPpix fillColor = {0,0,0xff};
	vector<vector<BMPpix>> fb; //frame buffer
	Rasterizer(uint16_t width, 	uint16_t height) {
		this->height = height;
		this->width = width;
		fb.resize(width, vector<BMPpix>(height));
		resetFB();
	};
	void resetFB() {
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				fb[j][i] = BMPpix{ 0x50,0x50,0x50 };
			}
		}
	}

	void fill(uint16_t x, uint16_t y) { //takes the outline drawn in drawOutline and fills it in
		fb[x][y] = fillColor;

		if (!fb[x + 1][y].isEqual(fillColor)) {
			fill(x + 1, y);
		}if (!fb[x - 1][y].isEqual(fillColor)) {
			fill(x - 1, y);
		}if (!fb[x][y + 1].isEqual(fillColor)) {
			fill(x, y + 1);
		}if (!fb[x][y-1].isEqual(fillColor)) {
			fill(x, y-1);
		}
	}

	void write(std::string fileName) {
		//write framebuffer to bmp
		BMP bmp(width, -height);
		for (short i = 0; i < height; i++) {
			for (short j = 0; j < width; j++) {
				bmp.data[3 * (i*bmp.infoHeader.width + j) + 0] = fb[j][i].b;
				bmp.data[3 * (i*bmp.infoHeader.width + j) + 1] = fb[j][i].g;
				bmp.data[3 * (i*bmp.infoHeader.width + j) + 2] = fb[j][i].r;

			}
		}
		bmp.write(fileName);
	};
};

class CircleRasta : public Rasterizer {
public:
	int rad;
	int octLen;
	vector<Vertex> octant;
	CircleRasta(uint16_t width, uint16_t height):Rasterizer(width, height) {
	};
	void draw(Vertex vertices[2]) {
		rad = abs(vertices[0].x - vertices[1].x) + abs(vertices[0].y - vertices[1].y);
		octLen = std::ceil(std::sinf(PI / 4) * rad); // gets the number of pixles in an octet
		octant.resize(octLen+1);
		//using Bresenham's algorithm
		//octant from pi/2 -> pi/4
		short x = 0;
		short y = rad;
		short d = 3 - (2 * rad);
		short i = 0;
		while (y > x){
			Vertex p{ x, y };
			octant[i] = p;
			if (d < 0) {
				d += 4 * x + 6;
				x++;
			}
			else {
				d += 4 * (x - y) + 10;
				x++;
				y--;
			}
			i++;
		}
		octant[i] = Vertex{ x,y }; // so that it draws the final pixel.
			//Neccissary for the circle being closed
		octLen = i+1;

		octant.resize(i+1);
		vector<Vertex> circle(octLen * 8); //Now lets copy the octant eight times
		for (short i = 0; i < octLen; i++) {
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

			circle[i].x += vertices[0].x;
			circle[i].y += vertices[0].y;
		}

		for (Vertex v : circle) { // draw to framebuffer
			fb[v.x][v.y].r = 0xff;

		}
	}

};

class EllipseRasta : public Rasterizer {
public:
	int rad;
	int quadLen;
	vector<Vertex> quadrant;
	EllipseRasta(uint16_t width, uint16_t height) : Rasterizer(width, height) {
	};

	float findSlope(vec2 ei, vec2 ej, vec2 ij, int a2, int b2, float &djdi) {
		//takes unit vectors of i and j, ij coords of point, and squared coeficients
		//returns dy/dx at point i,j

		
		if (ij.y == 0) { djdi = -1 * ij.x * 99999.0f; cout << "ij.y = 0\n"; } //this is a way to preserve the original sign
		else djdi = (-1 * ij.x *b2) / (ij.y *a2); //if no div 0
		//cout << "js: " << ij.y << endl;
		vec2 projdj =  ej*djdi + ei * 1.0f; // projection of dj/di into x/y coords
		cout << "djdi: " << djdi << endl;
		float r;
		cout << "projdj.x: " << projdj.x << endl;
		cout << "projdj.y: " << projdj.y << endl;

		if (projdj.x == 0) r = projdj.y * 99999.0f; //same pattern
		else r = projdj.y / projdj.x; // dy/dx
		return r;
	}

	void drawOutlineZingl(Vertex vertices[3]) {

		//using modified Zingl algorithm
		//breaks, J is off

		vec2 dist1 = vec2(vertices[1].x - vertices[0].x, vertices[1].y - vertices[0].y);
		float a = glm::length(dist1);
		float a2 = a * a;
		vec2 ei = dist1 / a; //unit vector of i
		vec2 dist2 = vec2(vertices[2].x - vertices[0].x, vertices[2].y - vertices[0].y);
		float b = glm::length(dist2);
		float b2 = b * b;
		vec2 ej = dist2 / b; //unit vector of j
		cout << ei.x << "   " << ei.y << endl;
		cout << ej.x << "   " << ej.y << endl;
		
		int ellLength = ceil(5 * a + 5 * b); //because it draws some pixels twice, I want to give it a bit of padding
		vector<vec2>ellipse(ellLength);
		//vector<vec2>ellipse(16);

		//elipse b2 i2 + a2 j2 = b2 a2
		//scalar proj = a dot b /||b||
		//vector proj = (a dot b) * b/(b dot b)


		//midpoint = vec2(x,y)
		//i = mp dot ei
		//j = mp dot ej
		//d = b2i2 + a2j2 - b2a2

		//e = x2b2 + y2a2 - a2b2
		//e = b2i2 + a2j2 - b2a2
		//exy = (x+1)2b2 + (y+1)2a2 - a2b2 =(i + ei dot 1x)
		//yeah fuck it just brute force it
		//start at v2, look in from top-right to left

		//actually fuck that. Use tangents.
		//dj/dt = (ib2)/(ja2)

		short index = 0;
		string dir = "tr"; //holds the current direction
		ellipse[0] = dist1; // fills in virst pixel as vertex 2 (i know, confusing name)
		float i = dot(ellipse[0], ei); //i in xy coords
		float i2 = i * i; //man I don't even remember why I needed these
		float j = dot(ellipse[0], ej);
		float j2 = j * j;
		float djdi = 0.0f;//slope in djdi space
		/*float etr = abs(b2 * pow(dot(vec2(ellipse[0].x+1, ellipse[0].y+1), ei), 2) + a2 * pow(dot(vec2(ellipse[0].x + 1, ellipse[0].y + 1), ej), 2) - b2 * a2);
		float et = abs(b2 * pow(dot(vec2(ellipse[0].x, ellipse[0].y + 1), ei), 2) + a2 * pow(dot(vec2(ellipse[0], ellipse[0].y + 1), ej), 2) - b2 * a2);
		float etl = abs(b2 * pow(dot(vec2(ellipse[0].x - 1, ellipse[0].y + 1), ei), 2) + a2 * pow(dot(vec2(ellipse[0].x - 1, ellipse[0].y + 1), ej), 2) - b2 * a2);
		float el = abs(b2 * pow(dot(vec2(ellipse[0].x - 1, ellipse[0].y), ei), 2) + a2 * pow(dot(vec2(ellipse[0].x - 1, ellipse[0].y), ej), 2) - b2 * a2);
		float emin = etr;
		if (et < emin) { emin = et; dir = "t"; }
		if (etl < emin) { emin = etl; dir = "tl"; }
		if (el < emin) { emin = el; dir = "l"; }*/
		bool loop = true;
		glm::mat2 xyij = glm::inverse(glm::mat2(ei, ej)); // change of basis matrix
		vec2 ij = xyij * ellipse[0];
		float m = findSlope(ei, ej, ij, a2, b2, djdi);
		float djdi1 = djdi; //used to find stop
		string pass = "p1"; //short value that says whether it moves with or against the slope.
			//logs the sign of slope, and whether its the first pass or second with that sign
		if (m < 0) pass = "n1";
		string firstPass = pass; //used for stopping algorithm
		char startingOct = 0; // these three statements are all used for stopping
		bool leftStartingOct = false;
		bool lastLap = false;
		
		while (loop) {
			ij = xyij * ellipse[index];
			m = findSlope(ei, ej, ij, a2, b2, djdi);
			cout << "i: " << ij.x << endl;
			cout << "j: " << ij.y << endl;
			cout << "m: " << m << endl;
			cout << "index: " << index << endl;
			if (m < -1 && ij.y >= 0) { //octant I
				cout << "octant I\n";
				if (startingOct == 0) { startingOct = 1; }
				else if (startingOct == 1) {
					if (leftStartingOct) lastLap = true;
				}
				else {
					leftStartingOct = true;
					if (lastLap) loop = false; //ends loop if its passed all the way past the starting octant
				}
				float et = abs(b2 * pow(dot(vec2(ellipse[index].x, ellipse[index].y + 1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x, ellipse[index].y + 1), ej), 2) - b2 * a2); //error of the pixle to the top of the current one
				float etl = abs(b2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y + 1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y + 1), ej), 2) - b2 * a2);
				//cout << "et: " << et << endl;
				//cout << "etl: " << etl << endl;
				
				if (etl < et) dir = "tl"; // I should really replace these with enums. But whatevs
				else dir = "t";
				//now repeat this pattern 7 times. Joy.
			} else if (m >= -1 && m < 0 && ij.y >= 0) { //octant II
				if (startingOct == 0) { startingOct = 2; }
				else if (startingOct == 2) {
					if (leftStartingOct) lastLap = true;
				}
				else {
					leftStartingOct = true;
					if (lastLap) loop = false; //ends loop if its passed all the way past the starting octant
				}
				float etl = abs(b2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y + 1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y + 1), ej), 2) - b2 * a2);
				float el = abs(b2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y), ej), 2) - b2 * a2);
				if (el < etl) dir = "l";
				else dir = "tl";
				//now repeat this pattern 6 times. Joy.
			}
			else if (m >= 0 && m < 1 && ij.y >= 0) { //octant III
				if (startingOct == 0) { startingOct = 3; }
				else if (startingOct == 3) {
					if (leftStartingOct) lastLap = true;
				}
				else {
					leftStartingOct = true;
					if (lastLap) loop = false; //ends loop if its passed all the way past the starting octant
				}
				float el = abs(b2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y), ej), 2) - b2 * a2);
				float ebl = abs(b2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y-1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y-1), ej), 2) - b2 * a2);

				if (ebl < el) dir = "bl"; // I should really replace these with enums. But whatevs
				else dir = "l";
				//now repeat this pattern 5 times. Joy.
			}
			else if (m >= 1 && ij.y > 0) { //octant IV
				if (startingOct == 0) { startingOct = 4; }
				else if (startingOct == 4) {
					if (leftStartingOct) lastLap = true;
				}
				else {
					leftStartingOct = true;
					if (lastLap) loop = false; //ends loop if its passed all the way past the starting octant
				}
				float ebl = abs(b2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y - 1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x - 1, ellipse[index].y - 1), ej), 2) - b2 * a2);
				float eb = abs(b2 * pow(dot(vec2(ellipse[index].x, ellipse[index].y - 1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x, ellipse[index].y - 1), ej), 2) - b2 * a2);

				if (eb < ebl) dir = "b"; // I should really replace these with enums. But whatevs
				else dir = "bl";
				//now repeat this pattern 4 times. Joy.
			}
			else if (m <= -1 && ij.y < 0) { //octant V
				if (startingOct == 0) { startingOct = 5; }
				else if (startingOct == 5) {
					if (leftStartingOct) lastLap = true;
				}
				else {
					leftStartingOct = true;
					if (lastLap) loop = false; //ends loop if its passed all the way past the starting octant
				}
				float eb = abs(b2 * pow(dot(vec2(ellipse[index].x, ellipse[index].y - 1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x, ellipse[index].y - 1), ej), 2) - b2 * a2);
				float ebr = abs(b2 * pow(dot(vec2(ellipse[index].x+1, ellipse[index].y - 1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x+1, ellipse[index].y - 1), ej), 2) - b2 * a2);

				if (ebr < eb) dir = "br"; // I should really replace these with enums. But whatevs
				else dir = "b";
				//now repeat this pattern 3 times. I probably could have used a complex function. At this point, I don't care
			}
			else if (m < 0 && m >= -1 && ij.y < 0) { //octant VI
				if (startingOct == 0) { startingOct = 6; }
				else if (startingOct == 6) {
					if (leftStartingOct) lastLap = true;
				}
				else {
					leftStartingOct = true;
					if (lastLap) loop = false; //ends loop if its passed all the way past the starting octant
				}
				float ebr = abs(b2 * pow(dot(vec2(ellipse[index].x + 1, ellipse[index].y - 1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x + 1, ellipse[index].y - 1), ej), 2) - b2 * a2);
				float er = abs(b2 * pow(dot(vec2(ellipse[index].x + 1, ellipse[index].y), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x + 1, ellipse[index].y), ej), 2) - b2 * a2);
				if (er < ebr) dir = "r"; // I should really replace these with enums. But whatevs
				else dir = "br";
				//now repeat this pattern twice
			}
			else if (m < 1 && m >= 0 &&ij.y < 0) { //octant VII
				if (startingOct == 0) { startingOct = 7; }
				else if (startingOct == 7) {
					if (leftStartingOct) lastLap = true;
				}
				else {
					leftStartingOct = true;
					if (lastLap) loop = false; //ends loop if its passed all the way past the starting octant
				}
				float er = abs(b2 * pow(dot(vec2(ellipse[index].x + 1, ellipse[index].y), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x + 1, ellipse[index].y), ej), 2) - b2 * a2);
				float etr = abs(b2 * pow(dot(vec2(ellipse[index].x + 1, ellipse[index].y+1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x + 1, ellipse[index].y+1), ej), 2) - b2 * a2);
				if (etr < er) dir = "tr"; // I should really replace these with enums. But whatevs
				else dir = "r";
			}//now repeat this pattern once more

			else if (m > 1 && ij.y < 0) { //octant VIII
				if (startingOct == 0) { startingOct = 8; }
				else if (startingOct == 8) {
					if (leftStartingOct) lastLap = true;
				}
				else {
					leftStartingOct = true;
					if (lastLap) loop = false; //ends loop if its passed all the way past the starting octant
				}
				float etr = abs(b2 * pow(dot(vec2(ellipse[index].x + 1, ellipse[index].y + 1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x + 1, ellipse[index].y + 1), ej), 2) - b2 * a2);
				float et = abs(b2 * pow(dot(vec2(ellipse[index].x, ellipse[index].y + 1), ei), 2) + a2 * pow(dot(vec2(ellipse[index].x, ellipse[index].y + 1), ej), 2) - b2 * a2); //Finally didn't have to replace this one
				if (et < etr) dir = "t"; // I should really replace these with enums. But whatevs
				else dir = "tr";
			} //Finally done
			cout << "Dir:" << dir << endl;

			if (dir == "tr")ellipse[index+1] = vec2(ellipse[index].x + 1, ellipse[index].y + 1); //should refactor
			if (dir == "t")ellipse[index+1] = vec2(ellipse[index].x, ellipse[index].y + 1);
			if (dir == "tl")ellipse[index+1] = vec2(ellipse[index].x - 1, ellipse[index].y + 1);
			if (dir == "l")ellipse[index+1] = vec2(ellipse[index].x - 1, ellipse[index].y);
			if (dir == "bl")ellipse[index+1] = vec2(ellipse[index].x - 1, ellipse[index].y - 1);
			if (dir == "b")ellipse[index+1] = vec2(ellipse[index].x, ellipse[index].y - 1);
			if (dir == "br")ellipse[index+1] = vec2(ellipse[index].x + 1, ellipse[index].y - 1);
			if (dir == "r")ellipse[index+1] = vec2(ellipse[index].x + 1, ellipse[index].y);
			cout << endl;
			index++;
		}ellipse[500].x = 2;

		ellipse.shrink_to_fit();
		for (short i = 0; i < ellipse.size(); i++) { // go ahead and place it in normalized coordinates

			ellipse[i].x += vertices[0].x;
			ellipse[i].y += vertices[0].y;
		}

		for (vec2 v : ellipse) { // draw to framebuffer
			fb[v.x][v.y] = fillColor;

		}

	}

	void drawOutlineKennedy(Vertex vertices[3]) {

		short a = abs(vertices[0].x - vertices[1].x) + abs(vertices[0].y - vertices[1].y); //semi major
		short b = abs(vertices[0].x - vertices[2].x) + abs(vertices[0].y - vertices[2].y); //semi minor
		quadLen = std::ceil(a+b); // gets number of pixles in quartant
		quadrant.resize(quadLen + 1);
		//using John Kennedy's algorithm
		//NOTE: fails in narrow edge cases such as length of 62 and height of 2

		int twoA2 = 2 * a * a;
		int twoB2 = 2 * b * b;
		short x = a;
		short y = 0;
		int dx = b * b*(1 - 2 * a); //delta x
		int dy = a * a;
		int e = 0;
		int sx = twoB2 * a; //stopping x
		int sy = 0;
		short i = 0; //index

		while (sx >= sy) {//octant from 0 -> pi/4
			quadrant[i] = Vertex{ x, y };

			y++;
			sy += twoA2;			
			e += dy;
			dy += twoA2;
			if ((2 * e + dx) > 0) {
				x--;
				sx -= twoB2;
				e += dx;
				dx += twoB2;
			}
			i++;
		}

		x = 0;
		y = b;
		dx = b * b;
		dy = a * a*(1 - 2*b);
		e = 0;
		sx = 0;
		sy = twoA2 * b;
		while (sx <= sy) { //octant from pi/2 -> pi/4
			quadrant[i] = Vertex{ x, y };

			x++;
			sx += twoB2;
			e += dx;
			dx += twoB2;
			if ((2 * e +dy) > 0) {
				y--;
				sy -= twoA2;
				e += dy;
				dy += twoA2;
			}

			i++;
		}
		quadLen = i;

		quadrant.resize(i);
		vector<Vertex> ellipse(quadLen * 4); //Now lets copy the quadrant four times
		for (short i = 0; i < quadLen; i++) {
			short ci = i * 4;//index for the elipse, output of this loop
			ellipse[ci].x = quadrant[i].x; //QI
			ellipse[ci].y = quadrant[i].y;

			ellipse[ci+1].x = quadrant[i].x*-1; //QII
			ellipse[ci+1].y = quadrant[i].y;

			ellipse[ci+2].x = quadrant[i].x*-1; //QIII
			ellipse[ci+2].y = quadrant[i].y*-1;

			ellipse[ci+3].x = quadrant[i].x; //QIV
			ellipse[ci+3].y = quadrant[i].y*-1;


		}
		for (short i = 0; i < ellipse.size(); i++) { // go ahead and place it in normalized coordinates

			ellipse[i].x += vertices[0].x;
			ellipse[i].y += vertices[0].y;
		}

		for (Vertex v : ellipse) { // draw to framebuffer
			fb[v.x][v.y] = fillColor;

		}
	}

};

int main(){

	EllipseRasta test = EllipseRasta(512, 512);
	test.drawOutlineZingl(vertices);
	//test.drawOutlineKennedy(vertices);
	//test.fill(vertices[0].x, vertices[0].y);
	test.write("results.bmp");

	

}
