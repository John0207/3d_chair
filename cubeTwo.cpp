/*
 *
 *
 *  Created on: June 12, 2020
 *      Author: John deLuccia
 */
// Header Inclusions
#include<iostream>
#include<GL/glew.h>
#include<GL/freeglut.h>
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include "SOIL2/SOIL2.h"

//GLM Math Header Inclusions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;  //standard namespace

#define WINDOW_TITLE "Modern OpenGL" //window title macro

//shader program macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

//variable declaration for shader, window size initialization, buffer and array objects
GLint shaderProgram, cubeShaderProgram, WindowWidth = 800, WindowHeight = 600, lampShaderProgram;
GLuint VBO, VAO, EBO, texture, LightVAO; //CubeVAO;

//subject postion and scale
glm::vec3 cubePosition(0.0f, 0.0f, 0.0f);
glm::vec3 cubeScale(2.0f);

GLfloat cameraSpeed = 0.0005f; // movement speed per frame

GLchar currentKey; //store key pressed

//variables for mouse movement
GLfloat lastMouseX = 400, lastMouseY = 300;
GLfloat mouseXOffset, mouseYOffset, yaw = 0.0f, pitch = 0.0f;
GLfloat sensitivity = 0.005f;
bool mouseDetected = true;


//global vector declarations
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 front;

//Function prototypes
void UResizeWindow(int, int);
void URenderGraphics(void);
void UCreateShader(void);
void UCreateBuffers(void);
void UKeyboard(unsigned char key, int x, int y);
void UKeyReleased(unsigned char key, int x, int y);
void UMouseMove(int x, int y);
void UMouseMoveWithState(int button, int state, int x, int y);
void UMousePressedMove(int x, int y);
void UGenerateTexture();

//light position and scale
glm::vec3 lightScale(0.3f);
glm::vec3 lightPosition(-1.0f, 0.5f, 0.5f);

//Cube and light color
glm::vec3 objectColor(0.6f, 0.5f, 0.75f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

// Vertex shader source
const GLchar* cubeVertexShaderSource = GLSL(330,
  layout(location = 0) in vec3 position;
  layout(location = 1) in vec3 normal;
  out vec3 Normal;
  out vec3 FragmentPos;
// Global variables for transform matrices
  uniform mat4 model;
  uniform mat4 view;
  uniform mat4 projection;

void main() {
		gl_Position = projection * view * model * vec4(position, 1.0f);
		FragmentPos = vec3(model * vec4(position, 1.0f));
		Normal = mat3(transpose(inverse(model))) * normal;
	}
);

// Fragment shader source
const GLchar* cubeFragmentShaderSource = GLSL(330,
		in vec3 Normal;
		in vec3 FragmentPos;
		out vec4 cubeColor;
		uniform vec3 objectColor;
		uniform vec3 lightColor;
		uniform vec3 lightPos;
		uniform vec3 viewPosition;

	void main() {

		//calculate ambient lighting
		float ambientStrength = 0.1f; //set ambient or global lighting strength
		vec3 ambient = ambientStrength * lightColor; //generate ambient light color

		//calculate diffues lighting
		vec3 norm = normalize(Normal); //normalize vectors to 1 unit
		vec3 lightDirection = normalize(lightPos - FragmentPos); //calculate distance
		float impact = max(dot(norm, lightDirection), 0.0); //calculate diffuse impact
		vec3 diffuse = impact * lightColor; //generate diffuse light color

		//calculate specular lighting
		float specularIntensity = 5.0f; //set specular light strength
		float highlightSize = 16.0f; //set speculat highlight size
		vec3 viewDir = normalize(viewPosition - FragmentPos); //calculate view direction
		vec3 reflectDir = reflect(-lightDirection, norm); //calculate reflection vector

		//calculate specular component
		float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
		vec3 specular = specularIntensity * specularComponent * lightColor;

		//calculate phong result
		vec3 phong = (ambient + diffuse + specular) * objectColor;
		cubeColor = vec4(phong, 1.0f); //send lighting results to GPU
	}
);


//Vertex shader source code
const GLchar* vertexShaderSource = GLSL(330,
		layout (location = 0) in vec3 position; //vertex data from vertex attrib pointer 0
		layout(location = 2) in vec2 textureCoordinate;
		out vec2 mobileTextureCoordinate;
		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;

void main(){
	gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
	mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y);
}
);


