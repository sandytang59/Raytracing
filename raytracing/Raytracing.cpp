#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <stdlib.h>  
#include <string>
#include <time.h>
#include <math.h>

#include "glut.h"
#include "Algebra3h.cpp"



#ifdef _WIN32
#	include <windows.h>
#else
#	include <sys/time.h>
#endif


#ifdef _WIN32
static DWORD lastTime;
#else
static struct timeval lastTime;
#endif

#define PI 3.14159265
#define ELLIPSOID 0
#define TRIANGLE 1
#define MAX_COUNT 5

using namespace std;

typedef struct RGB
{
	GLfloat r;
	GLfloat g;
	GLfloat b;
}rgb;


/*
function prototype
*/

//rgb getFinalColor_dLight (point p, point pl, rgb pl_i);
void processNormalKeys(unsigned char key, int x, int y);
void init_light();
void reset_finalColor ();
vec4 getEllipNormal(vec3 p);
vec4 getTriangNormal();
mat4 matrixConstructor(vec3 scale,vec3 translate, vec3 Axis, double angleDeg, bool flip);
GLfloat checkHitEllipsoid(mat4 w2o, vec4 ray, vec3 point, GLfloat rad, bool * hits);
GLfloat checkHitTriangle(mat4 w2o, vec4 ray, vec3 point , bool * hits);
rgb rayTracer(vec4 ray, vec3 p, vec4 normal, vec3 eye);
rgb shading_pointLight (vec3 p, vec4 rayIn, vec4 normal, vec3 pl, rgb pl_i, vec3 e);
void initializeMatrix();
int checkVisible(vec3 point, vec3 pl);


//****************************************************
// Some Classes
//****************************************************
class Viewport {
public:
	int w, h; // width and height
};


//****************************************************
// Global Variableso
//****************************************************
Viewport	viewport;

//vec3 origin = {0, 0, 0};    //origin point
rgb ka = {0.0f, 0.0f, 0.0f};  //constant of the ambient light
rgb kd = {0.4f, 0.3f, 0.1f};  //constant of the diffuse light
rgb ks = {0.1f, 0.3f, 0.2f};  //constant of the specular light
rgb kr = {0.3f, 0.5f, 0.6f};  //constant of the reflection
rgb kt = {0.8f, 0.3f, 0.5f};  //constant of the refraction

rgb pl_i[5];  //array of point lights' intensities
rgb dl_i[5];  //array of directional lights' intensities
GLfloat s = 4.0;  //power of specular
vec3 pl[5];  //array of point lights' positions
vec3 dl[5];  //array of directional lights' positions

mat4 O2W[3];   //an array that contain the matresses to convert the objects to the world (initializer at the end of the codes)
mat4 W2O[3];   //an array that contain the matresses to convert the objects back to the object space (initializer at the end of the codes)
mat4 C2W;
mat4 W2C;

vec4 l; //light coming vector
vec4 r; //relection vector
vec4 n; //normal vector
rgb final_c = {0,0,0}; //the final color that the OpenGL should draw
rgb pointLightColor[5];  //the array of the total color of each lights
rgb directionalLightColor[5]; //the array of the total color of each lights

//hit status for every object.
bool hit_ellipse1 = false;
bool hit_ellipse2 = false;
bool hit_triangle = false;

//radius of Ellipsoids
GLfloat radius[2];
vec3 vertex[3];

//counter for ray tracer recursive call
int count = 0;



//****************************************************
// reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
	viewport.w = w;
	viewport.h = h;

	glViewport(0,0,viewport.w,viewport.h);// sets the rectangle that will be the window
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();				// loading the identity matrix for the screen

	//----------- setting the projection -------------------------
	// glOrtho sets left, right, bottom, top, zNear, zFar of the chord system


	// glOrtho(-1, 1 + (w-400)/200.0 , -1 -(h-400)/200.0, 1, 1, -1); // resize type = add
	// glOrtho(-w/400.0, w/400.0, -h/400.0, h/400.0, 1, -1); // resize type = center

	glOrtho(-1, 1, -1, 1, -1, 1);	// resize type = stretch
	//gluOrtho2D(-viewport.w/2,viewport.w/2,-viewport.h/2,viewport.h/2); 

	//glOrtho(-viewport.h/2,viewport.h/2,-viewport.w/2,viewport.w/2,-viewport.w,viewport.w);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//------------------------------------------------------------
}


