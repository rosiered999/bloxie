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
int arrow_key = 0;
glm::mat4 MVP,VP;
int blockState = 1;
float blockRotation = 0.0f;
int level = 0;
int moves;
int view = 0;
int jump = 0;
glm::vec3 block_pos(2,1,2), axis (0,0,1);
glm::vec3 camera_pos(8,10,10), target_pos(0,0,0);
double last_update_time, current_time;

int initPos[3][2] = {
    {2,2},
    {4,1},
    {4,0},
};
int bridgeCheck,bridge_toggle=0;
int eyeX, eyeY, eyeZ;
int targetX, targetY, targetZ;

int map1[3][11][15] = {
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,1,1,1,0,0,0,0,0,0,0,0,0,0,0},
        {0,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
        {0,1,1,1,1,1,1,1,0,0,0,0,0,0,0},
        {0,0,1,1,1,1,1,1,1,0,0,0,0,0,0},
        {0,0,0,0,1,1,2,1,1,0,0,0,0,0,0},
        {0,0,0,0,0,1,3,1,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,1,1,1,1,3,3,0,0},
        {1,1,1,1,0,0,1,1,1,0,0,3,3,0,0},
        {1,1,1,1,4,4,1,1,1,1,1,3,3,1,1},
        {1,5,1,1,4,4,1,1,1,1,1,3,3,2,1},
        {1,1,1,1,0,0,0,0,0,0,0,1,1,1,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0},
        {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0},
        {0,0,0,0,0,1,1,1,1,1,1,1,1,0,0},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {0,0,0,0,1,1,1,1,1,1,1,1,1,2,1},
        {0,0,0,0,1,1,1,0,0,0,0,0,1,1,1},
        {0,0,0,0,0,0,1,0,0,1,1,0,0,0,0},
        {0,0,0,0,0,0,1,1,1,1,1,0,0,0,0},
        {0,0,0,0,0,0,1,1,1,1,1,0,0,0,0},
        {0,0,0,0,0,0,0,1,1,1,0,0,0,0,0},
    },
};
int heliViewFlag = 0;

int checkBridges()
{
  int boardX1 = block_pos.x;
  int boardY1 = block_pos.z;
  if(map1[level][boardX1][boardY1]==5 && blockState==1)
  {
    bridge_toggle=1;
  }
  return bridge_toggle;
}
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
  bridgeCheck = checkBridges();
     // Function is called first on GLFW_PRESS.
    if (action == GLFW_PRESS)
        switch (key) {
          case GLFW_KEY_LEFT:
              arrow_key = 1;
              moves++;
              system("mpg123 -n 30 -q tick.mp3 &");
              break;
          case GLFW_KEY_RIGHT:
              arrow_key = 2;
              moves++;
              system("mpg123 -n 30 -q tick.mp3 &");
              break;
          case GLFW_KEY_DOWN:
          arrow_key = 4;
              moves++;
              system("mpg123 -n 30 -q tick.mp3 &");
              break;
          case GLFW_KEY_UP:
          arrow_key = 3;
              moves++;
              system("mpg123 -n 30 -q tick.mp3 &");
              break;
          case GLFW_KEY_ESCAPE:
              exit(1);
              break;
          case GLFW_KEY_0:
              view = 0;
              break;
          case GLFW_KEY_1:
              view = 1;
              break;
          case GLFW_KEY_2:
              view = 2;
              break;
          case GLFW_KEY_3:
              view = 3;
              break;
          case GLFW_KEY_4:
              view = 4;
              break;
          case GLFW_KEY_5:
              view = 5;
              break;
          case GLFW_KEY_6:
              view = 6;
              break;
          case GLFW_KEY_J:
              if(level>=1)
              {
                jump ^= 1;
              }
              else jump = 0;
              break;
          case GLFW_KEY_O:
            if(jump==1 && blockState==1)
            {
              block_pos.x -= 2;
            }
            break;
          case GLFW_KEY_P:
          if(jump==1 && blockState==1)
            {
              block_pos.x += 2;
            }
            break;
          case GLFW_KEY_K:
            if(jump==1 && blockState==1)
            {
              block_pos.z -= 2;
            }
          break;
          case GLFW_KEY_L:
            if(jump==1 && blockState==1)
            {
              block_pos.z += 2;
            }
          break;
          default:
              break;
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
      case 'W':
      case 'w':
      if(heliViewFlag==1)
      camera_pos.y++;
      break;
      case 'S':
      case 's':
      if(heliViewFlag==1)
      camera_pos.y--;
      break;
      case 'A':
      case 'a':
      if(heliViewFlag==1)
      camera_pos.x--;
      break;
      case 'D':
      case 'd':
      if(heliViewFlag==1)
      camera_pos.x++;
      break;
      case 'X':
      case 'x':
      if(heliViewFlag==1)
      camera_pos.z--;
      break;
      case 'Z':
      case 'z':
      if(heliViewFlag==1)
      camera_pos.z++;
      break;
      case 'T':
      case 't':
      if(heliViewFlag==1)
      target_pos.y++;
      break;
      case 'G':
      case 'g':
      if(heliViewFlag==1)
      target_pos.y--;
      break;
      case 'F':
      case 'f':
      if(heliViewFlag==1)
      target_pos.x--;
      break;
      case 'H':
      case 'h':
      if(heliViewFlag==1)
      target_pos.x++;
      break;
      case 'C':
      case 'c':
      if(heliViewFlag==1)
      target_pos.z--;
      break;
      case 'V':
      case 'v':
      if(heliViewFlag==1)
      target_pos.z++;
      break;
	    default:
			break;
    }
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

