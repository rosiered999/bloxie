#include <bits/stdc++.h>
#include <unistd.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
    glm::mat4 projectionO, projectionP;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID;
} Matrices;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open())
	{
	    std::string Line = "";
	    while(getline(VertexShaderStream, Line))
		VertexShaderCode += "\n" + Line;
	    VertexShaderStream.close();
	}

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
	std::string Line = "";
	while(getline(FragmentShaderStream, Line))
	    FragmentShaderCode += "\n" + Line;
	FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> VertexShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);

    // Compile Fragment Shader
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);

    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

void initGLEW(void){
    glewExperimental = GL_TRUE;
    if(glewInit()!=GLEW_OK){
	fprintf(stderr,"Glew failed to initialize : %s\n", glewGetErrorString(glewInit()));
    }
    if(!GLEW_VERSION_3_3)
	fprintf(stderr, "3.3 version not available\n");
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat[3*numVertices];
    for (int i=0; i<numVertices; i++)
    {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject (primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

bool keystates_pressed[350];
float ex = 0,ey = -6, ez = 10;
int tx = 0,ty = 0,tz = 0;
int blockState = 1;
float blockRotation = 0.0f;
float screenRotation = 0.0f;
float blackScreenYTranslation;
bool gameover;
bool makeBridge;
bool breakFragileTiles = false;
bool madeMove = false;
bool gamewon = false;
bool printed;
int currentView = 0;
int level = 1;
int moves;
float score = 0;
int minMoves = 26;
int radius = 6;
float angle = 0.0f;
bool showBlackScreen;
bool mouse_keystates_pressed[8];
bool mouse_keystates_released[8];
int views[11] = {'0','1','2','a','d','w','s','i','j','k','l'};
int glfwViewKeys[11] = {48,49,50,65,68,87,83,73,74,75,76};
glm::vec3 block_pos, board_block_pos, axis (0,0,1);
double last_update_time, current_time;

struct Position{
	int x;
	int y;
};
Position on_board;

int board[12][12] = {
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,1,1,1,1,1,2,2,1,0,0},
	{0,0,1,9,1,1,0,2,2,1,0,0},
	{0,0,1,1,1,1,0,2,2,2,2,0},
	{0,0,0,0,0,0,0,4,0,2,2,0},
	{0,0,8,0,0,1,1,4,0,0,1,0},
	{0,0,1,0,0,1,1,0,0,0,1,0},
	{0,1,1,0,0,1,1,0,0,0,1,0},
	{0,1,1,1,3,1,0,0,1,1,1,0},
	{0,1,1,1,1,1,1,1,1,1,0,0},
	{0,0,5,1,1,0,0,0,1,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0}
};

int board2[12][12]={
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,1,1,1,0,0,0,0,1,1,1,0},
	{0,1,2,1,0,0,0,2,2,1,1,0},
	{0,1,2,1,0,0,0,2,2,1,1,0},
	{0,1,9,1,2,2,1,2,1,1,1,0},
	{0,1,1,1,0,0,0,0,1,1,1,0},
	{0,0,0,0,0,0,0,0,4,0,0,0},
	{0,0,0,0,0,0,0,0,4,0,0,0},
	{0,8,1,3,1,4,4,1,1,1,0,0},
	{0,1,1,1,5,0,0,0,1,1,0,0},
	{0,1,1,1,1,0,0,0,1,1,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0}
};

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.
    if (action == GLFW_PRESS)
        keystates_pressed[key] = true;
}

void setViewValue()
{
	for (int i=0; i<11; i++)
	{
		if (views[i] != currentView)
			keystates_pressed[glfwViewKeys[i]] = false;
	}
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
    switch (key)
    {
	    case 'Q':
	    case 'q':
			quit(window);
			break;
		case '1':
			ex = 0;
			ey = 0;
			ez = 10;
			tx = 0;
			ty = 0;
			tz = 0;
			currentView = key;
			break;
		case '0':
			ex = 0;
			ey = -6;
			ez = 10;
			tx = 0;
			ty = 0;
			tz = 0;
			currentView = key;
			break;
		case '2':
			tx = 0;
			ty = 0;
			tz = 0;
			ez = 10;
			currentView = key;
			break;
		case 'f':
			if (currentView == '2')
				angle += 0.05;
			break;
		case 'h':
			if (currentView == '2')
				angle -= 0.05;
			break;
		case 'd':
		case 'a':
		case 'w':
		case 's':
		case 'k':
		case 'l':
		case 'j':
		case 'i':
			currentView = key;
			break;
	    default:
			break;
    }
    setViewValue();
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

    GLfloat fov = M_PI/2;

    // sets the viewport of openGL renderer
    glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

    // Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    Matrices.projectionP = glm::perspective(fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.05f, 25.05f);
}