//****************************************************
// sets the window up
//****************************************************
void initScene(){
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear to black, fully transparent
	//glClearColor(1.0f, 1.0f, 1.0f, 0.0f); // Clear to white, fully transparent	

	glShadeModel(GL_SMOOTH);

	myReshape(viewport.w,viewport.h);
}


//***************************************************
// function that does the actual drawing
//***************************************************
void myDisplay() {
	
	glClear(GL_COLOR_BUFFER_BIT);				// clear the color buffer (sets everything to black)
	
	glMatrixMode(GL_MODELVIEW);					// indicate we are specifying camera transformations
	glLoadIdentity();							// make sure transformation is "zero'd"
	
//ADDING.........................
 

	glBegin(GL_POINTS);

	//initialize first point light pl[0]
	pl[0][0] = 0.8;
	pl[0][1] = 0.9;
	pl[0][2] = 0.9;
	pl_i[0].r = 1.0;
	pl_i[0].g = 1.0;
	pl_i[0].b = 1.0;

	//initialize Ellipsoids radius
	radius[0] = 0.5;
	radius[1] = 0.3;

	//initialize traingle vertex
	vertex[0][0] = 0.0;
	vertex[0][1] = 0.0;
	vertex[0][2] = 0.0;

	vertex[1][0] = 0.5;
	vertex[1][1] = 0.0;
	vertex[1][2] = 0.0;

	vertex[2][0] = 0.2;
	vertex[2][1] = 0.5;
	vertex[2][2] = 0.0;

	//the eye vector in the camara space, world space, all object spaces
	vec3 eye_c = vec3(0.0, 0.0, 0.0);
	vec3 eye_w = C2W * eye_c;

	//assuming LL = (-1, -1) LR = (1 , -1) UL = ( -1, 1) UR = ( 1, 1), in the plane z = -1;
	//vec3 ll = vec3 (-1, -1, -1);
	//vec3 lr = vec3 (1, -1, -1);
	//vec3 ul = vec3 (-1, 1, -1);
	//vec3 ur = vec3 (1, 1, -1);
	GLfloat left = -1.0;
	GLfloat right = 1.0;
	GLfloat top = 1.0;
	GLfloat bottom = -1.0;

	GLfloat t;
	GLfloat t2;
	GLfloat t3;


	//for(GLfloat u= 1/viewport.w ; u < 1; u = u + 1.0/viewport.w){
	//	
	//	for(GLfloat v= 1/viewport.h ; v < 1 ; v = v + 1.0/viewport.h){
	for(float j = 0.0 ; j < viewport.h ; j ++){
		for(float i = 0.0 ; i < viewport.w ; i ++){


			final_c.r = 0.0;
			final_c.g = 0.0;
			final_c.b = 0.0;
			cout << " i " << i << " j " << j << endl;

			GLfloat u = left + ((( right - left) * ( i + 0.5)) / viewport.w);
			GLfloat v = bottom + ((( top - bottom) * ( j + 0.5)) / viewport.h);

			count = 0;
			hit_ellipse1 = false;
			hit_ellipse2 = false;
			hit_triangle = false;

			//vec3 p = u * ( v * ll + (1.0 − v) * ul) + (1.0 − u) * (v * lr + (1.0 - v) * ur);
			//get every pixel in the viewport.
			//vec3 pixTemp = u * (v * ll + (1.0 - v) * ul);
			//pixTemp = pixTemp + (1.0 - u) * (v * lr + (1.0 - v) * ur);
			////change pix[2] back to -1, since pix must be in the plane z = -1.
			//vec3 pix = vec3( pixTemp[0], pixTemp[1], -1);
			
			//generate ray in camara space and world space
/*			vec3 ray_c = vec3( pix[0] - eye_c[0], pix[1] - eye_c[1], pix[2] - eye_c[2]);	*/	
			vec3 pix_c = vec3 (u, v, -1);
			vec3 pix_w = C2W * pix_c;
			vec4 ray_c = vec4(pix_c[0] - eye_c[0], pix_c[1] - eye_c[1], pix_c[2] - eye_c[2], 0.0);
			vec4 ray_w = C2W * ray_c;

			//check if it hits Ellipsoid 1
			t = checkHitEllipsoid(W2O[0], ray_w, eye_w, radius[0],&hit_ellipse1);
			//check if it hits Ellipsoid 2
			t2 = checkHitEllipsoid(W2O[1], ray_w, eye_w, radius[1],&hit_ellipse2);
			//check if it hits traingle
			t3 = checkHitTriangle(W2O[2], ray_w, eye_w , &hit_triangle);


			if(hit_ellipse2 == true){
				//if the ray does not hit anything yet OR the ray is closer to Ellipsoid 2 than the previous object.  
				if(t <= 0 || t2 < t){
					t=t2;
					hit_ellipse1 = false;
				}
				else{
					hit_ellipse2 = false;
				}
				
			}

			
			if(hit_triangle == true){ 
				if(t <= 0 || t3 < t){
					t=t3;
					hit_ellipse2 = false;
					hit_ellipse1 = false;
				}
				else{
					hit_triangle = false;
				}
				
			}

			//check if it hits Triangle	(continued)...		

			if(hit_ellipse1){

				//increment count
				count++;
				//hit point in object space
				vec4 hp_o4 = t * (W2O[0] * ray_w);
				vec3 hp_o3 = W2O[0] * eye_w;
				vec3 hp_o = vec3(hp_o4[0] + hp_o3[0], hp_o4[1] + hp_o3[1], hp_o4[2] + hp_o3[2]);
				//hit point in world space
				vec3 hp_w = O2W[0] * hp_o;

				//generate normal on the point
				//in order to get the normal in the world space for the point, need to calculate O2W ^-T (inverse transpose)
				mat4 mx_calculateNormal = (O2W[0].inverse()).transpose();
				vec4 normal = mx_calculateNormal * getEllipNormal(hp_o);
				normal = vec4 (normal[0], normal[1], normal[2], 0.0);

				//if the ray hit a point, color that point with its local shading and reflection and refration shadings
				final_c = shading_pointLight (hp_w, ray_w, normal, pl[0], pl_i[0], eye_w);
			}
			if (hit_ellipse2){

				//increment count
				count++;
				//hit point in object space
				//vec3 hp_o = W2O[1] * eye_w + t * vec3(W2O[1] * ray_w);
				vec4 hp_o4 = t * (W2O[1] * ray_w);
				vec3 hp_o3 = W2O[1] * eye_w;
				vec3 hp_o = vec3(hp_o4[0] + hp_o3[0], hp_o4[1] + hp_o3[1], hp_o4[2] + hp_o3[2]);

				//hit point in world space
				vec3 hp_w = O2W[1] * hp_o;


				//generate normal on the point
				//in order to get the normal in the world space for the point, need to calculate O2W ^-T (inverse transpose)
				mat4 mx_calculateNormal = (O2W[1].inverse()).transpose();
				vec4 normal = mx_calculateNormal * getEllipNormal(hp_o);
				normal = vec4 (normal[0], normal[1], normal[2], 0.0);

				//if the ray hit a point, color that point with its local shading and reflection and refration shadings
				final_c = shading_pointLight (hp_w, ray_w, normal, pl[0], pl_i[0], eye_w);
			}
			if(hit_triangle){
				//increment count
				count++;
				vec4 hp_o4 = t * (W2O[2] * ray_w);
				vec3 hp_o3 = W2O[2] * eye_w;
				vec3 hp_o = vec3(hp_o4[0] + hp_o3[0], hp_o4[1] + hp_o3[1], hp_o4[2] + hp_o3[2]);

				//hit point in world space
				vec3 hp_w = O2W[2] * hp_o;


				//generate normal on the point
				//in order to get the normal in the world space for the point, need to calculate O2W ^-T (inverse transpose)
				mat4 mx_calculateNormal = (O2W[2].inverse()).transpose();
				vec4 normal = mx_calculateNormal * getTriangNormal();
				normal = vec4 (normal[0], normal[1], normal[2], 0.0);

				//if the ray hit a point, color that point with its local shading and reflection and refration shadings
				final_c = shading_pointLight (hp_w, ray_w, normal, pl[0], pl_i[0], eye_w);
			}

			glColor3f(final_c.r,final_c.g,final_c.b);
			glVertex3f(u , v, 0.0f);
			
				
		}
			
	}



	glEnd();

	
	glFlush();
	glutSwapBuffers();					// swap buffers (we earlier set double buffer)
}



