/*
* Nurul Al Hadi
* Eka Chandra Septiana
*
*
*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "imageloader.h"
#include "vec3f.h"
#endif

/*
komen coba commit
*/

static GLfloat spin, spin2 = 0.0;
float angle = 0;
using namespace std;

float lastx, lasty;
GLint stencilBits;
static int viewx = -200;
static int viewy = 100;
static int viewz = 200;

float rot = 0;

GLuint texture[2]; //array untuk texture

//GLint slices = 16;
//GLint stacks = 16;

struct Images {
unsigned long sizeX;
unsigned long sizeY;
char *data;
};
typedef struct Images Images;
//class untuk terain 2D
class Terrain {
private:
int w; //Width
int l; //Length
float** hs; //Heights
Vec3f** normals;
bool computedNormals; //Whether normals is up-to-date
public:
Terrain(int w2, int l2) {
w = w2;
l = l2;

hs = new float*[l];
for (int i = 0; i < l; i++) {
hs[i] = new float[w];
}

normals = new Vec3f*[l];
for (int i = 0; i < l; i++) {
normals[i] = new Vec3f[w];
}

computedNormals = false;
}

~Terrain() {
for (int i = 0; i < l; i++) {
delete[] hs[i];
}
delete[] hs;

for (int i = 0; i < l; i++) {
delete[] normals[i];
}
delete[] normals;
}

int width() {
return w;
}

int length() {
return l;
}

//Sets the height at (x, z) to y
void setHeight(int x, int z, float y) {
hs[z][x] = y;
computedNormals = false;
}

//Returns the height at (x, z)
float getHeight(int x, int z) {
return hs[z][x];
}

//Computes the normals, if they haven't been computed yet
void computeNormals() {
if (computedNormals) {
return;
}

//Compute the rough version of the normals
Vec3f** normals2 = new Vec3f*[l];
for (int i = 0; i < l; i++) {
normals2[i] = new Vec3f[w];
}

for (int z = 0; z < l; z++) {
for (int x = 0; x < w; x++) {
Vec3f sum(0.0f, 0.0f, 0.0f);

Vec3f out;
if (z > 0) {
out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
}
Vec3f in;
if (z < l - 1) {
in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
}
Vec3f left;
if (x > 0) {
left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
}
Vec3f right;
if (x < w - 1) {
right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
}

if (x > 0 && z > 0) {
sum += out.cross(left).normalize();
}
if (x > 0 && z < l - 1) {
sum += left.cross(in).normalize();
}
if (x < w - 1 && z < l - 1) {
sum += in.cross(right).normalize();
}
if (x < w - 1 && z > 0) {
sum += right.cross(out).normalize();
}

normals2[z][x] = sum;
}
}

//Smooth out the normals
const float FALLOUT_RATIO = 0.5f;
for (int z = 0; z < l; z++) {
for (int x = 0; x < w; x++) {
Vec3f sum = normals2[z][x];

if (x > 0) {
sum += normals2[z][x - 1] * FALLOUT_RATIO;
}
if (x < w - 1) {
sum += normals2[z][x + 1] * FALLOUT_RATIO;
}
if (z > 0) {
sum += normals2[z - 1][x] * FALLOUT_RATIO;
}
if (z < l - 1) {
sum += normals2[z + 1][x] * FALLOUT_RATIO;
}

if (sum.magnitude() == 0) {
sum = Vec3f(0.0f, 1.0f, 0.0f);
}
normals[z][x] = sum;
}
}

for (int i = 0; i < l; i++) {
delete[] normals2[i];
}
delete[] normals2;

computedNormals = true;
}

//Returns the normal at (x, z)
Vec3f getNormal(int x, int z) {
if (!computedNormals) {
computeNormals();
}
return normals[z][x];
}
};
//end class

//Loads a terrain from a heightmap. The heights of the terrain range from
//-height / 2 to height / 2.
Terrain* loadTerrain(const char* filename, float height) {
Image* image = loadBMP(filename);
Terrain* t = new Terrain(image->width, image->height);
for (int y = 0; y < image->height; y++) {
for (int x = 0; x < image->width; x++) {
unsigned char color = (unsigned char) image->pixels[3 * (y
* image->width + x)];
float h = height * ((color / 255.0f) - 0.5f);
t->setHeight(x, y, h);
}
}

delete image;
t->computeNormals();
return t;
}

