#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <string>
#include <BMP.h>


#define WIDTH 100 //framebuffer size
#define HEIGHT 100
#define PI 3.14159265

using std::ios;
using std::vector;
using std::cout;
using std::endl;
using std::string;
using std::to_string;

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
	{128, 128}, // midpoint
	{190, 128}, // right side
	{128, 134}, // top

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
		//using midpoint algorithm
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

	EllipseRasta test = EllipseRasta(256, 256);
	test.drawOutlineKennedy(vertices);
	test.fill(vertices[0].x, vertices[0].y);
	test.write("results.bmp");

	

}
