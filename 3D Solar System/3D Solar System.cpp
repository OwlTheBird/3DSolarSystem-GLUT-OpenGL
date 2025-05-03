#include <GL/glut.h>
#include <SOIL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

static const double PI = 3.14159;
#define MAX_PLANETS 7

static double orbitAngles[MAX_PLANETS] = { 0 }; // Current orbital angles
static double spinAngles[MAX_PLANETS] = { 0 };  // Current rotation angles
static int numDrawCalls = 0;                // Draw call counter (debug)

// Texture handling
static GLuint textures[MAX_PLANETS + 2];   // Stores texture IDs (sun + planets + starscape)
static const char* textureFiles[MAX_PLANETS + 2] = {
    "Sun.jpg", "Mercury.jpg", "Venus.jpg", "Earth.jpg",
    "Mars.jpg", "Jupiter.jpg", "Saturn.jpg", "Uranus.jpg",  // Texture filenames
    "starscape.jpg"
};

// Planetary data arrays (distances , sizes relative to Earth)
static const double planetDistances[] = { 6.0, 10.0, 14.0, 20.0, 30.0, 40.0, 50.0 }; // Distance from sun
static const double planetSizes[] = { 0.3,  0.6,  0.8,  1.0,  1.8,  1.5,  1.2 }; // Planet radii
static const double orbitalPeriods[] = { 3.0,  6.0,  8.0, 12.0, 24.0, 30.0, 40.0 }; // Days per full orbit
static const double rotationalPeriods[] = { 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0 }; // Days per full rotation
static const bool hasVisibleRings[MAX_PLANETS] = { false, false, false, false, true, true, false }; // Ring visibility flags

// Lighting properties
static const GLfloat lightAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f }; // Ambient light component
static const GLfloat lightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Diffuse light component
static const GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Specular light component

// Camera position variables
static float camX = 0.0f, camY = 5.0f, camZ = 60.0f;

// Texture loading function
void loadTextures() {
    glEnable(GL_TEXTURE_2D);
    for (int i = 0; i <= MAX_PLANETS + 1; ++i) {
        // Load image file using SOIL
        textures[i] = SOIL_load_OGL_texture(
            textureFiles[i],
            SOIL_LOAD_AUTO,
            SOIL_CREATE_NEW_ID,
            SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
        );
        if (textures[i] == 0) {
            printf("Failed to load %s: %s\n", textureFiles[i], SOIL_last_result());
            exit(EXIT_FAILURE);
        }
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        // Configure texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

void initGL(void) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
    glEnable(GL_DEPTH_TEST); // Enable depth buffering
    glEnable(GL_LIGHTING); // Enable lighting system
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);

    // Configure light source (Sun)
    const GLfloat lightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    // Configure material properties
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    loadTextures();
}

// ASpect Ratio
void reshape(int width, int height) {
    if (height == 0) height = 1;
    float aspect = (float)width / (float)height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(75.0, aspect, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Draw starscape background
void drawBackground() {
    // Temporarily switch to 2D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
    // Draw fullscreen textured quad

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[MAX_PLANETS + 1]);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(-1, -1);
    glTexCoord2f(1, 1); glVertex2f(1, -1);
    glTexCoord2f(1, 0); glVertex2f(1, 1);
    glTexCoord2f(0, 0); glVertex2f(-1, 1);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Create a textured sphere
void drawTexturedSphere(GLuint texID, double radius, bool isSun = false) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID);
    GLUquadric* quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluQuadricNormals(quad, GLU_SMOOTH);

    if (isSun) {
        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        // Make Sun self-illuminated
        GLfloat sunEmission[] = { 1.0f, 1.0f, 0.9f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, sunEmission);
    }

    gluSphere(quad, radius, 40, 40);

    if (isSun) {
        GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, black);
        glPopMatrix();
    }

    gluDeleteQuadric(quad);
    glDisable(GL_TEXTURE_2D);
}


// Draw planet with orbital mechanics
void drawPlanet(double distance, double size, double orbitalPeriod, double rotationalPeriod, int index) {
    if (index >= MAX_PLANETS) return;

    double orbitalSpeed = 2 * PI / orbitalPeriod;
    double rotationalSpeed = 360.0 / rotationalPeriod;

    orbitAngles[index] += orbitalSpeed * 0.016; // 16ms frame time approximation
    if (orbitAngles[index] > 2 * PI) orbitAngles[index] -= 2 * PI;

    spinAngles[index] += rotationalSpeed * 0.016; // Calculate rotational motion
    if (spinAngles[index] > 360.0) spinAngles[index] -= 360.0;

    double origX = distance * cos(orbitAngles[index]);
    double origZ = distance * sin(orbitAngles[index]);
    double x = -origZ;
    double z = origX;

    glPushMatrix();
    glTranslatef((GLfloat)x, 0.0f, (GLfloat)z);

    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glRotatef((GLfloat)spinAngles[index], 0.0f, 0.0f, 1.0f);

    // Set planet material properties
    GLfloat planetAmbDiff[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat planetSpecular[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, planetAmbDiff);
    glMaterialfv(GL_FRONT, GL_SPECULAR, planetSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, 10.0f);

    drawTexturedSphere(textures[index + 1], size);

    if (hasVisibleRings[index]) {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING);

        if (index == 4) {
            glColor3f(0.7f, 0.5f, 0.3f);
            glutSolidTorus(size * 0.05, size * 1.2, 30, 30);
        }
        else if (index == 5) {
            glColor3f(0.9f, 0.9f, 0.95f);
            glutSolidTorus(size * 0.02, size * 2.5, 30, 30);
        }

        glPopAttrib();
    }

    glPopMatrix();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw starscape background
    drawBackground();

    // Set up 3D view
    glLoadIdentity();
    gluLookAt(camX, camY, camZ, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    // Draw Sun (light source)
    glPushMatrix();
    drawTexturedSphere(textures[0], 3.0, true);
    glPopMatrix();

    // Draw planets with lighting
    for (int i = 0; i < MAX_PLANETS; i++) {
        drawPlanet(planetDistances[i], planetSizes[i],
            orbitalPeriods[i], rotationalPeriods[i], i);
    }

    glutSwapBuffers();
    numDrawCalls++;
}

void updateScene(int value) {
    glutPostRedisplay();
    glutTimerFunc(16, updateScene, 0);
}

void specialKeys(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_LEFT:  camX -= 0.5f; break;
    case GLUT_KEY_RIGHT: camX += 0.5f; break;
    case GLUT_KEY_UP:    camY -= 0.5f; break;
    case GLUT_KEY_DOWN:  camY += 0.5f; break;
    case GLUT_KEY_PAGE_UP:   camZ -= 1.0f; break;
    case GLUT_KEY_PAGE_DOWN: camZ += 1.0f; break;
    }
    camZ = fmax(fmin(camZ, 150.0f), 10.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
    glutInitWindowSize(1920, 1080);
    glutCreateWindow("3D Solar System");
    glutFullScreen();

    initGL();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(0, updateScene, 0);

    glutMainLoop();
    return 0;
}