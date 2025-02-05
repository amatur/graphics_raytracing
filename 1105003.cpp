#define _SCL_SECURE_NO_WARNINGS
#include <windows.h> //needed for codeblocks
#include <glut.h> //needed for codeblocks
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<iomanip>
#include<fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <assert.h>
//#include <GL\glut.h> //for visual studio 2013
#include "bitmap_image.hpp"
using namespace std;

#define pi (2*acos(0.0))
bool debug = false;
double EPSILON = 0.000001;
double MOVEAHEAD = 0.3;
bool TEXTURE_ENABLED = true;

/** -------- Epsilon check ------------ **/
double removeNegZero(double theDouble, int precision){
	if ((theDouble < 0.0f) &&
		(-log10(abs(theDouble)) > precision))
	{
		return -theDouble;
	}
	else
	{
		return theDouble;
	}
}
int compare(double v1, double v2, double e = 0.1){
	v1 = removeNegZero(v1, 7);
	v2 = removeNegZero(v2, 7);

	if (abs(v1 - v2) < e){
		// v1 == v2
		return 0;
	}
	else if (v1 > v2){
		return 1;
	}
	else if (v1 < v2){
		return -1;
	}
	return -999;
}

class Vector {
public:
	double x, y, z;
	Vector(double x = 0.0, double y = 0.0, double z = 0.0) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	//pass in a vector, pass in a scalar, return the product
	friend Vector operator*(float num, Vector const &vec)
	{
		return Vector(vec.x * num, vec.y * num, vec.z * num);
	}

	Vector operator*(const double& scalar) const
	{
		return Vector(x*scalar, y*scalar, z*scalar);
	}
	Vector operator+(const Vector &v) const
	{
		return Vector(x + v.x, y + v.y, z + v.z);
	}
	Vector operator-(const Vector &v) const
	{
		return Vector(x - v.x, y - v.y, z - v.z);
	}
	double magnitude()
	{
		return sqrt((x * x) + (y * y) + (z * z));
	}
	Vector normalize()
	{
		double mag = magnitude();
		if (mag != 0){
			;
		}
		if(mag == 0) return mag;
		return Vector(x / mag, y / mag, z / mag);
	}

	/* a is the angle in degree, r is the axis to rotate against */
	Vector rotate(double a, Vector r) //
	{
		Vector i(*this);
		Vector u = r.cross(i);
		i = i * cosf(a*pi / 180);
		u = u * sinf(a*pi / 180);
		return i + u;
	}
	double dot(const Vector &vec) const
	{
		return x * vec.x + y * vec.y + z * vec.z;
	}
	Vector cross(const Vector &vec) const
	{
		return Vector(y * vec.z - z * vec.y,
			z * vec.x - x * vec.z,
			x * vec.y - y * vec.x);
	}
	friend ostream &operator<<(ostream &output, const Vector &vec)
	{
		output << vec.x << " " << vec.y << " " << vec.z;
		return output;
	}
};
class Point {
public:
	double x, y, z;
	Point(double x = 0.0, double y = 0.0, double z = 0.0) {
		this->x = x;
		this->y = y;
		this->z = z;
	}
	Point operator+(const Vector &v) const
	{
		return Point(x + v.x, y + v.y, z + v.z);
	}
	Point operator+(const double &d) const
	{
		return Point(x + d, y + d, z + d);
	}
	Point operator-(const Vector &v) const
	{
		return Point(x - v.x, y - v.y, z - v.z);
	}

	Vector operator-(const Point &b) const
	{
		return Vector(x - b.x, y - b.y, z - b.z);
	}
	friend ostream &operator<<(ostream &output, const Point &p)
	{
		double prec = output.precision();
		//output << removeNegZero(p.x, prec) << " " << removeNegZero(p.y, prec) << " " << removeNegZero(p.z, prec);
		output << (p.x) << " " << (p.y) << " " << (p.z);
		return output;
	}
};

/** -------- CAMERA CONTROL related ------------ **/
int drawgrid;
int drawaxes;
/* speed */
double ROTATE_SPEED = 3; // angle in degree
double RIGHT_SPEED = 2;
double UP_SPEED = 2;

/* position and up, right, look direction */
//l, pos, u, r
Point pos(0, -140, 10);
Vector u(0, 0, 1), l(0, 1, 0), r(1, 0, 0);
//Point pos(100, 100, 2);
//Vector u(0, 0, 1), l(-1 / sqrt(2.0), -1 / sqrt(2.0), 0), r(-1 / sqrt(2.0), 1 / sqrt(2.0), 0);
// -------------------------------------------- //