float _angle = 45.0f;

Terrain* _terrainBukit;
Terrain* _terrainAir;


void cleanup() {
delete _terrainBukit;
delete _terrainAir;
}


void initRendering() {
glEnable(GL_DEPTH_TEST);
glEnable(GL_COLOR_MATERIAL);
glEnable(GL_LIGHTING);
glEnable(GL_LIGHT0);
glEnable(GL_NORMALIZE);
glShadeModel(GL_SMOOTH);
}







void drawScene() {
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
float scale = 500.0f / max(_terrainBukit->width() - 1, _terrainBukit->length() - 1);
glScalef(scale, scale, scale);
glTranslatef(-(float) (_terrainBukit->width() - 1) / 2, 0.0f,
-(float) (_terrainBukit->length() - 1) / 2);

glColor3f(0.3f, 0.9f, 0.0f);
for (int z = 0; z < _terrainBukit->length() - 1; z++) {
//Makes OpenGL draw a triangle at every three consecutive vertices
glBegin(GL_TRIANGLE_STRIP);
for (int x = 0; x < _terrainBukit->width(); x++) {
Vec3f normal = _terrainBukit->getNormal(x, z);
glNormal3f(normal[0], normal[1], normal[2]);
glVertex3f(x, _terrainBukit->getHeight(x, z), z);
normal = _terrainBukit->getNormal(x, z + 1);
glNormal3f(normal[0], normal[1], normal[2]);
glVertex3f(x, _terrainBukit->getHeight(x, z + 1), z + 1);
}
glEnd();
}

}


void gambarTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {

float scale = 400.0f / max(terrain->width() - 1, terrain->length() - 1);
glScalef(scale, scale, scale);
glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
-(float) (terrain->length() - 1) / 2);

glColor3f(r, g, b);
for (int z = 0; z < terrain->length() - 1; z++) {
//Makes OpenGL draw a triangle at every three consecutive vertices
glBegin(GL_TRIANGLE_STRIP);
for (int x = 0; x < terrain->width(); x++) {
Vec3f normal = terrain->getNormal(x, z);
glNormal3f(normal[0], normal[1], normal[2]);
glVertex3f(x, terrain->getHeight(x, z), z);
normal = terrain->getNormal(x, z + 1);
glNormal3f(normal[0], normal[1], normal[2]);
glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
}
glEnd();
}

}

void gambarAir(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {

float scale = 350.0f / max(terrain->width() - 1, terrain->length() - 1);
glScalef(scale, scale, scale);
glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
-(float) (terrain->length() - 1) / 2);

glColor3f(r, g, b);
for (int z = 0; z < terrain->length() - 1; z++) {
//Makes OpenGL draw a triangle at every three consecutive vertices
glBegin(GL_TRIANGLE_STRIP);
for (int x = 0; x < terrain->width(); x++) {
Vec3f normal = terrain->getNormal(x, z);
glNormal3f(normal[0], normal[1], normal[2]);
glVertex3f(x, terrain->getHeight(x, z), z);
normal = terrain->getNormal(x, z + 1);
glNormal3f(normal[0], normal[1], normal[2]);
glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
}
glEnd();
}

}






void update(int value) {

glutPostRedisplay();
glutTimerFunc(25, update, 0);
}


void freetexture(GLuint texture) {
glDeleteTextures(2, &texture);
}





























const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };



void pohon(void){
//batang
GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);

glPushMatrix();
glColor3ub(104,70,14);
glRotatef(270,1,0,0);
gluCylinder(pObj, 4, 0.7, 30, 25, 25);
glPopMatrix();
}

//ranting
void ranting(void){
GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);
glPushMatrix();
glColor3ub(104,70,14);
glTranslatef(0,27,0);
glRotatef(330,1,0,0);
gluCylinder(pObj, 0.6, 0.1, 15, 25, 25);
glPopMatrix();

//daun
glPushMatrix();
glColor3ub(18,118,13);
glScaled(5, 5, 5);
glTranslatef(0,7,3);
glutSolidDodecahedron();
glPopMatrix();
}