VAO *block, *brick, *orange_brick, *bridge_brick;

// Creates the cube object used in this sample code
void createBlock ()
{
  GLfloat vertex_buffer_data [] = {
    -0.5, 1, 0.5,
    -0.5, -1, 0.5,
    0.5, -1, 0.5,
    -0.5, 1, 0.5,
    0.5, -1, 0.5,
    0.5, 1, 0.5,
    0.5, 1, 0.5,
    0.5, -1, 0.5,
    0.5, -1, -0.5,
    0.5, 1, 0.5,
    0.5, -1, -0.5,
    0.5, 1, -0.5,
    0.5, 1, -0.5,
    0.5, -1, -0.5,
    -0.5, -1, -0.5,
    0.5, 1, -0.5,
    -0.5, -1, -0.5,
    -0.5, 1, -0.5,
    -0.5, 1, -0.5,
    -0.5, -1, -0.5,
    -0.5, -1, 0.5,
    -0.5, 1, -0.5,
    -0.5, -1, 0.5,
    -0.5, 1, 0.5,
    -0.5, 1, -0.5,
    -0.5, 1, 0.5,
    0.5, 1, 0.5,
    -0.5, 1, -0.5,
    0.5, 1, 0.5,
    0.5, 1, -0.5,
    -0.5, -1, 0.5,
    -0.5, -1, -0.5,
    0.5, -1, -0.5,
    -0.5, -1, 0.5,
    0.5, -1, -0.5,
    0.5, -1, 0.5,
  };

  GLfloat color_buffer_data [] = {
    1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,

  };
    // create3DObject creates and returns a handle to a VAO that can be used later
    block = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createBrick ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
        -0.5, 0.1, 0.5,
        -0.5, -0.1, 0.5,
        0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        0.5, -0.1, 0.5,
        0.5, 0.1, 0.5,
        0.5, 0.1, 0.5,
        0.5, -0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, 0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, 0.1, -0.5,
        0.5, 0.1, -0.5,
        0.5, -0.1, -0.5,
        -0.5, -0.1, -0.5,
        0.5, 0.1, -0.5,
        -0.5, -0.1, -0.5,
        -0.5, 0.1, -0.5,
        -0.5, 0.1, -0.5,
        -0.5, -0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, 0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        -0.5, 0.1, -0.5,
        -0.5, 0.1, 0.5,
        0.5, 0.1, 0.5,
        -0.5, 0.1, -0.5,
        0.5, 0.1, 0.5,
        0.5, 0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, -0.1, -0.5,
        0.5, -0.1, -0.5,
        -0.5, -0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        0.5, 0.1, -0.5,
        0.5, 0.25, -0.5,
    };

    static const GLfloat color_buffer_data [] = {
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,

        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        0, 0, 0,
        0, 0, 0,
        1, 1, 1,
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    brick = create3DObject(GL_TRIANGLES, 13*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createOrangeBrick ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
        -0.5, 0.1, 0.5,
        -0.5, -0.1, 0.5,
        0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        0.5, -0.1, 0.5,
        0.5, 0.1, 0.5,
        0.5, 0.1, 0.5,
        0.5, -0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, 0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, 0.1, -0.5,
        0.5, 0.1, -0.5,
        0.5, -0.1, -0.5,
        -0.5, -0.1, -0.5,
        0.5, 0.1, -0.5,
        -0.5, -0.1, -0.5,
        -0.5, 0.1, -0.5,
        -0.5, 0.1, -0.5,
        -0.5, -0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, 0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        -0.5, 0.1, -0.5,
        -0.5, 0.1, 0.5,
        0.5, 0.1, 0.5,
        -0.5, 0.1, -0.5,
        0.5, 0.1, 0.5,
        0.5, 0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, -0.1, -0.5,
        0.5, -0.1, -0.5,
        -0.5, -0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        0.5, 0.1, -0.5,
        0.5, 0.25, -0.5,
    };

    static const GLfloat color_buffer_data [] = {
        1,0.5,0,
        1,1,1,
        1,0.5,0,

        1,0.5,0,
        1,1,1,
        1,0.5,0,

        1,0.5,0,
          1,1,1,
        1,0.5,0,

        1,0.5,0,
          1,1,1,
        1,0.5,0,

        1,0.5,0,
        1,1,1,
        1,0.5,0,

        1,0.5,0,
        1,1,1,
        1,0.5,0,

        1,0.5,0,
        1,1,1,
        1,0.5,0,

        1,0.5,0,
        1,1,1,
        1,0.5,0,

        1,0.5,0,
        1,1,1,
        1,0.5,0,

        1,0.5,0,
      1,1,1,
        1,0.5,0,

        1,0.5,0,
        1,1,1,
        1,0.5,0,

        1,0.5,0,
        1,1,1,
        1,0.5,0,

        1,0.5,0,
        1,1,1,
        1,0.5,0
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    orange_brick = create3DObject(GL_TRIANGLES, 13*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createBridgeBrick ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
        -0.5, 0.1, 0.5,
        -0.5, -0.1, 0.5,
        0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        0.5, -0.1, 0.5,
        0.5, 0.1, 0.5,
        0.5, 0.1, 0.5,
        0.5, -0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, 0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, 0.1, -0.5,
        0.5, 0.1, -0.5,
        0.5, -0.1, -0.5,
        -0.5, -0.1, -0.5,
        0.5, 0.1, -0.5,
        -0.5, -0.1, -0.5,
        -0.5, 0.1, -0.5,
        -0.5, 0.1, -0.5,
        -0.5, -0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, 0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        -0.5, 0.1, -0.5,
        -0.5, 0.1, 0.5,
        0.5, 0.1, 0.5,
        -0.5, 0.1, -0.5,
        0.5, 0.1, 0.5,
        0.5, 0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, -0.1, -0.5,
        0.5, -0.1, -0.5,
        -0.5, -0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        0.5, 0.1, -0.5,
        0.5, 0.25, -0.5,
    };

    static const GLfloat color_buffer_data [] = {
        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    bridge_brick = create3DObject(GL_TRIANGLES, 13*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}
VAO* switch_brick;
void createSwitchBrick ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
        -0.5, 0.1, 0.5,
        -0.5, -0.1, 0.5,
        0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        0.5, -0.1, 0.5,
        0.5, 0.1, 0.5,
        0.5, 0.1, 0.5,
        0.5, -0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, 0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, 0.1, -0.5,
        0.5, 0.1, -0.5,
        0.5, -0.1, -0.5,
        -0.5, -0.1, -0.5,
        0.5, 0.1, -0.5,
        -0.5, -0.1, -0.5,
        -0.5, 0.1, -0.5,
        -0.5, 0.1, -0.5,
        -0.5, -0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, 0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        -0.5, 0.1, -0.5,
        -0.5, 0.1, 0.5,
        0.5, 0.1, 0.5,
        -0.5, 0.1, -0.5,
        0.5, 0.1, 0.5,
        0.5, 0.1, -0.5,
        -0.5, -0.1, 0.5,
        -0.5, -0.1, -0.5,
        0.5, -0.1, -0.5,
        -0.5, -0.1, 0.5,
        0.5, -0.1, -0.5,
        0.5, -0.1, 0.5,
        -0.5, 0.1, 0.5,
        0.5, 0.1, -0.5,
        0.5, 0.25, -0.5,
    };

    static const GLfloat color_buffer_data [] = {
        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,

        0.2,0.2,0,
        1,1,1,
        0.2,0.2,0,
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
switch_brick = create3DObject(GL_TRIANGLES, 13*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void moveBlock()
{
  int boardX = block_pos.x;
  int boardY = block_pos.z;
  if (blockState == 1 && arrow_key==1)
  {
    blockRotation = M_PI/2;
    axis.x = 0;
    axis.y = 0;
    axis.z = 1;
    block_pos.x -= 1.5;
    block_pos.y = 0.5;
    blockState = 3;
    arrow_key = 0;
  }
  else if (blockState == 2 && arrow_key==1)
  {
    blockRotation = M_PI/2;
    axis.x = 1;
    axis.y = 0;
    axis.z = 0;
    block_pos.x -= 1;
    //    block_pos.z = 0.25;
    block_pos.y =0.5;
    blockState = 2;
    arrow_key=0;
  }
  else if (blockState == 3 && arrow_key==1)
  {
    blockRotation = M_PI/2;
    axis.x = 0;
    axis.y = 1;
    axis.z = 0;
    block_pos.y =1;
  //  block_pos.y = 1;
    block_pos.x -= 1.5;
    blockState = 1;
    arrow_key=0;
  }
  else if (blockState == 1 && arrow_key==2)
  {
    blockRotation = M_PI/2;
    axis.x = 0;
    axis.y = 0;
    axis.z = 1;
    block_pos.x += 1.5;
    block_pos.y = 0.5;
    blockState = 3;
    arrow_key=0;
  }
  else if (blockState == 2 && arrow_key==2)
  {
    blockRotation = M_PI/2;
    axis.x = 1;
    axis.y = 0;
    axis.z = 0;
    block_pos.x += 1;
    block_pos.y =0.5;
    //    block_pos.z = 0.25;
    blockState = 2;
    arrow_key=0;
  }
  else if (blockState == 3 && arrow_key==2)
	{
		blockRotation = M_PI;
		axis.x = 0;
		axis.y = 0;
		axis.z = 1;
		block_pos.x += 1.5;
    block_pos.y =0.5;
	//	block_pos.z = 1.25;
		blockState = 1;
    arrow_key=0;
	}
  else if (blockState == 1 && arrow_key==3)
	{
		blockRotation = M_PI/2;
		axis.x = 1;
		axis.y = 0;
		axis.z = 0;
		block_pos.y = 0.5;
  		block_pos.z -= 1.5;
		blockState = 2;
    arrow_key=0;
	}
	else if (blockState == 2 && arrow_key==3)
	{
		blockRotation = M_PI/2;
		axis.x = 0;
		axis.y = -1;
		axis.z = 0;
		block_pos.z -= 1.5;
		blockState = 1;
    block_pos.y =1;
    arrow_key=0;
	}
  else if (blockState == 3 && arrow_key==3)
	{
		blockRotation = M_PI/2;
		axis.x = 0;
		axis.y = 0;
		axis.z = 1;
	//	block_pos.y += 1;
  block_pos.y =0.5;
		block_pos.z -= 1;
		blockState = 3;
    arrow_key=0;
	}
	else if (blockState == 1 && arrow_key==4)
	{
		blockRotation = -M_PI/2;
		axis.x = -1;
		axis.y = 0;
		axis.z = 0;
		block_pos.z += 1.5;
    block_pos.y = 0.5;
		blockState = 2;
		arrow_key=0;
	}
	else if (blockState == 2 && arrow_key==4)
	{
		blockRotation = M_PI/2;
		axis.x = 0;
		axis.y = 1;
		axis.z = 0;
		//block_pos.y -= 2;
    block_pos.y =1;
		block_pos.z += 1.5;
		blockState = 1;
    arrow_key=0;
	}
	else if (blockState == 3 && arrow_key==4)
	{
		blockRotation = M_PI/2;
		axis.x = 0;
		axis.y = 0;
		axis.z = 1;
		block_pos.z += 1;
    block_pos.y =0.5;
	//	block_pos.y = 0.75;
		blockState = 3;
    arrow_key=0;
	}
  else if(map1[level][boardX][boardY]==2 && blockState==1)
  {
    printf("you win\n");
    system("mpg123 -n 100 -q cheer.mp3 &");
    block_pos.y -= 1;
    if(level<2)
    {
      level++;
      block_pos.x = initPos[level][0];
      block_pos.z = initPos[level][1];
      block_pos.y = 1;
      bridge_toggle = 0;
      jump = 0;
    }
    else if(level==2)
    {
      printf("you won all three levels\n");
      exit(0);
    }
  }
  else if(map1[level][boardX][boardY]==3 && blockState==1)
  {
    printf("you are on a fragile tile\n");
    system("mpg123 -n 100 -q lose.mp3 &");
    block_pos.y -= 1;
    bridge_toggle = 0;
    jump = 0;
  }
  else if(map1[level][boardX][boardY]==0 || (map1[level][boardX][boardY]==4 && bridge_toggle==0) || boardX < 0 || boardY < 0)
  {
    printf("you fell\n");
    system("mpg123 -n 100 -q lose.mp3 &");
    block_pos.y -= 1;
    bridge_toggle = 0;
    jump = 0;
  }
printf("JUMP %d\n", jump);
  if(block_pos.y<-15)
  {
    //put block back to the initial Position
    block_pos.x = initPos[level][0];
    block_pos.z = initPos[level][1];
    block_pos.y = 1;
    blockState = 1;
    blockRotation = 0;
    axis.x = 0;
    axis.y = 0;
    axis.z = 1;
    arrow_key=0;
    bridge_toggle = 0;
    jump = 0;
  }
  printf("LEVEL %d\n", level);
  printf("CURRENT TIME %f\n",current_time);
  printf("MOVES %d\n",moves);
  printf("BLOCKSTATE %d\n",blockState);
  printf("BLOCKPOS: %f,%f,%f\n", block_pos.x,block_pos.y,block_pos.z);
}

void drawAxis()
{
    static const GLfloat X [] = {
        10, 0, 0,		-10, 0, 0,
    };
    static const GLfloat Y [] = {
        0, 10, 0,		0, -10, 0
    };
    static const GLfloat Z [] = {
        0, 0, 10,		0, 0, -10
    };
    static const GLfloat colX [] = {
        1, 1, 1,		1, 1, 1
    };
    static const GLfloat colY [] = {
        0, 1, 0,		0, 1, 0
    };
    static const GLfloat colZ [] = {
        1, 0, 0,		1, 0, 0
    };
    VAO* line1= create3DObject(GL_LINE_STRIP,2, X, colX, GL_FILL);
    VAO* line2= create3DObject(GL_LINE_STRIP,2, Y, colY, GL_FILL);
    VAO* line3= create3DObject(GL_LINE_STRIP,2, Z, colZ, GL_FILL);
    Matrices.model = glm::mat4(1.0);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(line1);
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(line2);
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(line3);
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

if(view==0)
{//normal
  eyeX = 8;
  eyeY = 10;
  eyeZ = 10;
  targetX = 0;
  targetY = 0;
  targetZ = 0;
}
else if(view==1)
{//block
  eyeX = block_pos.x-1;
  eyeY = 3;
  eyeZ = block_pos.z;
  targetX = block_pos.x+5;
  targetY = 0;
  targetZ = block_pos.z+5;
}
else if(view==2)
{//top
  eyeX = 0;
  eyeY = 12;
  eyeZ = 0;
  targetX = 1;
  targetY = 1;
  targetZ = 1;
}
else if(view==3)
{//tower
  eyeX = 7;
  eyeY = 15;
  eyeZ = 7;
  targetX = 1;
  targetY = 1;
  targetZ = 1;
}
else if(view==4)
{//follow camera actor mode
  eyeX = block_pos.x-5;
  eyeY = 5;
  eyeZ = block_pos.z;
  targetX = block_pos.x+5;
  targetY = 0;
  targetZ = block_pos.z+5;
}
else if(view==5)
{//helicopter WASD to move in helicopter mode
  heliViewFlag = 1;
  eyeX = camera_pos.x;
  eyeY = camera_pos.y;
  eyeZ = camera_pos.z;
  targetX = target_pos.x;
  targetY = target_pos.y;
  targetZ = target_pos.z;
}
else if(view==6)
{//followcam bird mode
  eyeX = block_pos.x-10;
  eyeY = 10;
  eyeZ = block_pos.z;
  targetX = block_pos.x+10;
  targetY = 10;
  targetZ = block_pos.z+10;
}

    // Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye (eyeX,eyeY,eyeZ);

	// Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (targetX,targetY,targetZ);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0,1,0);

    // Compute Camera matrix (view)
    // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
    //  Don't change unless you are sure!!
    Matrices.view = glm::lookAt(eye, target, up); // Fixed camera for 2D (ortho) in XY plane

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    VP = Matrices.projectionP * Matrices.view;

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    //glm::mat4 MVP;	// MVP = Projection * View * Model

    /* Render your scene */
    //  Don't change unless you are sure!!
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    drawAxis();

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();

    moveBlock();

    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateBlock = glm::translate (block_pos);        // glTranslatef
	glm::mat4 translateBlock_to_origin = glm::translate (glm::vec3(-1.0*block_pos.x,-1.0*block_pos.y,-1.0*block_pos.z));
	glm::mat4 rotateBlock = glm::rotate(blockRotation, axis);  // rotate about vector (1,0,0)
	glm::mat4 translateBlock_back = glm::translate (glm::vec3(block_pos.x, block_pos.y, block_pos.z));
	Matrices.model *= translateBlock_back*rotateBlock*translateBlock_to_origin*translateBlock;

    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(block);

    for(int i=0;i<11;i++)
    {
        for(int j=0;j<15;j++)
        {
            if(map1[level][i][j] == 1)
            {
                Matrices.model = glm::mat4(1.0f);
                glm::mat4 translateBrick = glm::translate(glm::vec3(0,0,0));
                translateBrick *= glm::translate(glm::vec3(i,0,j));
                Matrices.model *= translateBrick;
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(brick);
            }
            else if(map1[level][i][j]==3)
            {
              Matrices.model = glm::mat4(1.0f);
              glm::mat4 translateBrick = glm::translate(glm::vec3(0,0,0));
              translateBrick *= glm::translate(glm::vec3(i,0,j));
              Matrices.model *= translateBrick;
              MVP = VP * Matrices.model;
              glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
              draw3DObject(orange_brick);
            }
            else if(map1[level][i][j]==4 && bridgeCheck==1)
            {
              Matrices.model = glm::mat4(1.0f);
              glm::mat4 translateBrick = glm::translate(glm::vec3(0,0,0));
              translateBrick *= glm::translate(glm::vec3(i,0,j));
              Matrices.model *= translateBrick;
              MVP = VP * Matrices.model;
              glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
              draw3DObject(bridge_brick);
            }
            else if(map1[level][i][j]==5)
            {
              Matrices.model = glm::mat4(1.0f);
              glm::mat4 translateBrick = glm::translate(glm::vec3(0,0,0));
              translateBrick *= glm::translate(glm::vec3(i,0,j));
              Matrices.model *= translateBrick;
              MVP = VP * Matrices.model;
              glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
              draw3DObject(switch_brick);
            }
        }
    }
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
  //  glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
//    glfwSetScrollCallback(window, scroll_callback);

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
    // Create the models
    createBlock ();
    createBrick();
    createOrangeBrick();
    createBridgeBrick();
    createSwitchBrick();

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

    GLFWwindow* window = initGLFW(width, height);
    initGLEW();
    initGL (window, width, height);

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

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        last_update_time = current_time;

        if(current_time - last_update_time > 1)
        {
          printf("CURRENT TIME %f\n",current_time);
          printf("MOVES %d\n",moves);
          printf("BLOCKSTATE %d\n",blockState);
          printf("BLOCKPOS: %f,%f,%f\n", block_pos.x,block_pos.y,block_pos.z);
        }
    }

    glfwTerminate();
    //    exit(EXIT_SUCCESS);
}