/** ---------- RAY TRACE related -------------- **/
bitmap_image image;
bitmap_image texture;
void raytraceMain();
double MAXT = 99999;
// -------------------------------------------- //

class Color{
public:
	double r, g, b;
	Color(double r = 0.0, double g = 0.0, double b = 0.0) {
		this->r = r;
		this->g = g;
		this->b = b;
	}
	Color normalize(){
		Color c(*this);
		if (r > 1 || g > 1 || b > 1) {
			double sum = r + g + b;
			c.r = r / sum;
			c.g = g / sum;
			c.b = b / sum;
		}
		return c;
	}
	Color clamp(){
		Color c(*this);
		//return c;
		if (r > 1) c.r = 1;
		if (g > 1) c.g = 1;
		if (b > 1) c.b = 1;
		return c;
	}
	friend Color operator*(Color const &c, double factor)
	{
		return Color(c.r * factor, c.g * factor, c.b * factor);
	}
	friend Color operator+(Color const &c1, Color const &c2)
	{
		return Color(c1.r + c2.r, c1.g + c2.g, c1.b + c2.b);
	}
	friend ostream &operator<<(ostream &output, const Color &col)
	{
		output << col.r << " " << col.g << " " << col.b;
		return output;
	}
};
class Intersection{
public:
	double t;
	Vector normal;
	Color col;
	Color surfaceCol;
	double ambient, diffuse, specular, reflection, shininess;
	Intersection(){};
	void set(double t, Vector n, Color c, double ambient, double diffuse, double specular, double reflection, double shininess){
		this->t = t;
		this->normal = n;
		this->col = c;
		this->ambient = ambient;
		this->diffuse = diffuse;
		this->specular = specular;
		this->reflection = reflection;
		this->shininess = shininess;
	}
};

class Ray {
public:
	Point origin;
	Vector direction;
	Intersection intersection;

	Ray(Point eye, Vector dir, Color backColor) {
		origin = eye;
		direction = dir.normalize();
		intersection.col = backColor;
		intersection.t = MAXT;
	}
};

class Obj{
public:
	string name;	//for debug
	Color color;
	double ambient, diffuse, specular, reflection, shininess;
};

class Board : public Obj{
public:
	//plane is in point normal form
	Vector n;	//normal (unit)
	Point pO;	//point on plane

	bool intersect(Ray& ray)
	{
		// for ray: p = rO + rD * t
		// for plane: (p - pO).n = 0

		double denom = n.dot(ray.direction);
		// if dot product is 0 then no intersection
		if (abs(denom) > EPSILON) {
			Vector diff = pO - ray.origin;
			double t = diff.dot(n) / denom;
			if (t >= 0 && t <= ray.intersection.t){
				Point iP = ray.origin + (ray.direction * t);
				int x = iP.x;
				int y = iP.y;
				int w = texture.width();
				int h = texture.height();
				int XHS = 10;
				int XFS = XHS * 2;
				int YHS = 10;
				int YFS = YHS * 2;

				//texturing
				if (TEXTURE_ENABLED){
					unsigned char r, g, b;

					double xrem = (iP.x - floor(iP.x / XFS) * XFS) / XFS;
					double yrem = (iP.y - floor(iP.y / YFS) * YFS) / YFS;

                    if (xrem < 0) xrem *= -1;;
                    if (yrem < 0) yrem *= -1;

					int textureX = (xrem)*w;
					int textureY = (yrem)*h;

					//cout << textureX << " " << textureY << endl;

					texture.get_pixel(textureX, textureY, r, g, b);
					ray.intersection.set(t, n, Color(r / 255.0, g / 255.0, b / 255.0), ambient, diffuse, specular, reflection, shininess);
				}
				else{
					//checkerboard
					int xr = abs(x % XFS);
					int yr = abs(y % YFS);
					if ( x < 0 ) xr = XFS - xr;
					if ( y < 0 ) yr = YFS - yr;
					if (xr >= XHS && yr >= YHS || xr < XHS && yr < YHS){
						ray.intersection.set(t, n, Color(0, 0, 0), ambient, diffuse, specular, reflection, shininess);
					}
					else{
						ray.intersection.set(t, n, Color(1.0, 1.0, 1.0), ambient, diffuse, specular, reflection, shininess);
					}

				}
				return true;
			}
		}
		return false;
	}
};