VAO *cubeY, *cubeO, *cubeB, *cubeBridge, *cubeK, *cubeW, *block, *line, *blackScreen;

GLfloat vertex_buffer_data [] = {
-0.5, 0.5, 0.5,
-0.5, -0.5, 0.5,
0.5, -0.5, 0.5,
-0.5, 0.5, 0.5,
0.5, -0.5, 0.5,
0.5, 0.5, 0.5,
0.5, 0.5, 0.5,
0.5, -0.5, 0.5,
0.5, -0.5, -0.5,
0.5, 0.5, 0.5,
0.5, -0.5, -0.5,
0.5, 0.5, -0.5,
0.5, 0.5, -0.5,
0.5, -0.5, -0.5,
-0.5, -0.5, -0.5,
0.5, 0.5, -0.5,
-0.5, -0.5, -0.5,
-0.5, 0.5, -0.5,
-0.5, 0.5, -0.5,
-0.5, -0.5, -0.5,
-0.5, -0.5, 0.5,
-0.5, 0.5, -0.5,
-0.5, -0.5, 0.5,
-0.5, 0.5, 0.5,
-0.5, 0.5, -0.5,
-0.5, 0.5, 0.5,
0.5, 0.5, 0.5,
-0.5, 0.5, -0.5,
0.5, 0.5, 0.5,
0.5, 0.5, -0.5,
-0.5, -0.5, 0.5,
-0.5, -0.5, -0.5,
0.5, -0.5, -0.5,
-0.5, -0.5, 0.5,
0.5, -0.5, -0.5,
0.5, -0.5, 0.5
};

GLfloat color_buffer_data [] = {
1.0f, 1.0f, 0.0f,
1.0f, 1.0f, 0.0f,
1.0f, 1.0f, 0.0f,
1.0f, 1.0f, 0.0f,
1.0f, 1.0f, 0.0f,
1.0f, 1.0f, 0.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 1.0f, 1.0f,
0.0f, 1.0f, 1.0f,
0.0f, 1.0f, 1.0f,
0.0f, 1.0f, 1.0f,
0.0f, 1.0f, 1.0f,
0.0f, 1.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f,
0.0f, 0.0f, 1.0f
};

GLfloat line_buffer_data [] = {
0.0f, 0.0f, 0.26f,
1.0f, 0.0f, 0.26f,
1.0f, 0.0f, 0.26f,
1.0f, 1.0f, 0.26f,
0.0f, 0.0f, 0.26f,
0.0f, 1.0f, 0.26f,
0.0f, 1.0f, 0.26f,
1.0f, 1.0f, 0.26f
};

GLfloat line_color_data [] = {
0.0f, 0.0f, 0.0f,
0.0f, 0.0f, 0.0f,
0.0f, 0.0f, 0.0f,
0.0f, 0.0f, 0.0f,
0.0f, 0.0f, 0.0f,
0.0f, 0.0f, 0.0f,
0.0f, 0.0f, 0.0f,
0.0f, 0.0f, 0.0f
};

void createBlackScreen ()
{
	GLfloat black_box_buffer_data [] = {
	-20.0f, -20.0f, 3.0f,
	-20.0f, 20.0f, 3.0f,
	20.0f, -20.0f, 3.0f,
	-20.0f, 20.0f, 3.0f,
	20.0f, -20.0f, 3.0f,
	20.0f, 20.0f, 3.0f
	};

	blackScreen = create3DObject(GL_TRIANGLES, 6, black_box_buffer_data, line_color_data, GL_FILL);
}