//****************************************************
// called by glut when there are no messages to handle
//****************************************************
void myFrameMove() {
	//nothing here for now
#ifdef _WIN32
	Sleep(10);						//give ~10ms back to OS (so as not to waste the CPU)
#endif
	glutPostRedisplay(); // forces glut to call the display function (myDisplay())
}




//****************************************************
// the usual stuff, nothing exciting here
//****************************************************
int main(int argc, char *argv[]) {

	init_light();


  	//This initializes glut
  	glutInit(&argc, argv);
  
  	//This tells glut to use a double-buffered window with red, green, and blue channels 
  	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

  	// Initalize theviewport size
  	//viewport.w = 20;
  	//viewport.h = 20;
	viewport.w = 640;
	viewport.h = 480;


  	//The size and position of the window
  	glutInitWindowSize(viewport.w, viewport.h);
  	glutInitWindowPosition(0, 0);


	//glutInitWindowPosition (100, 100);
	 glutCreateWindow("Raytracing");

  	initScene();							// quick function to set up scene


	initializeMatrix();						// Initialize the matrices.
	//cout << C2W;
	glutDisplayFunc(myDisplay);				// function to run when its time to draw something
	glutReshapeFunc(myReshape);				// function to run when the window gets resized
	glutIdleFunc(myFrameMove);				// function to run when not handling any other task


	glutKeyboardFunc(processNormalKeys);    //exit when space is pressed
 

	glutMainLoop();							// infinite loop that will keep drawing and resizing and whatever else
	

  
  	return 0;
}