class Sphere : public Obj{
public:
	double radius;
	Point center;
	Vector normal;

	bool intersect(Ray &ray) {
		Vector EC = center - ray.origin;    //E: eye/origin, C: Centre
		double v = ray.direction.dot(EC);
        double t = radius * radius + v * v - EC.x * EC.x - EC.y * EC.y - EC.z * EC.z;
        if (t < 0){
			return false;
		}
        t = v - ((double)sqrt(t));
		if (t > ray.intersection.t || t < 0){
			return false;
		}
		Point Phit = ray.origin + t*ray.direction;
		ray.intersection.set(t, (Phit - center).normalize(), this->color, ambient, diffuse, specular, reflection, shininess);
		return true;
	}
};
class Triangle : public Obj{
public:
	Point p0, p1, p2;
	Vector normal;
	Triangle(Point p0, Point p1, Point p2, Color c, double ambient, double diffuse, double specular, double reflection, double shininess){
		this->p0 = p0;
		this->p1 = p1;
		this->p2 = p2;
		this->color = c;
		normal = (p2 - p1).cross(p2 - p0); // N
		normal = normal.normalize();
		if (Vector(0.0, 0.0, 1.0).dot(normal) < 0){	// cos < 0 means obtuse angle (>90 degree)
			normal = normal * (-1);
		}
		this->ambient = ambient;
		this->diffuse = diffuse;
		this->specular = specular;
		this->reflection = reflection;
		this->shininess = shininess;
	}
	Triangle(){}
	bool intersect(Ray& ray)
	{
		Vector AB = p1 - p0;
		Vector BC = p2 - p1;
		Vector CA = p0 - p2;

		double denom = normal.dot(ray.direction);
		if (abs(denom) <= EPSILON) {    // if dot product is 0 then no intersection
			return false;
		}
		Vector EA = p0 - ray.origin;    //E: eye, A: first vertex p0 of triangle
		double t = EA.dot(normal) / denom;
		if (t < 0 || t > ray.intersection.t){ // t<0 means the triangle is behind eye
			return false;
		}
		Point Phit = ray.origin + t * ray.direction;

		//inside triangle or not
		Vector AH = Phit - p0; //H: hit point, A: p0
		Vector BH = Phit - p1;
		Vector CH = Phit - p2;

		if (normal.dot(AB.cross(AH)) < 0 || normal.dot(BC.cross(BH)) < 0 || normal.dot(CA.cross(CH)) < 0){
		    return false; // Phit is outside triangle
        }
		ray.intersection.set(t, normal, this->color, ambient, diffuse, specular, reflection, shininess);
		return true;
	}
};
class PyramidBase : public Obj
{
public:
	//plane is in point normal form
	Vector n;	//normal (unit)
	Point pO;	//point on plane (lowest point)
	double width;
	PyramidBase(){}
	PyramidBase(Point pO, double width, Color c, double ambient, double diffuse, double specular, double reflection, double shininess){
		this->n = Vector(0.0, 0.0, -1.0);
		this->pO = pO;
		this->color = c;
		this->width = width;
		this->ambient = ambient;
		this->diffuse = diffuse;
		this->specular = specular;
		this->reflection = reflection;
		this->shininess = shininess;
	}
	bool intersect(Ray& ray)
	{
		// assuming vectors are all normalized
		double denom = n.dot(ray.direction);
		// if dot product is 0 then no intersection
		if (abs(denom) > EPSILON) {
			Vector diff = pO - ray.origin;
			double t = diff.dot(n) / denom;
			if (t >= 0 && t <= ray.intersection.t){
				Point iP = ray.origin + (ray.direction * t);
				int x = iP.x;
				int y = iP.y;
				if (x > pO.x  && x < pO.x + width  && y > pO.y  && y < pO.y + width){
					ray.intersection.set(t, this->n, this->color, ambient, diffuse, specular, reflection, shininess);
					return true;
				}
			}
		}
		return false;
	}
};
class Pyramid : public Obj{
public:
	Point lowestPoint;
	double width;
	double height;
	Triangle t[4];
	PyramidBase base;
	Pyramid(Point lp, double w, double h, Color color, double ambient, double diffuse, double specular, double reflection, double shininess){
		lowestPoint = lp;
		width = w;
		height = h;
		Point p0(lp.x + 0, lp.y + 0, lp.z + 0); //front left vertex
		Point p1(lp.x + w, lp.y + 0, lp.z + 0); //front right vertex
		Point p2(lp.x + w, lp.y + w, lp.z + 0); //back right vertex
		Point p3(lp.x + 0, lp.y + w, lp.z + 0);//back left vertex
		Point top(lp.x + w / 2.0, lp.y + w / 2.0, lp.z + h); //top vertex
		t[0] = Triangle(p0, p1, top, color, ambient, diffuse, specular, reflection, shininess);
		t[1] = Triangle(p1, p2, top, color, ambient, diffuse, specular, reflection, shininess);
		t[2] = Triangle(p2, p3, top, color, ambient, diffuse, specular, reflection, shininess);
		t[3] = Triangle(p3, p0, top, color, ambient, diffuse, specular, reflection, shininess);
		base = PyramidBase(lp, w, color, ambient, diffuse, specular, reflection, shininess);
		this->color = color;
		this->ambient = ambient;
		this->diffuse = diffuse;
		this->specular = specular;
		this->reflection = reflection;
		this->shininess = shininess;
	}
};