//fragment shader source code
const GLchar * fragmentShaderSource = GLSL(330,
		in vec2 mobileTextureCoordinate;
		out vec4 gpuTexture;
		uniform sampler2D uTexture;
	void main(){
		gpuTexture = texture(uTexture, mobileTextureCoordinate);
	}
);

//lamp shader source
const GLchar * lampVertexShaderSource = GLSL(330,
		layout (location = 0) in vec3 position; //VAP position 0 for vertex position data
		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;

		void main()
		{
			gl_Position = projection * view * model * vec4(position, 1.0f); //transforms verticies
		}
);

// Fragment shader source code
const GLchar * lampFragmentShaderSource = GLSL(330,
		out vec4 color; //for outgoing lamp color
		void main()
		{
			color = vec4(1.0f); //set color to white (1.0f, 1.0f, 1.0f)
		}
);


//boolean variables for alt and mouse buttons
bool LButtonDown = false;
bool altKeyDown = false;
bool RButtonDown = false;

//Main program
int main(int argc, char* argv[]){
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutCreateWindow(WINDOW_TITLE);
	glutReshapeFunc(UResizeWindow);
	glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK)
		{
			std::cout << "Failed to initialize GLEW" << std::endl;
			return -1;
		}

	UCreateShader();
	UCreateBuffers();
	UGenerateTexture();

	//use the shader program
	glUseProgram(shaderProgram);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);//set background color

	glutDisplayFunc(URenderGraphics);//function for displaying graphics

	glutPassiveMotionFunc(UMouseMove);//Function to handle mouse movement

	glutMouseFunc(UMouseMoveWithState);//function to handle the state of the mouse (clicked or not)

	glutMotionFunc(UMousePressedMove);//function to handle continuous mouse motion

	glutKeyboardFunc(UKeyboard);//function used for handling the keyboard

	glutKeyboardUpFunc(UKeyReleased);//function which prints to console when a key is released

	glutMainLoop();

	//Destroys Buffer objects once used
	glDeleteVertexArrays(1, &LightVAO);
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	return 0;
	}

	//resize Window
void UResizeWindow(int w, int h)
{
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, WindowWidth, WindowHeight);
}


void UKeyboard(unsigned char key, GLint x, GLint y)
{

//switch statement to print various keys used to the console, used in debugging
	switch(key){

	case'w':
		currentKey = key;
		//cout<<"You pressed w"<<endl;
		break;

	case's':
			currentKey = key;
			//cout<<"You pressed s"<<endl;
			break;

	case'a':
			currentKey = key;
			//cout<<"You pressed a"<<endl;
			break;

	case'd':
			currentKey = key;
			//cout<<"You pressed d"<<endl;
			break;

	case'o':
				currentKey = key;
				//cout<<"You pressed o"<<endl;
				break;

	/*default:
			cout<<"You pressed a key"<<endl;*/
	}
}

//prints key released to the console, used in debugging
void UKeyReleased(unsigned char key, GLint x, GLint y)
{
	//cout<<"Key released"<<endl;
	currentKey = 0;
}