rgb shading_pointLight (vec3 p, vec4 rayIn, vec4 normal, vec3 pl, rgb pl_i, vec3 e)
{
	int visible = 1;
	visible = checkVisible(p, pl);
	//final return color.
	rgb finalColor = {0, 0, 0};
	rgb ambientColor = {0, 0, 0};
	rgb diffuseColor = {0, 0, 0};
	rgb specularColor = {0, 0, 0};
	rgb reflection = {0 , 0 , 0};
	rgb refraction = {0, 0 , 0};

	//compute the light unit vector
	//GLfloat x = (pl.x - p.x) / getLength(pl, p);
	//GLfloat y = (pl.y - p.y) / getLength(pl, p);
	//GLfloat z = (pl.z - p.z) / getLength(pl, p);
	vec4 light = vec4(pl[0] - p[0], pl[1] - p[1], pl[2] - p[2], 0.0);
	l = light / light.length();


	//compute the eye to point unit vector
	//x = (e.x - p.x) / getLength(e, p);
	//y = (e.y - p.y) / getLength(e, p);
	//z = (e.z - p.z) / getLength(e, p);

	vec4 viewer = vec4(e[0] - p[0], e[1] - p[1], e[2] - p[2], 0.0);
	vec4 v = viewer / viewer.length();


	//compute the normal vector

	//get unit vector of normal
	n = normal / normal.length();


	//compute the reflection light unit vector
	GLfloat cos = l * n;
	r = (2.0 * cos) * n;
	r = r * (-1);
	r = r + l;
	//r.x = -(2 * n.x * dotProduct(l, n) - l.x);
	//r.y = -(2 * n.y * dotProduct(l, n) - l.y); 
	//r.z = -(2 * n.z * dotProduct(l, n) - l.z);

	//compute the ambient color
	ambientColor.r = ka.r * pl_i.r;
	ambientColor.g = ka.g * pl_i.g;
	ambientColor.b = ka.b * pl_i.b;

	//compute the diffuse color
	GLfloat scaler_d = max ((l*n), 0.0f);

	diffuseColor.r = kd.r * pl_i.r * scaler_d;
	diffuseColor.g = kd.g * pl_i.g * scaler_d;
	diffuseColor.b = kd.b * pl_i.b * scaler_d;

	//compute the specular color
	GLfloat scaler_s = max((r*v) , 0.0f);
	scaler_s = pow(scaler_s, s);


	specularColor.r = ks.r * pl_i.r * scaler_s;
	specularColor.g = ks.g * pl_i.g * scaler_s;
	specularColor.b = ks.b * pl_i.b * scaler_s;

	//compute the reflection color
	rgb reflect = rayTracer(rayIn, p, n, e);
	reflection.r = kr.r * reflect.r;
	reflection.g = kr.g * reflect.g;
	reflection.b = kr.b * reflect.b;

	//compute the refraciton color
	//rgb refract = refraction_compute();
	/*refraction.r = kt.r * refrac.r;
	refraction.g = kt.g * refrac.g;
	refraction.b = kt.b * refrac.b;*/


	//compute final color
	finalColor.r = ambientColor.r + visible * (diffuseColor.r + specularColor.r) + reflection.r ;//+ refraction.r;
	finalColor.g = ambientColor.g + visible * (diffuseColor.g + specularColor.g) + reflection.g ;//+ refraction.g;
	finalColor.b = ambientColor.b + visible * (diffuseColor.b + specularColor.b) + reflection.b ;//+ refraction.b;


	return finalColor;
}