class Scene{
public:
	int levelRecursion;
	int height;
	int width;
	int numObj;
	int numLightSrc;
	double fov;
	vector<Sphere> spheres;
	vector<Pyramid> pyramids;
	Board checkerboard;
	vector<Point> lightSources;
	Color backColor;
};
Scene s;

bool trace(Ray &r) {
	bool foundTraced = false;

	for (vector<Sphere>::iterator it = s.spheres.begin(); it != s.spheres.end(); ++it){
		Sphere object = *it;
		if (object.intersect(r) == true){
			foundTraced = true;
		}
	}

	for (vector<Pyramid>::iterator it = s.pyramids.begin(); it != s.pyramids.end(); ++it){
		Pyramid p = *it;
		for (int i = 0; i < 4; i++)
		{
			if (p.t[i].intersect(r) == true){
				foundTraced = true;
			}
		}
		if (p.base.intersect(r) == true){
			foundTraced = true;
		}
	}

	Board object = s.checkerboard;
	if (object.intersect(r) == true){
		foundTraced = true;
	}

	return (foundTraced);
}

void drawAxes()
{
	if (drawaxes == 1)
	{
		glBegin(GL_LINES); {
			glColor3f(1.0, 0.0, 0.0);
			glVertex3f(100, 0, 0);
			glVertex3f(-100, 0, 0);

			glColor3f(0.0, 1.0, 0.0);
			glVertex3f(0, -100, 0);
			glVertex3f(0, 100, 0);

			glColor3f(0.0, 0.0, 1.0);
			glVertex3f(0, 0, 100);
			glVertex3f(0, 0, -100);
		}glEnd();
	}
}
void drawGrid()
{
	int i;
	if (drawgrid == 1)
	{
		glColor3f(0.6, 0.6, 0.6);	//grey
		glBegin(GL_LINES); {
			for (i = -8; i <= 8; i++){

				if (i == 0)
					continue;	//SKIP the MAIN axes

				//lines parallel to Y-axis
				glVertex3f(i * 10, -90, 0);
				glVertex3f(i * 10, 90, 0);

				//lines parallel to X-axis
				glVertex3f(-90, i * 10, 0);
				glVertex3f(90, i * 10, 0);
			}
		}glEnd();
	}
}
void drawPyramid(Pyramid& p){
	glBegin(GL_TRIANGLES); {
		for (int i = 0; i < 4; i++)
		{
			Color c = p.color;
			glColor3f(c.r, c.g / 2, c.b / 2); glVertex3f(p.t[i].p0.x, p.t[i].p0.y, p.t[i].p0.z);
			glColor3f(c.r / 2, c.g / 2, c.b); glVertex3f(p.t[i].p1.x, p.t[i].p1.y, p.t[i].p1.z);
			glColor3f(c.r / 2, c.g, c.b / 2); glVertex3f(p.t[i].p2.x, p.t[i].p2.y, p.t[i].p2.z);
		}
	}glEnd();

	Point lp = p.lowestPoint;
	double w = p.width;
	double h = p.height;
	Point p0(lp.x + 0, lp.y + 0, lp.z + 0); //front left vertex
	Point p1(lp.x + w, lp.y + 0, lp.z + 0); //front right vertex
	Point p2(lp.x + w, lp.y + w, lp.z + 0); //back right vertex
	Point p3(lp.x + 0, lp.y + w, lp.z + 0);//back left vertex
	Point top(lp.x + w / 2.0, lp.y + w / 2.0, lp.z + h); //top vertex
	glBegin(GL_QUADS); {
		glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(p0.x, p0.y, p0.z);
		glColor3f(0.0f, 1.0f, 0.0f); glVertex3f(p1.x, p1.y, p1.z);
		glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(p2.x, p2.y, p2.z);
		glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(p3.x, p3.y, p3.z);
	}glEnd();
}
void drawScene(){
	glPushMatrix(); {
		glTranslatef(-150, -150, 0);
		glBegin(GL_QUADS); {
			for (unsigned int x = 0; x < 150; ++x){
				for (unsigned int y = 0; y < 150; ++y)
				{
					if (abs((int)(x + y)) % 2) //modulo 2
						glColor3f(1.0f, 1.0f, 1.0f); //white
					else
						glColor3f(0.0f, 0.0f, 0.0f); //black

					glVertex2f(x * 10, y * 10);
					glVertex2f((x + 1) * 10, y * 10);
					glVertex2f((x + 1) * 10, (y + 1) * 10);
					glVertex2f(x * 10, (y + 1) * 10);

				}
			}
		}glEnd();

	}glPopMatrix();


	for (vector<Point>::iterator it = s.lightSources.begin(); it != s.lightSources.end(); ++it){
		Point lightSrc = *it;
		glPushMatrix(); {
			glColor3f(1, 1, 1);
			glTranslatef(lightSrc.x, lightSrc.y, lightSrc.z); //move to axis center
			glutSolidSphere(10, 100, 5);
		}
		glPopMatrix();
	}
	for (vector<Sphere>::iterator it = s.spheres.begin(); it != s.spheres.end(); ++it){
		Sphere object = *it;
		glPushMatrix(); {
			glColor3f(object.color.r, object.color.g, object.color.b);
			glTranslatef(object.center.x, object.center.y, object.center.z); //move to axis center
			glutSolidSphere(object.radius, 100, 100);
		}
		glPopMatrix();

	}
	for (vector<Pyramid>::iterator it = s.pyramids.begin(); it != s.pyramids.end(); ++it){
		Pyramid p = *it;
		drawPyramid(p);
	}
}