//bunga
void bungamerah()
{
     GLUquadricObj *pObj;
    pObj =gluNewQuadric();
    gluQuadricNormals(pObj, GLU_SMOOTH);

   glColor3f(1,0,0);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

   glPushMatrix();
   glTranslatef(-4, 0, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

   glColor3f(1,0,0);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

   glPushMatrix();
   glTranslatef(-5, 0, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

   glColor3f(1,0,0);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

   glPushMatrix();
   glTranslatef(-4.5, 1, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

   glColor3f(0.3,0.2,0);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

   glPushMatrix();
   glTranslatef(-4, -5, 0);
   glRotatef(270,1,0,0);
   gluCylinder(pObj, 0.35, 0.35, 5, 100, 15); //ptr, rbase, rtop, height, slices, stacks
   glPopMatrix();
     }
void bungaungu()
{
     GLUquadricObj *pObj;
    pObj =gluNewQuadric();
    gluQuadricNormals(pObj, GLU_SMOOTH);

   glColor3f(0.2745,0.0039,0.2745);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-4, 0, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

   glColor3f(0.2745,0.0039,0.2745);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-5, 0, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

    glColor3f(0.2745,0.0039,0.2745);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-4.5, 1, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

   glColor3f(0.3,0.2,0);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-4, -5, 0);
  glRotatef(270,1,0,0);
   gluCylinder(pObj, 0.35, 0.35, 5, 100, 15); //ptr, rbase, rtop, height, slices, stacks
   glPopMatrix();
     }

void bungapink()
{
     GLUquadricObj *pObj;
    pObj =gluNewQuadric();
    gluQuadricNormals(pObj, GLU_SMOOTH);

   glColor3f(0.976,0.604,0.961);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-4, 0, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

   glColor3f(0.976,0.604,0.961);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-5, 0, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

    glColor3f(0.976,0.604,0.961);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-4.5, 1, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

   glColor3f(0.3,0.2,0);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-4, -5, 0);
  glRotatef(270,1,0,0);
   gluCylinder(pObj, 0.35, 0.35, 5, 100, 15); //ptr, rbase, rtop, height, slices, stacks
   glPopMatrix();
     }

//bunga kuning
void bungakuning()
{
     GLUquadricObj *pObj;
    pObj =gluNewQuadric();
    gluQuadricNormals(pObj, GLU_SMOOTH);

   glColor3f(0.929,0.988,0.008);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-4, 0, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

   glColor3f(0.929,0.988,0.008);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-5, 0, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

    glColor3f(0.929,0.988,0.008);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-4.5, 1, 0);
   glutSolidSphere(1.5,100,100);
   glPopMatrix();

   glColor3f(0.3,0.2,0);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslatef(-4, -5, 0);
  glRotatef(270,1,0,0);
   gluCylinder(pObj, 0.35, 0.35, 5, 100, 15); //ptr, rbase, rtop, height, slices, stacks
   glPopMatrix();
     }