void UMouseMove(int x, int y)
{
	//used for debugging
	//cout << "Mouse State" << LButtonDown <<  endl;

	//Immediately replaces center locked coordinates with the new mouse coordinates
	if(mouseDetected)
	{
		lastMouseX = x;
		lastMouseY = y;
		mouseDetected = false;
	}

	//gets the direction the mouse was moved in x and y
	mouseXOffset = x - lastMouseX;
	mouseYOffset = lastMouseY - y; //inverted y

	//updates with new mouse coordinates
	lastMouseX = x;
	lastMouseY = y;

	//applies sensitivity to mouse direction
	mouseXOffset *= sensitivity;
	mouseYOffset *= sensitivity;

	//accumulates the yaw and pitch variables
	yaw += mouseXOffset;
	pitch += mouseYOffset;

	//maintains a 90 degree pitch for gimbal lock
	if(pitch > 89.0f * 3.141593 / 180.00)
		pitch = 89.0f * 3.141593 / 180.00;

	if(pitch < -89.0f * 3.141593 / 180.00)
		pitch = -89.0f * 3.141593 / 180.00;

	//cout<<"pitch = "<< pitch <<" yaw = "<< yaw <<endl;


	//orbits around center
	front.x = 10.0f * cos(yaw);
	front.y = 10.0f * sin(pitch);
	front.z = sin(yaw) * cos(pitch) * 10.0f;
	int mod = glutGetModifiers();
	if ((mod != GLUT_ACTIVE_ALT))
		altKeyDown = false;
	else
		altKeyDown = LButtonDown || RButtonDown;
	//used in debugging
    //cout<<"("<< x << "," << y << ")" << endl;

}

void UMousePressedMove(int x, int y)
{
		//cout << "Mouse State" << LButtonDown <<  endl; used for debugging
	//Immediately replaces center locked coordinates with the new mouse coordinates
	if(mouseDetected)
	{
		lastMouseX = x;
		lastMouseY = y;
		mouseDetected = false;
	}

	//gets the direction the mouse was moved in x and y
	mouseXOffset = x - lastMouseX;
	mouseYOffset = lastMouseY - y; //inverted y

	//updates with new mouse coordinates
	lastMouseX = x;
	lastMouseY = y;

	//applies sensitivity to mouse direction
	mouseXOffset *= sensitivity;
	mouseYOffset *= sensitivity;

	//accumulates the yaw and pitch variables
	yaw += mouseXOffset;
	pitch += mouseYOffset;

	if(pitch > 89.0f * 3.141593 / 180.00)
		pitch = 89.0f * 3.141593 / 180.00;

	if(pitch < -89.0f * 3.141593 / 180.00)
		pitch = -89.0f * 3.141593 / 180.00;

	//cout<<"pitch = "<< pitch <<" yaw = "<< yaw <<endl;

	//orbits around center
	front.x = 10.0f * cos(yaw);
	front.y = 10.0f * sin(pitch);
	front.z = sin(yaw) * cos(pitch) * 10.0f;
	//used for handling the alt key
	int mod = glutGetModifiers();
	if ((mod != GLUT_ACTIVE_ALT))
		altKeyDown = false;
	else
		altKeyDown = LButtonDown || RButtonDown;

    //cout<<"("<< x << "," << y << ")" << endl;  used for debugging

}



void UMouseMoveWithState(int button, int state, int x, int y){
		if(button == GLUT_LEFT_BUTTON) {
			if(state == GLUT_DOWN){
				LButtonDown = true;
				//cout<<"Left Mouse is pressed"<<endl;
			}
			else{
				LButtonDown = false;
			//cout<<"Left Mouse is released"<<endl;
			}
		}

		if( button == GLUT_RIGHT_BUTTON) {
					if (state == GLUT_DOWN){
						RButtonDown = true;
						//cout<<"Right is pressed"<<endl;
					}
					else{
						RButtonDown = false;
					//cout<<"Right is released"<<endl;
					}
				}
}



void URenderGraphics(void)
{
	glEnable(GL_DEPTH_TEST); // enable z depth
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//clears the screen
	glBindVertexArray(VAO); // Activate the vertex array object before rendering and transforming them
	//transforms the object
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0.0, 0.0f, 0.0f)); // place the object at the center of the viewport
	model = glm::rotate(model, 45.0f, glm::vec3(1.0f, 1.0f, 1.0f)); //rotate the object on the XYZ
	model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f)); // Increase object size by scale of 2

	glm::mat4 view;