void printCamera(){
	ofstream cameraPosition;
	cameraPosition.open("camera.txt");
	cameraPosition << l << endl;
	cameraPosition << pos << endl;
	cameraPosition << u << endl;
	cameraPosition << r << endl;
	cameraPosition.close();
}

void keyboardListener(unsigned char key, int x, int y){
	switch (key){
		// rotate/ look right
	case '1':
		l = l.rotate(ROTATE_SPEED, u);
		r = r.rotate(ROTATE_SPEED, u);
		break;

		// rotate/look left
	case '2':
		l = l.rotate(-ROTATE_SPEED, u);
		r = r.rotate(-ROTATE_SPEED, u);
		break;

		//look up
	case '3':
		l = l.rotate(ROTATE_SPEED, r);
		u = u.rotate(ROTATE_SPEED, r);
		break;

		//look down
	case '4':
		l = l.rotate(-ROTATE_SPEED, r);
		u = u.rotate(-ROTATE_SPEED, r);
		break;

		// tilt clockwise
	case '5':
		r = r.rotate(-ROTATE_SPEED, l);
		u = u.rotate(-ROTATE_SPEED, l);
		break;

		// tilt counterclockwise
	case '6':
		r = r.rotate(ROTATE_SPEED, l);
		u = u.rotate(ROTATE_SPEED, l);
		break;
	case '0':
	    TEXTURE_ENABLED = false;
	    cout << "Rendering... please wait" << endl;
		//printCamera();
		if (debug) cout << l << endl;
		if (debug) cout << pos << endl;
		if (debug) cout << u << endl;
		raytraceMain();
		cout << "FINISHED!\n" << endl;
		break;
    case '9':
        TEXTURE_ENABLED = true;
		cout << "Rendering... please wait" << endl;
		//printCamera();
		if (debug) cout << l << endl;
		if (debug) cout << pos << endl;
		if (debug) cout << u << endl;
		raytraceMain();
		cout << "FINISHED!\n" << endl;
		break;
	case 'g':
		drawgrid = 1 - drawgrid;
		drawaxes = 1 - drawaxes;
		break;
	default:
		break;
	}
}
void specialKeyListener(int key, int x, int y){
	switch (key){
		//1. Move Forward
	case GLUT_KEY_UP:		// up arrow key
		pos = pos + UP_SPEED * l;
		break;
		//2. Move Backward
	case GLUT_KEY_DOWN:		//down arrow key
		pos = pos - UP_SPEED * l;
		break;
	case GLUT_KEY_RIGHT:
		pos = pos + r * RIGHT_SPEED;
		break;
	case GLUT_KEY_LEFT:
		pos = pos - r * RIGHT_SPEED;
		break;

		//move up
	case GLUT_KEY_PAGE_UP:
		pos = pos + UP_SPEED * u;
		break;
	case GLUT_KEY_PAGE_DOWN:
		pos = pos - UP_SPEED * u;
		break;

	case GLUT_KEY_INSERT:
		break;

	default:
		break;
	}
}
void mouseListener(int button, int state, int x, int y){	//x, y is the x-y of the screen (2D)
	switch (button){
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN){		// 2 times?? in ONE click? -- solution is checking DOWN or UP
			//drawaxes = 1 - drawaxes;
		}
		break;

	case GLUT_RIGHT_BUTTON:
		//........
		break;

	case GLUT_MIDDLE_BUTTON:
		//........
		break;

	default:
		break;
	}
}