//tambahan dari ekok
//kincir angin
static int putarx=90;
static int putary=0;
void kincir()
{
     GLUquadricObj *pObj;
    pObj =gluNewQuadric();
    gluQuadricNormals(pObj, GLU_SMOOTH);

    //badan
     glColor3f(0.7f,1.0f,0.7f);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glPushMatrix();
    glTranslatef(-4, -5, 0);
    glRotatef(270,1,0,0);
    gluCylinder(pObj, 12, 10, 30, 100, 15); //ptr, rbase, rtop, height, slices, stacks
    glPopMatrix();
    //atas
    glColor3f(0.3,0.2,0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glPushMatrix();
    glTranslatef(-4, 25, 0);
    glRotatef(270,1,0,0);
    glutSolidCone(12, 10, 25, 100);
    glPopMatrix();


}

void baling()
{
	 glPushMatrix();
	    glColor3f(0.929,0.988,0.008);
	    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	    glTranslatef(-15, 0, 0);
	    glRotatef(90,0,0,1);
	    glRotatef(45,0,1,0);
	    glScalef(7,0.3,0.5);
	    glutSolidCube(5);
	    glPopMatrix();

	    glPushMatrix();
	    glColor3f(1,0,0);
	    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	    glTranslatef(-15, 0, 0);
	    glRotatef(90,0,0,1);
	    glRotatef(45,0,1,0);
	    glScalef(5,1,3);
	    glutSolidCube(2);
	    glPopMatrix();

	    glPushMatrix();
	    glColor3f(1,0,0);
	    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	    glTranslatef(-15, 0, 0);
	    glRotatef(90,0,0,1);
	    glRotatef(135,0,1,0);
	    glScalef(5,1,3);
	    glutSolidCube(2);
	    glPopMatrix();


	    //baling2
	        glPushMatrix();
	       glColor3f(0.929,0.988,0.008);
	       glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	       glTranslatef(-15, 0, 0);
	       glRotatef(90,0,0,1);
	       glRotatef(135,0,1,0);
	       glScalef(7,0.3,0.5);
	       glutSolidCube(5);
	       glPopMatrix();

	       glPushMatrix();
	       glColor3f(1,0,0);
	       glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	       glTranslatef(-15, 0, 0);
	       glRotatef(90,0,0,1);
	       glRotatef(135,0,1,0);
	       glScalef(5,1,3);
	       glutSolidCube(2);
	       glPopMatrix();

	       glPushMatrix();
	       glColor3f(1,0,0);
	       glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	       glTranslatef(-15, 0, 0);
	       glRotatef(90,0,0,1);
	       glRotatef(45,0,1,0);
	       glScalef(5,1,3);
	       glutSolidCube(2);
	       glPopMatrix();

}

void timer(int value)
{
	//tuliskan varibel yang berubah nilainya disini
	putarx +=30;
	putary +=4;
	glutPostRedisplay();
	glutTimerFunc(25,timer,0);
}




















void display(void){
// glutSwapBuffers();
glClearStencil(0); //clear the stencil buffer
glClearDepth(1.0f);

glClearColor(0.0, 0.6, 0.8, 1);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();
gluLookAt(viewx, viewy, viewz, 0.0, 10.0, 0.0, 0.0, 1.0, 0.0);
//gluLookAt(0.0,10.0,3.0,0.0,0.0,0.0,0.0,1.0,0.0);


    glPushMatrix();
    glPopMatrix();
    glPushMatrix();

glBindTexture(GL_TEXTURE_2D, texture[0]);
gambarTanah(_terrainBukit, 0.3f, 0.9f, 0.0f);
glPopMatrix();

glPushMatrix();
gambarAir(_terrainAir, 0.0f, 0.2f, 0.5f);
glPopMatrix();



//pohon1
glPushMatrix();
glTranslatef(-80,0,-120);
glRotatef(90,0,1,0);
pohon();
ranting();
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();
glPopMatrix();

//pohon1
glPushMatrix();
glTranslatef(-30,0,-130);
glRotatef(90,0,1,0);
pohon();
ranting();
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();
glPopMatrix();


//p3
glPushMatrix();
glTranslatef(-135,0,-90);
glRotatef(90,0,1,0);
pohon();
ranting();
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();
glPopMatrix();


//pohon4
glPushMatrix();
glTranslatef(-160,0,-60);
glRotatef(90,0,1,0);
pohon();
ranting();
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();
glPopMatrix();
glPopMatrix();


//pohon5
glPushMatrix();
glTranslatef(-180,0,-0);
glRotatef(90,0,1,0);
pohon();
ranting();
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();
glPopMatrix();
glPopMatrix();


int x=0;
for (int i=1;i<23;i++)
{
//bunga merah
glPushMatrix();
glRotatef(90,0,1,0);
glScalef(1.8, 1.8, 1.8);
glTranslatef(10-x,10,-10);
if ((i>10)&&(i<15))
{}
else
{bungamerah();}
glPopMatrix();
x +=4;
}


x=0;
for (int i=1;i<23;i++)
{
//bunga ungu
glPushMatrix();
glRotatef(90,0,1,0);
glScalef(1.8, 1.8, 1.8);
glTranslatef(10-x,10,-2);
if ((i>10)&&(i<15))
{}
else
{bungapink();}
glPopMatrix();
x +=4;
}


x=0;
for (int i=1;i<23;i++)
{
//bunga kuning
glPushMatrix();
glRotatef(90,0,1,0);
glScalef(1.8, 1.8, 1.8);
glTranslatef(10-x,10,-6);
if ((i>10)&&(i<15))
{}
else
{bungakuning();}
glPopMatrix();
x +=4;
}


//tambahan manggil kincir angin

//kincir1
glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(50,10,-30);
kincir();
glPopMatrix();

//kincir2
glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(50,10,50);
kincir();
glPopMatrix();


glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(45,30,55);
glRotated(putarx,1,0,0);
baling();
glPopMatrix();


glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(40,30,-20);
glRotated(putarx,1,0,0);
baling();
glPopMatrix();

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); //disable the color mask
glDepthMask(GL_FALSE); //disable the depth mask

glEnable(GL_STENCIL_TEST); //enable the stencil testing

glStencilFunc(GL_ALWAYS, 1, 0xFFFFFFFF);
glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE); //set the stencil buffer to replace our next lot of data

