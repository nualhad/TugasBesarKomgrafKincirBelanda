/*
 * Nurul Al Hadi
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

//Loads a terrain from a heightmap.  The heights of the terrain range from
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

/*

int ImageLoad(char *filename, Images *image) {
	FILE *file;
	unsigned long size;
	unsigned long i;
	unsigned short int plane;

	unsigned short int bpp;
	char temp;


	if ((file = fopen(filename, "rb")) == NULL) {
		printf("File Not Found : %s\n", filename);
		return 0;
	}

	fseek(file, 18, SEEK_CUR);

	if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
		printf("Error reading width from %s.\n", filename);
		return 0;
	}

	if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {
		printf("Error reading height from %s.\n", filename);
		return 0;
	}

	size = image->sizeX * image->sizeY * 3;
	// read the planes
	if ((fread(&plane, 2, 1, file)) != 1) {
		printf("Error reading planes from %s.\n", filename);
		return 0;
	}
	if (plane != 1) {
		printf("Planes from %s is not 1: %u\n", filename, plane);
		return 0;
	}
	// read the bitsperpixel
	if ((i = fread(&bpp, 2, 1, file)) != 1) {
		printf("Error reading bpp from %s.\n", filename);

		return 0;
	}
	if (bpp != 24) {
		printf("Bpp from %s is not 24: %u\n", filename, bpp);
		return 0;
	}
	// seek past the rest of the bitmap header.
	fseek(file, 24, SEEK_CUR);
	// read the data.
	image->data = (char *) malloc(size);
	if (image->data == NULL) {
		printf("Error allocating memory for color-corrected image data");
		return 0;
	}
	if ((i = fread(image->data, size, 1, file)) != 1) {
		printf("Error reading image data from %s.\n", filename);
		return 0;
	}
	for (i = 0; i < size; i += 3) {
		temp = image->data[i];
		image->data[i] = image->data[i + 2];
		image->data[i + 2] = temp;
	}
	//
	return 1;
}





Images * loadTexture() {
	Images *image1;

	image1 = (Images *) malloc(sizeof(Images));
	if (image1 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}

	if (!ImageLoad("grass.bmp", image1)) {
		exit(1);
	}
	return image1;
}


Images * loadTextureSatu() {
	Images *image1;

	image1 = (Images *) malloc(sizeof(Images));
	if (image1 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}

	if (!ImageLoad("balon.bmp", image1)) {
		exit(1);
	}
	return image1;
}

Images * loadTextureDua() {
	Images *image1;
	// alokasi memmory untuk tekstur
	image1 = (Images *) malloc(sizeof(Images));
	if (image1 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}

	if (!ImageLoad("kotak.bmp", image1)) {
		exit(1);
	}
	return image1;
}
*/


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

/*
unsigned int texture_balon;
unsigned int texture_kotak;
unsigned int LoadTextureFromBmpFile(char *filename);
*/

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

void display(void){
//    glutSwapBuffers();
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






/*
Images *image1 = loadTexture();
Images *image2 = loadTextureSatu();
Images *image3 = loadTextureDua();

if (image1 == NULL) {
		printf("Image was not returned from loadTexture\n");
		exit(0);
	}

glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


	// Generate texture/ membuat texture
	glGenTextures(2, texture);

------------tekstur balon---------------

	//binding texture untuk membuat texture 2D
	glBindTexture(GL_TEXTURE_2D, texture[1]);


    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

	//menyesuaikan ukuran textur ketika image lebih besar dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //
	//menyesuaikan ukuran textur ketika image lebih kecil dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //

	glTexImage2D(GL_TEXTURE_2D, 0, 3, image2->sizeX, image2->sizeY, 0, GL_RGB,
			GL_UNSIGNED_BYTE, image2->data);
*/


/*
------------tekstur kotak---------------

	//binding texture untuk membuat texture 2D
	glBindTexture(GL_TEXTURE_2D, texture[2]);


	//menyesuaikan ukuran textur ketika image lebih besar dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //
	//menyesuaikan ukuran textur ketika image lebih kecil dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //

	glTexImage2D(GL_TEXTURE_2D, 0, 3, image3->sizeX, image3->sizeY, 0, GL_RGB,
			GL_UNSIGNED_BYTE, image3->data);




	//baris tekstur buatan #belang
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


*/
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