void display(){
	//clear the display
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0, 0, 0, 0);	//color black
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glShadeModel(GL_SMOOTH); // Enable smooth shading
	/********************
	/ set-up camera here
	********************/
	//load the correct matrix -- MODEL-VIEW matrix
	glMatrixMode(GL_MODELVIEW);

	//initialize the matrix
	glLoadIdentity();

	//now give three info
	//1. where is the camera (viewer)?
	//2. where is the camera looking?
	//3. Which direction is the camera's UP direction?
	gluLookAt(pos.x, pos.y, pos.z, pos.x + l.x, pos.y + l.y, pos.z + l.z, u.x, u.y, u.z);

	//again select MODEL-VIEW
	glMatrixMode(GL_MODELVIEW);


	/****************************
	/ Add your objects from here
	****************************/
	//add objects
	drawAxes();
	drawGrid();
	drawScene();
	//ADD this line in the end --- if you use double buffer (i.e. GL_DOUBLE)
	glutSwapBuffers();
}
void animate(){
	//codes for any changes in Models, Camera
	glutPostRedisplay();
}
void init(){
	//codes for initialization
	drawgrid = 1;
	drawaxes = 0;

	//clear the screen
	glClearColor(0, 0, 0, 0);

	/************************
	/ set-up projection here
	************************/
	//load the PROJECTION matrix
	glMatrixMode(GL_PROJECTION);

	//initialize the matrix
	glLoadIdentity();

	//give PERSPECTIVE parameters
	gluPerspective(s.fov, 1, 1, 1000);
	//field of view in the Y (vertically)
	//aspect ratio that determines the field of view in the X direction (horizontally)
	//near distance
	//far distance
}