//handles rotating the object using the left mouse key and alt, and handles zooming in and out with the right key and alt
	if (!altKeyDown){
		view = glm::translate(view, glm::vec3(0.5f, 0.0f, -5.0f)); //moves the world .5units on x and -5 units on z
	}
	else{
		//cout<<"you pressed alt key"<<endl;
		if (LButtonDown){
			CameraForwardZ = front;
			view = glm::lookAt(CameraForwardZ, cameraPosition, CameraUpY);
		}
		if(RButtonDown){
			if(currentKey == 'w')
				cameraPosition += cameraSpeed * CameraForwardZ;
			if(currentKey == 's')
				cameraPosition -= cameraSpeed * CameraForwardZ;
			view = glm::lookAt(cameraPosition, cameraPosition + CameraForwardZ, CameraUpY);
		}
	}

	// Use and Activate the cube shader
		glUseProgram(cubeShaderProgram);
		//glBindVertexArray(CubeVAO);

		//transform the cube
		model = glm::translate(model, cubePosition);
		model = glm::scale(model, cubeScale);

//creates a perspective projection
	glm::mat4 projection;
//if o key is pressed switched to orthographic projection.
	if(currentKey == 'o')
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	else
		projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);

	GLint modelLoc, viewLoc, projLoc, objectColorLoc, lightColorLoc, lightPositionLoc, viewPositionLoc;


	//reference matrix uniforms from the cube shader
	modelLoc = glGetUniformLocation(cubeShaderProgram, "model");
	viewLoc = glGetUniformLocation(cubeShaderProgram, "view");
	projLoc = glGetUniformLocation(cubeShaderProgram, "projection");

	//pass matrix data to the cube shader programs matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//reference matrix uniforms from the cube shader program for the cube color light color ligiht position
	objectColorLoc = glGetUniformLocation(cubeShaderProgram, "objectColor");
	lightColorLoc =  glGetUniformLocation(cubeShaderProgram, "lightColor");
	lightPositionLoc = glGetUniformLocation(cubeShaderProgram, "lightPosition");
	viewPositionLoc = glGetUniformLocation(cubeShaderProgram, "viewPosition");

	//pass color light and camera data to the cube shader
	glUniform3f(objectColorLoc, objectColor.r, objectColor.g, objectColor.b);
	glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b);
	glUniform3f(lightPositionLoc, lightPosition.x, lightPosition.y, lightPosition.z);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);


	//retrieves and passes transforms matrices to shader program
	 modelLoc = glGetUniformLocation(shaderProgram, "model");
	 viewLoc = glGetUniformLocation(shaderProgram, "view");
	 projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