//rgb getFinalColor_dLight (point p, point dl, rgb dl_i)
//{
//	//final return color.
//	rgb finalColor = {0, 0, 0};
//	rgb ambientColor = {0, 0, 0};
//	rgb diffuseColor = {0, 0, 0};
//	rgb specularColor = {0, 0, 0};
//
//	//compute the light unit vector
//	l.n[0] = (dl.x - origin.x) / getLength(dl, p);
//	l.y = (dl.y - origin.y) / getLength(dl, p);
//	l.z = (dl.z - origin.z) / getLength(dl, p);
//
//	//compute the viewer unit vector
//	v.x = (viewer.x - p.x) / getLength(viewer, p);
//	v.y = (viewer.y - p.y) / getLength(viewer, p);
//	v.z = (viewer.z - p.z) / getLength(viewer, p);
//
//	//compute the normal vector
//	n.x = p.x / getLength(p,origin);
//	n.y = p.y / getLength(p,origin);
//	n.z = p.z / getLength(p,origin);
//
//	//compute the reflection light unit vector
//	r.x = -(2 * n.x * dotProduct(l, n) - l.x);
//	r.y = -(2 * n.y * dotProduct(l, n) - l.y); 
//	r.z = -(2 * n.z * dotProduct(l, n) - l.z);
//
//	//compute the ambient color
//	ambientColor.r = ka.r * dl_i.r;
//	ambientColor.g = ka.g * dl_i.g;
//	ambientColor.b = ka.b * dl_i.b;
//
//	//compute the diffuse color
//	GLfloat scaler_d = max ((dotProduct(l, n)), 0.0f);
//
//	diffuseColor.r = kd.r * dl_i.r * scaler_d;
//	diffuseColor.g = kd.g * dl_i.g * scaler_d;
//	diffuseColor.b = kd.b * dl_i.b * scaler_d;
//
//	//compute the specular color
//	GLfloat scaler_s = max((dotProduct(r, v)) , 0.0f);
//	scaler_s = pow(scaler_s, s);
//
//	specularColor.r = ks.r * dl_i.r * scaler_s;
//	specularColor.g = ks.g * dl_i.g * scaler_s;
//	specularColor.b = ks.b * dl_i.b * scaler_s;
//
//	//compute final color
//	finalColor.r = ambientColor.r + diffuseColor.r + specularColor.r;
//	finalColor.g = ambientColor.g + diffuseColor.g + specularColor.g;
//	finalColor.b = ambientColor.b + diffuseColor.b + specularColor.b;
//
//	return finalColor;
//}



	//check if the ray hit ellipsoids and return the t value if it does, return 0 if it does not
GLfloat checkHitEllipsoid(mat4 w2o, vec4 ray, vec3 point, GLfloat rad, bool * hits){
	GLfloat t = 0.0f;
	vec4 ray_o = w2o * ray;
	vec3 point_o = w2o * point;

	//R(t) = E + t(P - E)
	//find the t value for the intersect point in the ellipsoid object space.
	//from sphere equation
	GLfloat a = ray_o * ray_o;
	GLfloat b = 2 * ray_o * vec4(point_o);
	GLfloat c = point_o * point_o - (rad * rad);
	GLfloat delta = b*b - 4*a*c;

	if(delta <= 0){
		//no intersection or just a tangent to the ellipsoid
		t = 0;
		*hits = false;
	}
	else{
		//two solution means two intersections, get the smaller one
		t = (-b - sqrt(delta)) / (2*a);
			*hits = true;
	/*	}*/
	}

	return t;
}