void inputSceneParameters(){
	texture = bitmap_image("texture.bmp");
	ifstream sceneFileIn("description.txt");
	string line;

	if (sceneFileIn.is_open())
	{
		getline(sceneFileIn, line);		//Line 1: level of recursion
		stringstream(line) >> s.levelRecursion;
		getline(sceneFileIn, line);		//Line 2: number of pixels along both axes
		stringstream(line) >> s.height;
		s.width = s.height;
		while (getline(sceneFileIn, line)){ //Line 3 : blank
			if (!line.empty()) break;	//Line 4 : number of objects
		}
		stringstream(line) >> s.numObj;
		getline(sceneFileIn, line);		//Line 5 : blank
		for (int i = 0; i < s.numObj; i++)
		{
			while (getline(sceneFileIn, line)){
				if (!line.empty()) break;	//0: object name
			}
			//getline(sceneFileIn, line);	//0: object name
			if (line == "sphere"){
				Sphere o;
				o.name = "sphere";
				getline(sceneFileIn, line);	//1. center
				stringstream(line) >> o.center.x >> o.center.y >> o.center.z;
				getline(sceneFileIn, line);	//2. radius
				stringstream(line) >> o.radius;
				getline(sceneFileIn, line);	//-3: color
				stringstream(line) >> o.color.r >> o.color.g >> o.color.b;
				getline(sceneFileIn, line);	//-2: ambient diffuse specular reflection coefficient
				stringstream(line) >> o.ambient >> o.diffuse >> o.specular >> o.reflection;
				getline(sceneFileIn, line);	//-1: shininess
				stringstream(line) >> o.shininess;
				s.spheres.push_back(o);
			}
			else if (line == "pyramid"){
				Point lowestPoint;
				Color color;
				double width, height, ambient, diffuse, specular, reflection, shininess;
				getline(sceneFileIn, line);	//1. lowest point co-ordinate
				stringstream(line) >> lowestPoint.x >> lowestPoint.y >> lowestPoint.z;
				getline(sceneFileIn, line);	//2. width, height
				stringstream(line) >> width >> height;
				getline(sceneFileIn, line);	//-3: color
				stringstream(line) >> color.r >> color.g >> color.b;
				getline(sceneFileIn, line);	//-2: ambient diffuse specular reflection coefficient
				stringstream(line) >> ambient >> diffuse >> specular >> reflection;
				getline(sceneFileIn, line);	//-1: shininess
				stringstream(line) >> shininess;
				Pyramid o(lowestPoint, width, height, color, ambient, diffuse, specular, reflection, shininess);
				o.name = "pyramid";
				s.pyramids.push_back(o);
			}
			//getline(sceneFileIn, line);	//blank
		}
		while (getline(sceneFileIn, line)){
			if (!line.empty()) break; //number of light sources
		}
		stringstream(line) >> s.numLightSrc;
		if (debug) cout << "Num ls: " << s.numLightSrc << endl;
		for (int i = 0; i < s.numLightSrc; i++)
		{
			Point p;
			getline(sceneFileIn, line); //pos
			stringstream(line) >> p.x >> p.y >> p.z;
			if (debug) cout << p << endl;
			s.lightSources.push_back(p);
		}
		sceneFileIn.close();
	}
	s.backColor.r = 0;
	s.backColor.g = 0;
	s.backColor.b = 0;

	s.checkerboard.n = Vector(0.0, 0.0, 1.0);
	s.checkerboard.pO = Point(1.0, 1.0, 0.0);
	s.checkerboard.ambient = 0.3;
	s.checkerboard.diffuse = 0.3;
	s.checkerboard.specular = 0.0;
	s.checkerboard.reflection = 0.4;
	s.fov = 45;
}

void rayAddDiffuseSpecular(Ray &ray, Vector &R){

	for (vector<Point>::iterator it = s.lightSources.begin(); it != s.lightSources.end(); ++it){
		Point lightSrc = *it;
		Point hitPoint = ray.origin + ray.intersection.t * ray.direction;
		Ray shadowRay = Ray(lightSrc, hitPoint - lightSrc, s.backColor); //we want LI ray
		if (trace(shadowRay)){
			Point shadowHitPoint = shadowRay.origin + shadowRay.intersection.t * shadowRay.direction;
			double dij = abs((shadowHitPoint - lightSrc).magnitude());
			double D = abs((hitPoint - lightSrc).magnitude());
			if (compare(dij, D) == -1){
				//object hitpoint is in shadow
				//do not add diffuse specular
				continue;
			}
		};

		//L = point of lightsrc, H = hitpoint
		Vector lll = lightSrc - hitPoint; //HL
		lll = lll.normalize();

		//if it's not shadowed, add diffuse and specular
		//add diffuse

		double lambert = lll.dot(ray.intersection.normal);	//cos theta
		if (lambert > 0) {
			if (ray.intersection.diffuse > 0) {
				double diffuse = ray.intersection.diffuse * lambert;
				ray.intersection.col.r = ray.intersection.col.r + ray.intersection.surfaceCol.r * diffuse;
				ray.intersection.col.g = ray.intersection.col.g + ray.intersection.surfaceCol.g * diffuse;
				ray.intersection.col.b = ray.intersection.col.b + ray.intersection.surfaceCol.b * diffuse;
				ray.intersection.col = ray.intersection.col.clamp();
			}

		}

		if (ray.intersection.specular > 0) {
			Vector vvv = (lightSrc - hitPoint).normalize();	//from eye point E to hitpoint H, EH
			double spec = vvv.dot(R.normalize());
			if (spec > 0) {
				spec = ray.intersection.specular *((double)pow((double)spec, ray.intersection.shininess));

				//ray.intersection.col.r = ray.intersection.col.r + spec * ray.intersection.surfaceCol.r;
				//ray.intersection.col.g = ray.intersection.col.g + spec * ray.intersection.surfaceCol.g;
				//ray.intersection.col.b = ray.intersection.col.b + spec * ray.intersection.surfaceCol.b;

				Color color(spec, spec, spec);

				ray.intersection.col = ray.intersection.col + color;
				ray.intersection.col = ray.intersection.col.clamp();
			}
		}
	}
}