//draws triangles
	glDrawElements(GL_TRIANGLES, 36*6, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0); //deactivate the vertex array object

	//use lamp shader and activate the lamp vertex
	glBindVertexArray(LightVAO);

	//transform the cube
	model = glm::translate(model, lightPosition);
	model = glm::scale(model, lightScale);




	//reference matrix uniforms from the cube shader
	modelLoc = glGetUniformLocation(lampShaderProgram, "model");
	viewLoc = glGetUniformLocation(lampShaderProgram, "view");
	projLoc = glGetUniformLocation(lampShaderProgram, "projection");

	//pass matrix data to the cube shader program matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	glBindVertexArray(0); //deactivate the vertex array object


	glBindTexture(GL_TEXTURE_2D, texture);


	glutPostRedisplay();
	glutSwapBuffers();  // flips the back buffer with the front buffer every frame similar to gl flush



}
	//creates the shader program
	void UCreateShader()
	{
		//vertex shader

		GLint vertexShader = glCreateShader(GL_VERTEX_SHADER); // creates the vertex shader
		glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); //attaches the vertex shader to the source code
		glCompileShader(vertexShader); // compiles the vertex shader



		//fragment shader
		GLint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); // creates the fragment shader
		glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); //attaches the frag shader to the source code
		glCompileShader(fragmentShader); // compiles the frag shader

		//shader program
		shaderProgram = glCreateProgram(); // creates the shader program and returns and id
		glAttachShader(shaderProgram, vertexShader); // attach vertex shader to shader program
		glAttachShader(shaderProgram, fragmentShader); // attach frag shader to shader program
		glLinkProgram(shaderProgram); // link vertex and frag shaders to shader program

		//delete vertex and frag shaders
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		// cube Vertex shader
		GLint cubeVertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(cubeVertexShader, 1, &cubeVertexShaderSource, NULL);
		glCompileShader(cubeVertexShader);



		// cube Fragment shader
		GLint cubeFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(cubeFragmentShader, 1, &cubeFragmentShaderSource, NULL);
		glCompileShader(cubeFragmentShader);



		//cube Shader program
		cubeShaderProgram = glCreateProgram();
		glAttachShader(cubeShaderProgram, cubeVertexShader);
		glAttachShader(cubeShaderProgram, cubeFragmentShader);;
		glLinkProgram(cubeShaderProgram);




		// Delete the vertex and fragment shaders once linked
		glDeleteShader(cubeVertexShader);
		glDeleteShader(cubeFragmentShader);


		// lamp Vertex shader
		GLint lampVertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(lampVertexShader, 1, &lampVertexShaderSource, NULL);
		glCompileShader(lampVertexShader);


		// lamp Fragment shader
		GLint lampFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(lampFragmentShader, 1, &lampFragmentShaderSource, NULL);
		glCompileShader(lampFragmentShader);
		//used for debugging shaders
			/*	GLenum ers = glGetError();
				GLint params;
				glGetShaderiv(lampVertexShader, GL_COMPILE_STATUS, &params);
				GLchar infoLog [100];
				GLsizei length;
				glGetShaderInfoLog(lampVertexShader, 100, &length, infoLog);*/

		//lamp Shader program
		lampShaderProgram = glCreateProgram();
		glAttachShader(lampShaderProgram, lampVertexShader);
		glAttachShader(lampShaderProgram, lampFragmentShader);
		glLinkProgram(lampShaderProgram);

		// Delete the vertex and fragment shaders once linked
		glDeleteShader(lampVertexShader);
		glDeleteShader(lampFragmentShader);
	}