GLfloat checkHitTriangle(mat4 w2o, vec4 ray, vec3 point , bool * hits){
	GLfloat t = 0.0f;
	GLfloat B = 0.0f;
	GLfloat r = 0.0f;

	ray = w2o * ray;
	point = w2o * point;

	GLfloat a = vertex[0][0] - vertex[1][0]; // vertex[0] 's x - vertex[1] 's x
	GLfloat b = vertex[0][1] - vertex[1][1]; // vertex[0] 's y - vertex[1] 's y
	GLfloat c = vertex[0][2] - vertex[1][2]; // vertex[0] 's z - vertex[1] 's z

	GLfloat d = vertex[0][0] - vertex[2][0]; // vertex[0]'s x - vertex[2]'s x
	GLfloat e = vertex[0][1] - vertex[2][1]; // vertex[0]'s y - vertex[2]'s y
	GLfloat f = vertex[0][2] - vertex[2][2]; // vertex[0]'s z - vertex[2]'s z

	GLfloat g = ray[0]; // ray's x
	GLfloat h = ray[1]; // ray's y
	GLfloat i = ray[2]; // ray's z

	GLfloat j = vertex[0][0] - point[0]; // vertex[0]'s x - point's x
	GLfloat k = vertex[0][1] - point[1]; // vertex[0]'s y - point's y
	GLfloat l = vertex[0][2] - point[2]; // vertex[0]'s z - point's z

	GLfloat M = a * (e * i - h * f) + b * ( g * f - d * i ) + c * ( d * h - e * g);
	
	//beta
	B = (j * ( e * i - h * f ) + k * ( g * f - d * i ) + l * ( d * h - e * g )) / M;

	//r
	r = (i * ( a * k - j * b ) + h * ( j * c - a * l ) + g * ( b * l - k * c )) / M;

	//t 
	t = (-1) * ((f * ( a * k - j * b ) + e * ( j * c - a * l ) + g * ( b * l - k * c )) / M);

	//check if it hits
	if( r < 0 || r > 1){
		*hits = false;
		t = 0;
	}
	else if( B < 0 || B > (1-r)){
		*hits = false;
		t = 0;
	}
	else{
		*hits = true;
	}

	return t;

}