void rayTrace(Ray &ray, int depth){
	if (depth <= 0) return;

	bool traced = trace(ray);
	if (traced){
		Vector ON = ray.intersection.normal; //O incident point
		Vector LO = ray.direction;  //incident ray I, from light to incident point O
		Vector R = LO - 2 * LO.dot(ON) * ON;

		ray.intersection.surfaceCol = ray.intersection.col; //save the actual color before adding ambient
		ray.intersection.col = ray.intersection.col * ray.intersection.ambient;	//add ambient

		Point hitPoint = ray.origin + ray.intersection.t * ray.direction;
		rayAddDiffuseSpecular(ray, R);	//add diffuse and specular

		//now add reflection
		Ray reflectedRay = Ray(hitPoint + 0.03 * R.normalize(), R, s.backColor);
		rayTrace(reflectedRay, depth - 1);
		Color colorBroughtByReflectedRay = reflectedRay.intersection.col;
		ray.intersection.col = ray.intersection.col + colorBroughtByReflectedRay * ray.intersection.reflection;
		ray.intersection.col = ray.intersection.col.clamp();
	}
}

void raytraceMain(){
	int rows = s.height;
	int columns = s.width;
	Color** pixelBuffer;
	pixelBuffer = new Color*[rows];
	for (int j = 0; j < rows; j++)
	{
		pixelBuffer[j] = new Color[columns];
	}

	Vector ll = l;
	Vector uu = u;
	Vector rr = r;
	Point eye = pos;

	/*
	ifstream sceneFileIn("camera.txt");
	if (sceneFileIn.is_open())
	{
	string line;
	getline(sceneFileIn, line);	//look direction
	stringstream(line) >> l.x >> l.y >> l.z;
	getline(sceneFileIn, line); //eye position
	stringstream(line) >> eye.x >> eye.y >> eye.z;
	getline(sceneFileIn, line); //up direction
	stringstream(line) >> u.x >> u.y >> u.z;
	sceneFileIn.close();
	}*/

	// Compute viewing matrix that maps a
	// screen coordinate to a ray direction
	///*
	double fov = s.fov;
	double unitPixelLength = (double)((2 * ll.magnitude() * tan((0.5*fov)*pi / 180.0)) / s.width);
	Point look = eye + ll;
	uu = uu.normalize();
	rr = rr.normalize();
	Vector moveUp = uu * unitPixelLength;
	Vector moveRight = rr * unitPixelLength;
	Point topmid = look + moveUp * (s.width / 2);
	Point topleft = topmid - moveRight * (s.width / 2);

	Vector dir = topleft - eye;
	for (int j = 0; j < s.height; j++) {
		for (int i = 0; i < s.width; i++) {
			Ray ray(eye, dir, s.backColor);
			rayTrace(ray, s.levelRecursion);
			pixelBuffer[j][i] = ray.intersection.col;
			dir = dir + moveRight;
		}
		dir = dir - moveRight * (s.width);
		dir = dir - moveUp;
	}
	//*/

	//draw output to bmp file
	image = bitmap_image(s.width, s.height);
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			unsigned char r, g, b;
			r = (unsigned char)(pixelBuffer[i][j].r * 255);
			g = (unsigned char)(pixelBuffer[i][j].g * 255);
			b = (unsigned char)(pixelBuffer[i][j].b * 255);
			if (r > 255) r = 255;
			if (g > 255) g = 255;
			if (b > 255) b = 255;

			image.set_pixel(j, i, r, g, b);
		}
	}
	image.save_image("out.bmp");
}

int main(int argc, char **argv){
	inputSceneParameters();

	glutInit(&argc, argv);
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(0, 0);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);	//Depth, Double buffer, RGB color

	glutCreateWindow("Raytracing 1105003");

	init();

	glEnable(GL_DEPTH_TEST);	//enable Depth Testing

	glutDisplayFunc(display);	//display callback function
	glutIdleFunc(animate);		//what you want to do in the idle time (when no drawing is occuring)

	glutKeyboardFunc(keyboardListener);
	glutSpecialFunc(specialKeyListener);
	glutMouseFunc(mouseListener);

	glutMainLoop();		//The main loop of OpenGL

	return 0;
}