// Creates the cube object used in this sample code
void createBoard ()
{
    // GL3 accepts only Triangles. Quads are not supported
    for (int i=0; i<36; i++)
    {
    	vertex_buffer_data[i*3] += 0.5;
    	vertex_buffer_data[i*3+1] += 0.5;
    	vertex_buffer_data[i*3+2] /= 2;
    }
    // create3DObject creates and returns a handle to a VAO that can be used later
    cubeY = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);

    for (int i=0; i<6; i++)
    {
    	color_buffer_data[i*3] = 1.0;
    	color_buffer_data[i*3+1] = 0.45;
    	color_buffer_data[i*3+2] = 0.0;
    }
    cubeO = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);

    for (int i=0; i<6; i++)
    {
    	color_buffer_data[i*3] = 0.5;
    	color_buffer_data[i*3+1] = 0.25;
    	color_buffer_data[i*3+2] = 0.0;
    }
    cubeB = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);

    for (int i=0; i<6; i++)
    {
    	color_buffer_data[i*3] = 1.0;
    	color_buffer_data[i*3+1] = 0.80;
    	color_buffer_data[i*3+2] = 0.65;
    }
    cubeBridge = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);

    for (int i=0; i<6; i++)
    {
    	color_buffer_data[i*3] = 0.65;
    	color_buffer_data[i*3+1] = 0.60;
    	color_buffer_data[i*3+2] = 0.50;
    }
    cubeK = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);

    for (int i=0; i<6; i++)
    {
    	color_buffer_data[i*3] = 0.0f;
    	color_buffer_data[i*3+1] = 0.5f;
    	color_buffer_data[i*3+2] = 0.0f;
    }
    cubeW = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);

    line = create3DObject(GL_LINES, 8, line_buffer_data, line_color_data, GL_LINE);
}