//ground
//tanah(); //set the data plane to be replaced
glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); //enable the color mask
glDepthMask(GL_TRUE); //enable the depth mask

glStencilFunc(GL_EQUAL, 1, 0xFFFFFFFF);
glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); //set the stencil buffer to keep our next lot of data

glDisable(GL_DEPTH_TEST); //disable depth testing of the reflection

// glPopMatrix();
glEnable(GL_DEPTH_TEST); //enable the depth testing
glDisable(GL_STENCIL_TEST); //disable the stencil testing
//end of ground
glEnable(GL_BLEND); //enable alpha blending
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //set the blending function
glRotated(1, 0, 0, 0);

glDisable(GL_BLEND);

    glutSwapBuffers();
glFlush();
rot++;
angle++;
}


void init(void){

glEnable(GL_DEPTH_TEST);
glEnable(GL_LIGHTING);
glEnable(GL_LIGHT0);

glDepthFunc(GL_LESS);
glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
glDepthFunc(GL_LEQUAL);
glShadeModel(GL_SMOOTH);
glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
glEnable(GL_CULL_FACE);

glEnable(GL_TEXTURE_2D);
glEnable(GL_TEXTURE_GEN_S);
glEnable(GL_TEXTURE_GEN_T);

initRendering();
_terrainBukit = loadTerrain("TerrainBukit.bmp", 8);
_terrainAir = loadTerrain("TerrainAir.bmp",0);


}











void reshape(int w, int h){
glViewport(0, 0 , (GLsizei) w,(GLsizei)h);
glMatrixMode(GL_PROJECTION);
glLoadIdentity();


gluPerspective(45, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
glMatrixMode(GL_MODELVIEW);

}








static void keyboard(int key, int x, int y) {
switch (key) {
case GLUT_KEY_HOME:
viewy++;
break;
case GLUT_KEY_END:
viewy--;
break;
case GLUT_KEY_UP:
viewz--;
break;
case GLUT_KEY_DOWN:
viewz++;
break;

case GLUT_KEY_RIGHT:
viewx++;
break;
case GLUT_KEY_LEFT:
viewx--;
break;

case GLUT_KEY_F1: {
glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
}
;
break;
case GLUT_KEY_F2: {
glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient2);
glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse2);
glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
}
;
break;
default:
break;
}
}

void keyboard(unsigned char key, int x, int y) {
if (key == 'd') {

spin = spin - 1;
if (spin > 360.0)
spin = spin - 360.0;
}
if (key == 'a') {
spin = spin + 1;
if (spin > 360.0)
spin = spin - 360.0;
}
if (key == 'q') {
viewz++;
}
if (key == 'e') {
viewz--;
}
if (key == 's') {
viewy--;
}
if (key == 'w') {
viewy++;
}
}



int main(int argc, char** argv){
glutInit(&argc, argv);
glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
glutInitWindowSize(800,600);
glutInitWindowPosition(100,100);
glutCreateWindow("Taman Kincir Belanda");
init();

glutDisplayFunc(display);
glutIdleFunc(display);
glutReshapeFunc(reshape);
glutTimerFunc(500, timer, 0);

glutKeyboardFunc (keyboard);
glutSpecialFunc(keyboard);

glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
glLightfv(GL_LIGHT0, GL_POSITION, light_position);

glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
glColorMaterial(GL_FRONT, GL_DIFFUSE);

glutMainLoop();
return 0;
}