rgb rayTracer(vec4 ray, vec3 p, vec4 normal, vec3 eye){

	rgb Ir= {0.0, 0.0, 0.0};
	
	//If the count reaches max_count, stop recursive call
	if(count >= MAX_COUNT){
		return Ir;
	}

	//generate reflected ray
	vec4 rRay;
	//find unit vector of ray
	vec4 uni_ray = ray/ray.length();


	GLfloat cos = ((-1) * uni_ray) * normal;
	rRay = (2.0 * cos) * normal;
	rRay = rRay * (-1);
	rRay = rRay - uni_ray;

	//r.x = -(2 * n.x * dotProduct(l, n) - l.x);
	//r.y = -(2 * n.y * dotProduct(l, n) - l.y); 
	//r.z = -(2 * n.z * dotProduct(l, n) - l.z);

	hit_ellipse1 = false;
	hit_ellipse2 = false;
	hit_triangle = false;

	//check if the rRay hits ellipsiod1
	GLfloat t = checkHitEllipsoid(W2O[0], rRay, p, radius[0], &hit_ellipse1);
	//check if the rRay hits ellipsoid2
	GLfloat t2 = checkHitEllipsoid(W2O[1], rRay, p, radius[1], &hit_ellipse2);
	//check if the rRay hits triangle
	GLfloat t3 = checkHitTriangle(W2O[2], rRay, p, &hit_triangle);

	if(hit_ellipse2 == true){
		//if the ray does not hit anything yet OR the ray is closer to Ellipsoid 2 than the previous object.  
		if(t <= 0 || t2 < t){
			t=t2;
			hit_ellipse1 = false;
		}
		else{
			hit_ellipse2 = false;
		}		
	}

				
	if(hit_triangle == true){ 
		if(t <= 0 || t3 < t){
			t=t3;
			hit_ellipse2 = false;
			hit_ellipse1 = false;
		}
		else{
			hit_triangle = false;			
		}	
	}


	if(hit_ellipse1){

		//increment count
		count++;

		//hit point in object space
		vec4 hp_o4 = t * (W2O[0] * rRay);
		vec3 hp_o3 = W2O[0] * p;
		vec3 hp_o = vec3(hp_o4[0] + hp_o3[0], hp_o4[1] + hp_o3[1], hp_o4[2] + hp_o3[2]);
		//hit point in world space
		vec3 hp_w = O2W[0] * hp_o;

		//generate normal on the point
		//in order to get the normal in the world space for the point, need to calculate O2W ^-T (inverse transpose)
		mat4 mx_calculateNormal = (O2W[0].inverse()).transpose();
		vec4 normal = mx_calculateNormal * getEllipNormal(hp_o);
		normal = vec4 (normal[0], normal[1], normal[2], 0.0);

				
		//if the ray hit a point, color that point with its local shading and reflection and refration shadings
		Ir = shading_pointLight(hp_w, rRay, normal, pl[0], pl_i[0], eye);
	}
	if(hit_ellipse2){

		//increment count
		count++;

		//hit point in object space
        vec4 hp_o4 = t * (W2O[1] * rRay);
		vec3 hp_o3 = W2O[1] * p;
		vec3 hp_o = vec3(hp_o4[0] + hp_o3[0], hp_o4[1] + hp_o3[1], hp_o4[2] + hp_o3[2]);
		//hit point in world space
		vec3 hp_w = O2W[1] * hp_o;

		//generate normal on the point
		//in order to get the normal in the world space for the point, need to calculate O2W ^-T (inverse transpose)
		mat4 mx_calculateNormal = (O2W[1].inverse()).transpose();
		vec4 normal = mx_calculateNormal * getEllipNormal(hp_o);
		normal = vec4 (normal[0], normal[1], normal[2], 0.0);

		Ir = shading_pointLight (hp_w, rRay, normal, pl[0], pl_i[0], eye);
	}
	if(hit_triangle){
	
		//increment count
		count++;

		//hit point in object space
        vec4 hp_o4 = t * (W2O[2] * rRay);
		vec3 hp_o3 = W2O[2] * p;
		vec3 hp_o = vec3(hp_o4[0] + hp_o3[0], hp_o4[1] + hp_o3[1], hp_o4[2] + hp_o3[2]);
		//hit point in world space
		vec3 hp_w = O2W[2] * hp_o;

		//generate normal on the point
		//in order to get the normal in the world space for the point, need to calculate O2W ^-T (inverse transpose)
		mat4 mx_calculateNormal = (O2W[2].inverse()).transpose();
		vec4 normal = mx_calculateNormal * getEllipNormal(hp_o);
		normal = vec4 (normal[0], normal[1], normal[2], 0.0);

		Ir = shading_pointLight (hp_w, rRay, normal, pl[0], pl_i[0], eye);
	}
	return Ir;

}


void processNormalKeys(unsigned char key, int x, int y) {

	if (key == 32)  //key == "space" 
		exit(0) ;
}



void init_light(){
	
	for (int i = 0 ; i< 5 ; i++){
		pl_i[i].r = 0.0f;
		pl_i[i].g = 0.0f;
		pl_i[i].b = 0.0f;

		dl_i[i].r = 0.0f;
		dl_i[i].g = 0.0f;
		dl_i[i].b = 0.0f;

		pl[i][0] = 0.0f;
		pl[i][1]= 0.0f;
		pl[i][2]= 0.0f;

		dl[i][0]= 0.0f;
		dl[i][1]= 0.0f;
		dl[i][2]= 0.0f;

		pointLightColor[i].r = 0.0f;
		pointLightColor[i].g = 0.0f;
		pointLightColor[i].b = 0.0f;


		directionalLightColor[i].r = 0.0f;
		directionalLightColor[i].g = 0.0f;
		directionalLightColor[i].b = 0.0f;
	}

}


void reset_finalColor (){

	final_c.r = 0.0f;
	final_c.g = 0.0f;
	final_c.b = 0.0f;
		
}