// Creates the cube object used in this sample code
void createBlock ()
{
    // GL3 accepts only Triangles. Quads are not supported
    for (int i=0; i<36; i++)
    {
    	vertex_buffer_data[i*3+2] *= 4;

    	color_buffer_data[i*3] = 0.2;
    	color_buffer_data[i*3+1] = 0.2;
    	color_buffer_data[i*3+2] = 0.2;
    }

    // create3DObject creates and returns a handle to a VAO that can be used later
    block = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void moveBlock()
{
	if (keystates_pressed[GLFW_KEY_DOWN] || keystates_pressed[GLFW_KEY_UP] || keystates_pressed[GLFW_KEY_LEFT] || keystates_pressed[GLFW_KEY_RIGHT])
		moves ++;

	if (blockState == 1 && keystates_pressed[GLFW_KEY_DOWN])
	{
		blockRotation = M_PI/2;
		axis.x = 1;
		axis.y = 0;
		axis.z = 0;
		block_pos.y -= 1;
		block_pos.z = 0.25;
		blockState = 2;
		keystates_pressed[GLFW_KEY_DOWN] = false;
		madeMove = false;
		on_board.x += 15;
	}
	else if (blockState == 2 && keystates_pressed[GLFW_KEY_DOWN])
	{
		blockRotation = 0;
		axis.x = 1;
		axis.y = 0;
		axis.z = 0;
		block_pos.y -= 2;
		block_pos.z = 1.25;
		blockState = 1;
		keystates_pressed[GLFW_KEY_DOWN] = false;
		madeMove = false;
		on_board.x += 15;
	}
	else if (blockState == 1 && keystates_pressed[GLFW_KEY_UP])
	{
		blockRotation -= M_PI/2;
		axis.x = -1;
		axis.y = 0;
		axis.z = 0;
		block_pos.y += 2;
		block_pos.z = 0.25;
		blockState = 2;
		keystates_pressed[GLFW_KEY_UP] = false;
		madeMove = false;
		on_board.x -= 15;
	}
	else if (blockState == 2 && keystates_pressed[GLFW_KEY_UP])
	{
		blockRotation = 0;
		axis.x = -1;
		axis.y = 0;
		axis.z = 0;
		block_pos.y += 1;
		block_pos.z = 1.25;
		blockState = 1;
		keystates_pressed[GLFW_KEY_UP] = false;
		madeMove = false;
		on_board.x -= 15;
	}
	else if (blockState == 1 && keystates_pressed[GLFW_KEY_LEFT])
	{
		blockRotation = M_PI/2;
		axis.x = 0;
		axis.y = 1;
		axis.z = 0;
		block_pos.x -= 1;
		block_pos.z = 1.25;
		blockState = 3;
		keystates_pressed[GLFW_KEY_LEFT] = false;
		madeMove = false;
		on_board.y -= 15;
	}
	else if (blockState == 3 && keystates_pressed[GLFW_KEY_RIGHT])
	{
		blockRotation = 0;
		axis.x = 1;
		axis.y = 0;
		axis.z = 0;
		block_pos.x += 1;
		block_pos.z = 1.25;
		blockState = 1;
		keystates_pressed[GLFW_KEY_RIGHT] = false;
		madeMove = false;
		on_board.y += 15;
	}
	else if (blockState == 1 && keystates_pressed[GLFW_KEY_RIGHT])
	{
		blockRotation -= M_PI/2;
		axis.x = 0;
		axis.y = -1;
		axis.z = 0;
		block_pos.x += 2;
		block_pos.z = 1.25;
		blockState = 3;
		keystates_pressed[GLFW_KEY_RIGHT] = false;
		madeMove = false;
		on_board.y += 15;
	}
	else if (blockState == 3 && keystates_pressed[GLFW_KEY_LEFT])
	{
		blockRotation = 0;
		axis.x = -1;
		axis.y = 0;
		axis.z = 0;
		block_pos.x -= 2;
		block_pos.z = 1.25;
		blockState = 1;
		keystates_pressed[GLFW_KEY_LEFT] = false;
		madeMove = false;
		on_board.y -= 15;
	}
	else if (blockState == 2 && keystates_pressed[GLFW_KEY_RIGHT])
	{
		blockRotation = M_PI/2;
		axis.x = 1;
		axis.y = 0;
		axis.z = 0;
		block_pos.x += 1;
		block_pos.z = 0.25;
		blockState = 2;
		keystates_pressed[GLFW_KEY_RIGHT] = false;
		madeMove = false;
		on_board.y += 10;
	}
	else if (blockState == 2 && keystates_pressed[GLFW_KEY_LEFT])
	{
		blockRotation = M_PI/2;
		axis.x = 1;
		axis.y = 0;
		axis.z = 0;
		block_pos.x -= 1;
		block_pos.z = 0.25;
		blockState = 2;
		keystates_pressed[GLFW_KEY_LEFT] = false;
		madeMove = false;
		on_board.y -= 10;
	}
	else if (blockState == 3 && keystates_pressed[GLFW_KEY_UP])
	{
		blockRotation = M_PI/2;
		axis.x = 0;
		axis.y = 1;
		axis.z = 0;
		block_pos.y += 1;
		block_pos.z = 1.25;
		blockState = 3;
		keystates_pressed[GLFW_KEY_UP] = false;
		madeMove = false;
		on_board.x -= 10;
	}
	else if (blockState == 3 && keystates_pressed[GLFW_KEY_DOWN])
	{
		blockRotation = M_PI/2;
		axis.x = 0;
		axis.y = 1;
		axis.z = 0;
		block_pos.y -= 1;
		block_pos.z = 1.25;
		blockState = 3;
		keystates_pressed[GLFW_KEY_DOWN] = false;
		madeMove = false;
		on_board.x += 10;
	}
}

void setViewDirection()
{
    if (keystates_pressed[GLFW_KEY_D])
    {
		tx = 5;
		ty = 0;
		tz = 0;
		ex = block_pos.x;
		ey = block_pos.y - 2;
		ez = 6;
    }
    else if (keystates_pressed[GLFW_KEY_W])
    {
		tx = 0;
		ty = 6;
		tz = 0;
		ex = block_pos.x - 2;
		ey = block_pos.y;
		ez = 6;
	}
    else if (keystates_pressed[GLFW_KEY_A])
    {
		tx = -6;
		ty = 0;
		tz = 0;
		ex = block_pos.x + 2;
		ey = block_pos.y;
		ez = 6;
	}
    else if (keystates_pressed[GLFW_KEY_S])
    {
		tx = 0;
		ty = -5;
		tz = 0;
		ex = block_pos.x;
		ey = block_pos.y + 2;
		ez = 6;
	}
    else if (keystates_pressed[GLFW_KEY_L])
    {
		tx = 5;
		ty = 0;
		tz = -2;
		ex = block_pos.x + 1;
		ey = block_pos.y;
		ez = 2.5;
    }
    else if (keystates_pressed[GLFW_KEY_I])
    {
		tx = 0;
		ty = 6;
		tz = -2;
		ex = block_pos.x;
		ey = block_pos.y + 1;
		ez = 2.5;
	}
    else if (keystates_pressed[GLFW_KEY_J])
    {
		tx = -6;
		ty = 0;
		tz = -2;
		ex = block_pos.x - 1;
		ey = block_pos.y;
		ez = 2.5;
	}
    else if (keystates_pressed[GLFW_KEY_K])
    {
		tx = 0;
		ty = -5;
		tz = -2;
		ex = block_pos.x;
		ey = block_pos.y - 1;
		ez = 2.5;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (currentView == '2')
	{
		if (yoffset == -1)
			ez -= 0.1;
		else
			ez += 0.1;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		mouse_keystates_pressed[button] = true;
		mouse_keystates_released[button] = false;
	}
	else
	{
		mouse_keystates_pressed[button] = false;
		mouse_keystates_released[button] = true;
	}
}

void initialiseBlock()
{
	makeBridge = false;
	last_update_time = glfwGetTime();
	gameover = false;
	printed = false;
	blackScreenYTranslation = 0.0f;

  	for (int i=0; i<12; i++)
  	{
  		for (int j=0; j<12 ; j++)
  		{
		    if (board[i][j] == 8)
			    block_pos = glm::vec3(-6+j, 6-i, 1.25);
  		}
  	}
  	for (int i = 0; i < 350; i++)
  		keystates_pressed[i] = false;
}

void checkFragileTiles()
{
    if (on_board.x % 10 == 0 && on_board.y % 10 == 0 && board[on_board.x/10][on_board.y/10] == 2)
    {
		block_pos.z -= 0.5;
		if (block_pos.z < -10)
			gameover = true;
		breakFragileTiles = true;
    }
    if (!makeBridge)
    {
	    if (on_board.x % 10 == 0 && on_board.y % 10 == 0 && board[on_board.x/10][on_board.y/10] == 4)
			gameover = true;
	    else if (on_board.x % 10 == 5 && (board[(on_board.x-5)/10][on_board.y/10] == 4 || board[(on_board.x+5)/10][on_board.y/10] == 4))
			gameover = true;
	    else if (on_board.y % 10 == 5 && (board[on_board.x/10][(on_board.y-5)/10] == 4 || board[on_board.x/10][(on_board.y+5)/10] == 4))
			gameover = true;
    }
}

void checkBridges()
{
    if (on_board.x % 10 == 0 && on_board.y % 10 == 0 && board[on_board.x/10][on_board.y/10] == 3)
    	makeBridge = true;
    else if (on_board.x % 10 == 5 && (board[(on_board.x-5)/10][on_board.y/10] == 3 || board[(on_board.x+5)/10][on_board.y/10] == 3))
    	makeBridge = true;
    else if (on_board.y % 10 == 5 && (board[on_board.x/10][(on_board.y-5)/10] == 3 || board[on_board.x/10][(on_board.y+5)/10] == 3))
    	makeBridge = true;

    if (!madeMove)
    {
	    if (on_board.x % 10 == 0 && on_board.y % 10 == 0 && board[on_board.x/10][on_board.y/10] == 5){
	    	makeBridge = !makeBridge;
	    	madeMove = true;
	    }
	    else if (on_board.x % 10 == 5 && (board[(on_board.x-5)/10][on_board.y/10] == 5 || board[(on_board.x+5)/10][on_board.y/10] == 5)){
	    	makeBridge = !makeBridge;
	    	madeMove = true;
	    }
	    else if (on_board.y % 10 == 5 && (board[on_board.x/10][(on_board.y-5)/10] == 5 || board[on_board.x/10][(on_board.y+5)/10] == 5)){
	    	makeBridge = !makeBridge;
	    	madeMove = true;
	    }
    }
}

void printGameState()
{
	if (level > 1 && !printed)
	{
		cout<<"GREAT JOB! YOU MADE IT TO THE FINAL LEVEL"<<endl;
		cout<<"===============LEVEL "<<1<<" STATS============="<<endl;
		cout<<"YOU MADE "<<moves<<" MOVES"<<endl;
		cout<<"YOUR SCORE IS "<<score<<" ON 100"<<endl;
		cout<<"========================================="<<endl;
		printed = true;
		moves = 0;
	}
	else if (gameover && !gamewon)
	{
		cout<<"GAMEOVER"<<endl;
		cout<<"===============LEVEL "<<level<<" STATS============="<<endl;
		cout<<"YOU MADE "<<moves<<" MOVES"<<endl;
		cout<<"YOUR SCORE IS "<<score<<" ON "<<level*100<<endl;
		cout<<"========================================="<<endl;
	}
	else if (gamewon)
	{
		gameover = true;
		cout<<"GREAT JOB! YOU WON THE GAME"<<endl;
		cout<<"===============LEVEL "<<level<<" STATS============="<<endl;
		cout<<"YOU MADE "<<moves<<" MOVES"<<endl;
		cout<<"YOUR SCORE IS "<<score<<" ON "<<level*100<<endl;
		cout<<"========================================="<<endl;
	}
}

void gameState()
{
	if (on_board.y <= 0 || on_board.y >= 110 || on_board.x <= 0 || on_board.x >= 110)
		gameover = true;

    if (on_board.x % 10 == 0 && on_board.y % 10 == 0 && board[on_board.x/10][on_board.y/10] == 0)
		gameover = true;
    else if (on_board.x % 10 == 5 && (board[(on_board.x-5)/10][on_board.y/10] == 0 || board[(on_board.x+5)/10][on_board.y/10] == 0))
		gameover = true;
    else if (on_board.y % 10 == 5 && (board[on_board.x/10][(on_board.y-5)/10] == 0 || board[on_board.x/10][(on_board.y+5)/10] == 0))
		gameover = true;

	if (on_board.x % 10 == 0 && on_board.y % 10 == 0 && board[on_board.x/10][on_board.y/10] == 9)
	{
		if (level == 1)
		{
			score += (1.0*minMoves/moves)*100.0;
			for (int i = 0; i < 12; i++)
			{
				for (int j = 0; j < 12; j++)
					board[i][j] = board2[i][j];
			}
			on_board.x = 80;
			on_board.y = 10;
			initialiseBlock();
			minMoves = 30;
			level++;
		}
		else
		{
			gamewon = true;
			score += (1.0*minMoves/moves)*100.0;
		}
	}
	printGameState ();
}

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw (GLFWwindow* window, float x, float y, float w, float h)
{
    int fbwidth, fbheight;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);
    glViewport((int)(x*fbwidth), (int)(y*fbheight), (int)(w*fbwidth), (int)(h*fbheight));

    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram(programID);

    setViewDirection();

    // Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye (ex,ey,ez);

	if (showBlackScreen)
	{
		eye.x = 0;
		eye.y = 0;
		eye.z = 10;
	}

	if (currentView == '2')
	{
        if (mouse_keystates_pressed[GLFW_MOUSE_BUTTON_LEFT] && !mouse_keystates_released[GLFW_MOUSE_BUTTON_LEFT])
        	angle += 0.05;
        else if (mouse_keystates_pressed[GLFW_MOUSE_BUTTON_RIGHT] && !mouse_keystates_released[GLFW_MOUSE_BUTTON_RIGHT])
        	angle -= 0.05;

		eye.x = radius*cos(angle);
		eye.y = radius*sin(angle);
	}

	// Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (tx,ty,tz);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0,1,0);

    if (currentView == '2')
    {
    	up.x = -1*cos(angle);
    	up.y = -1*sin(angle);
    	up.z = 0;
    }

    // Compute Camera matrix (view)
    // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
    //  Don't change unless you are sure!!
    Matrices.view = glm::lookAt(eye, target, up); // Fixed camera for 2D (ortho) in XY plane

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    glm::mat4 VP = Matrices.projectionP * Matrices.view;

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    glm::mat4 MVP;	// MVP = Projection * View * Model

    /* Render your scene */
    //  Don't change unless you are sure!!
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();

    moveBlock();

/*    checkFragileTiles();

    board_block_pos.y = 6;
    board_block_pos.z = 0;*/
/*
    for (int i=0; i<12; i++)
    {
	    board_block_pos.x = -6;
    	for (int j=0; j<12; j++)
    	{
		    if (board[i][j] > 0)
		    {
			    Matrices.model = glm::mat4(1.0f);
			    glm::mat4 translateCube = glm::translate (board_block_pos);        // glTranslatef
			    Matrices.model *= translateCube;
			    MVP = VP * Matrices.model;
			    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

			    // draw3DObject draws the VAO given to it using current MVP matrix
			    if (board[i][j] == 1 || board[i][j] == 8)
				    draw3DObject(cubeY);
			    else if (board[i][j] == 2 && !breakFragileTiles)
				    draw3DObject(cubeO);
			    else if (board[i][j] == 3)
				    draw3DObject(cubeB);
			    else if (board[i][j] == 4 && makeBridge)
				    draw3DObject(cubeBridge);
			    else if (board[i][j] == 5)
				    draw3DObject(cubeK);
			    else if (board[i][j] == 9)
				    draw3DObject(cubeW);

				if ((board[i][j] == 2 && breakFragileTiles) || gameover || (board[i][j] == 4 && !makeBridge))
					;
				else
				    draw3DObject(line);
		    }
		    board_block_pos.x += 1;
    	}
	    board_block_pos.y -= 1;
    }
*/
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateBlock = glm::translate (block_pos);        // glTranslatef
	glm::mat4 translateBlock_to_origin = glm::translate (glm::vec3(-1.0*block_pos.x,-1.0*block_pos.y,-1.0*block_pos.z));
	glm::mat4 rotateBlock = glm::rotate(blockRotation, axis);  // rotate about vector (1,0,0)
	glm::mat4 translateBlock_back = glm::translate (glm::vec3(block_pos.x, block_pos.y, block_pos.z));
	Matrices.model *= translateBlock_back*rotateBlock*translateBlock_to_origin*translateBlock;

    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(block);

/*checkBridges();

    if (showBlackScreen)
    {
	    Matrices.model = glm::mat4(1.0f);
	    glm::mat4 translateBlackScreen = glm::translate (glm::vec3(0, blackScreenYTranslation, 0));   // glTranslatef
		Matrices.model *= translateBlackScreen;
        MVP = VP * Matrices.model;
	    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(blackScreen);
		blackScreenYTranslation -= 0.25
  }*/
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height){
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
	exit(EXIT_FAILURE);
        glfwTerminate();
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval( 1 );
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);
    glfwSetWindowCloseCallback(window, quit);
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window, scroll_callback);

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
    // Create the models
    createBoard ();
    createBlock ();
    createBlackScreen ();

    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
    // Get a handle for our "MVP" uniform
    Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

    reshapeWindow (window, width, height);

    // Background color of the scene
    glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
    glClearDepth (1.0f);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);
}

int main (int argc, char** argv)
{
    int width = 600;
    int height = 600;
/*  	board_block_pos = glm::vec3(0, 0, 0);
  	on_board.x = 50;
  	on_board.y = 20;*/

    GLFWwindow* window = initGLFW(width, height);
    initGLEW();
    initGL (window, width, height);

    initialiseBlock();

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

		// clear the color and depth in the frame buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	    // OpenGL Draw commands
		draw(window, 0, 0, 1, 1);
        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

    //    gameState();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
/*        if (current_time - last_update_time <= 1) // at most 1s elapsed since last frame
        	showBlackScreen = true;
        else
        	showBlackScreen = false;

        if (gameover)
        {
        	usleep(200000);
			quit(window);
        }
    }*/

    glfwTerminate();
    //    exit(EXIT_SUCCESS);
}