void UCreateBuffers()
{
	//position and color data
	GLfloat vertices[] = {
			//vertex positions, textures and colors
//seat
			0.5f, 0.5f, 0.0f,  0.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f,// G 0
			0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f,// F 1
			-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f,// E 2
			-0.5f, 0.5f, 0.0f,    0.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f,// H 3

			0.5f, -0.5f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // D 4
			0.5f, 0.5f, -0.25f,   1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// C 5
			-0.5f, 0.5f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// B 6
			-0.5f, -0.5f, -0.25f,    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// A 7
//seat
//back
			0.5f, -0.5f, 0.75f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// J 12
			0.5f, -0.75f, 0.75f,   1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// K 13
			-0.5f, -0.75f, 0.75f,    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// L 14
			-0.5f, -0.5f, 0.75f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// I 15


			0.5f, -0.75f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// D 8
			0.5f, -0.5f, -0.25f,   1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// M 9
			-0.5f, -0.5f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// N 10
			-0.5f, -0.75f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// A 11
//back
//left back leg

			-0.25f, -0.5f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// O 16
			-0.25f, -0.75f, -0.25f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// P 17
			-0.5f, -0.75f, -0.25f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// A 18
			-0.5f, -0.5f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// N 19

			-0.25f, -0.75f, -0.75f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// Q 20
			-0.25f, -0.5f, -0.75f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// R 21
			-0.5f, -0.5f, -0.75f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// S 22
			-0.5f, -0.75f, -0.75f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// T 23
//left back leg
//right back leg
			0.5f, -0.5f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// O 16
			0.5f, -0.75f, -0.25f,   1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// P 17
			0.25f, -0.75f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// A 18
			0.25f, -0.5f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// N 19

			0.5f, -0.75f, -0.75f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// O 16
			0.5f, -0.5f, -0.75f,   1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// P 17
			0.25f, -0.5f, -0.75f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// A 18
			0.25f, -0.75f, -0.75f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// N 19
//right back leg
//front left leg

			-0.25f, 0.5f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// O 16
			-0.25f, 0.25f, -0.25f,   1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// P 17
			-0.5f, 0.25f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// A 18
			-0.5f, 0.5f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// N 19

			-0.25f, 0.25f, -0.75f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// Q 20
			-0.25f, 0.5f, -0.75f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // R 21
			-0.5f, 0.5f, -0.75f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// S 22
			-0.5f, 0.25f, -0.75f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// T 23
//front left leg
//front right leg
			0.5f, 0.5f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// O 16
			0.5f, 0.25f, -0.25f,   1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// P 17
			0.25f, 0.25f, -0.25f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// A 18
			0.25f, 0.5f, -0.25f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// N 19

			0.5f, 0.25f, -0.75f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// O 16
			0.5f, 0.5f, -0.75f,   1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// P 17
			0.25f, 0.5f, -0.75f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// A 18
			0.25f, 0.25f, -0.75f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// N 19
//front right leg
	};
	// index data to share position data
	GLuint indices[] = {
			0, 1, 3,
			1, 2, 3,
			0, 1, 4,
			0, 4, 5,
			0, 5, 6,
			0, 3, 6,
			4, 5, 6,
			4, 6, 7,
			2, 3, 6,
			2, 6, 7,
			1, 4, 7,
			1, 2, 7,

			8, 9, 11,
			9, 10, 11,
			8, 9, 12,
			8, 12, 13,
			8, 13, 14,
			8, 11, 14,
			12, 13, 14,
			12, 14, 15,
			10, 11, 14,
			10, 14, 15,
			9, 12, 15,
			9, 10, 15,

			16, 17, 19,
			17, 18, 19,
			16, 17, 20,
			16, 20, 21,
			16, 21, 22,
			16, 19, 22,
			20, 21, 22,
			20, 22, 23,
			18, 19, 22,
			18, 22, 23,
			17, 20, 23,
			17, 18, 23,

			24, 25, 27,
			25, 26, 27,
			24, 25, 28,
			24, 28, 29,
			24, 29, 30,
			24, 27, 30,
			28, 29, 30,
			28, 30, 31,
			26, 27, 30,
			26, 30, 31,
			25, 28, 31,
			25, 26, 31,

			32, 33, 35,
			33, 34, 35,
			32, 33, 36,
			32, 36, 37,
			32, 37, 38,
			32, 35, 38,
			36, 37, 38,
			36, 38, 39,
			34, 35, 38,
			34, 38, 39,
			33, 36, 39,
			33, 34, 39,

			40, 41, 43,
			41, 42, 43,
			40, 41, 44,
			40, 44, 45,
			40, 45, 46,
			40, 43, 46,
			44, 45, 46,
			44, 46, 47,
			42, 43, 46,
			42, 46, 47,
			41, 44, 47,
			41, 42, 47,
	};

	// Generate buffer ids
	// Activate the vertex array object before binding and settomg any VBOs or vertex attribute pointers

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);


	//activate the vertex array object before binding and setting any VBOs and vertex attrib pointers
	glBindVertexArray(VAO);

	//activate the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER,sizeof(vertices), vertices, GL_STATIC_DRAW);// copy vertices to VBO

	//activate the element buffer object
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices), indices, GL_STATIC_DRAW);// copy vertices to EBO


	//set attrib pointer to 0 to hold pos data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9* sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);  // Enables the vertex attrib

	// Set attribute pointer 1 to hold Color data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(10 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//set attrib pointer to 1 to hold color data
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9* sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);  // Enables the vertex attrib



	glBindVertexArray(0); // Deactivates the VAO



	// Generate buffer ids for lamp
	glGenVertexArrays(1, &LightVAO);

	// Activate the vertex array object before binding and setting any VBOs or vertex attribute pointers
	glBindVertexArray(LightVAO);


	// set attribute pointer 0 to hold position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);



}
void UGenerateTexture(){

			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);

			int width, height;

			unsigned char* image = SOIL_load_image("snhu.jpg", &width, &height, 0, SOIL_LOAD_RGB); //loads texture

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
			glGenerateMipmap(GL_TEXTURE_2D);
			SOIL_free_image_data(image);
			glBindTexture(GL_TEXTURE_2D, 0); //unbind the texture

}