//compute the normal vector of the points in ELLIPSOID
vec4 getEllipNormal(vec3 p){

	vec4 normal;
	vec3 temp = p / p.length();
	normal = vec4(temp[0], temp[1], temp[2], 0.0);

	return normal;
}


//compute the normal vector of TRIANGLE
vec4 getTriangNormal(){

	vec3 temp1 = vertex[0] - vertex[1];
	vec3 temp2 = vertex[0] - vertex[2];
	vec3 n = temp1 ^ temp2;
	vec4 normal = vec4(n[0], n[1], n[2], 0.0);
	return normal;
}

//construct a transform matress
mat4 matrixConstructor(vec3 scale,vec3 translate, vec3 Axis, double angleDeg, bool flip){
	//construct the matress that transform the object to the world space.
	mat4 matrix = mat4();
	//scaling
	mat4 matS = scaling3D(scale);
	//translate
	mat4 matT = translation3D(translate);
	//rotate
	mat4 matR = rotation3D(Axis, angleDeg);
	
	matrix = matS * matR * matT;
	return matrix;
}

void initializeMatrix(){
	//object number 0 is Ellipse 1
	//object number 1 is Ellipse 2
	//object number 2 is traingle


	//contructor O2W, W2O, C2W, W2C matresses
	//O2W
	//Ellipsoid 1
	vec3 vScale = vec3 ( 1.0, 2.0, 1.0);
	vec3 vTranslate = vec3( 0.5, -0.05, 0.0);
	vec3 vAxis = vec3(1.0, 1.0, 0.0);
	double ang = -25.0;
	bool flip = false;
	O2W[0] = matrixConstructor(vScale,vTranslate, vAxis, ang, flip);
	//Ellipsoid 2
	vScale = vec3 (2.0, 1.0, 1.0);
	vTranslate = vec3( -0.5 , 0.0, -0.2);
	vAxis = vec3(0.0, 1.0, 0.0);
    ang = 0.0;
	flip = false;
	O2W[1] = matrixConstructor(vScale,vTranslate, vAxis, ang, flip);

	//Traingle.
	vScale = vec3 (1.5, 2.0, 1.3);
	vTranslate = vec3( -0.1 , 0.7 , 0.0);
	vAxis = vec3(1.0, 1.0, 0.0);
    ang = 5.0;
	flip = false;
	O2W[2] = matrixConstructor(vScale,vTranslate, vAxis, ang, flip);

	//W2O
	W2O[0] = O2W[0].inverse();
	W2O[1] = O2W[1].inverse();
	W2O[2] = O2W[2].inverse();
	
	//C2W
	vScale = vec3( 1.0, 1.0, 1.0);
	vTranslate = vec3( 0.0 ,1.0, 2.0);
	vAxis = vec3(1.0, 1.0, 0.0);
	ang = -45.0;
	flip = false;
	C2W = matrixConstructor(vScale,vTranslate, vAxis, ang, flip);

	//W2C
	W2C = C2W.inverse();
}

int checkVisible(vec3 point, vec3 pl){
	int visible = 1;
	vec4 rayToLight;
	vec3 temp = pl -point;
	rayToLight = vec4(temp[0], temp[1], temp[2], 0.0);

	hit_ellipse1 = false;
	hit_ellipse2 = false;
	hit_triangle = false;

	//cout << " hit_ellipse1 " << hit_ellipse1 << " hit_ellipse2 " << hit_ellipse1 << " hit_triangle " << hit_ellipse1 << endl;

	//check if the rRay hits ellipsiod1
	GLfloat t = checkHitEllipsoid(W2O[0], rayToLight, point, radius[0], &hit_ellipse1);
	//check if the rRay hits ellipsoid2
	GLfloat t2 = checkHitEllipsoid(W2O[1], rayToLight, point, radius[1], &hit_ellipse2);
	//check if the rRay hits triangle
	GLfloat t3 = checkHitTriangle(W2O[2], rayToLight, point, &hit_triangle);

	if(hit_triangle && t3 > 0){
		visible = 0;
	}
	else if( hit_ellipse1 && t> 0){
		visible = 0;
	}
	else if( hit_ellipse2 && t2 > 0){
		visible = 0;
	}
	return visible;
	
}