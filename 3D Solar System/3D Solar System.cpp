// Include necessary libraries
#include <GL/glut.h>          // OpenGL Utility Toolkit for window management
#include <SOIL.h>             // Simple OpenGL Image Library for texture loading
#include <math.h>             // Math functions (sin, cos, etc.)
#include <stdio.h>            // Standard I/O functions
#include <stdlib.h>           // Standard library functions
#include <string.h>           // String manipulation functions
#include <vector>             // STL vector container
#include <ctime>              // Time functions for random seed

// Define GL_CLAMP_TO_EDGE if not already defined (for texture wrapping)
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// Constants
static const double PI = 3.14159;  // Pi constant for calculations
#define MAX_PLANETS 7              // Number of planets in our solar system

// Structure to represent a star in the background
struct Star {
    float x, y, z;         // 3D position of the star
    float size;            // Visual size of the star
    float brightness;      // Base brightness level
    float flickerSpeed;    // Speed of brightness variation
};

// Structure to track camera state
struct CameraState {
    float x, y, z;         // Camera position coordinates
    float tx, ty, tz;      // Camera target (look-at point)
    int targetPlanet;       // Index of planet being viewed (-1 for none)
    bool isMoving;         // Flag for camera movement state
} camera = { 0, 15, 60, 0, 5, 0, -1, false }; // Initialize camera with default values

// Structure for planet information (name and facts)
struct PlanetInfo {
    const char* name;       // Planet name
    const char* facts[3];   // Array of 3 facts about the planet
};

// Global variables for planet animation
static double orbitAngles[MAX_PLANETS] = { 0 };  // Current orbit angles for planets
static double spinAngles[MAX_PLANETS] = { 0 };   // Current rotation angles for planets
static float saturnRingAngle = 0.0f;             // Rotation angle for Saturn's rings

// Texture handles and filenames
static GLuint textures[MAX_PLANETS + 2];  // OpenGL texture IDs (planets + sun + background)
static const char* textureFiles[MAX_PLANETS + 2] = {  // Texture filenames
    "Sun.jpg", "Mercury.jpg", "Venus.jpg", "Earth.jpg",
    "Mars.jpg", "Jupiter.jpg", "Saturn.jpg", "Uranus.jpg",
    "galaxy.jpg"
};

// Planet system parameters
static const double planetDistances[] = { 6.0, 10.0, 14.0, 20.0, 30.0, 40.0, 50.0 };  // Orbital radii
static const double planetSizes[] = { 0.3, 0.6, 0.8, 1.0, 1.8, 1.5, 1.2 };            // Relative sizes
static const double orbitalPeriods[] = { 3.0, 6.0, 8.0, 12.0, 24.0, 30.0, 40.0 };     // Orbit speeds
static const double rotationalPeriods[] = { 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0 };      // Rotation speeds
static const bool hasVisibleRings[MAX_PLANETS] = { false, false, false, false, false, true, false }; // Ring flags

// Planet information database
static const PlanetInfo planetInfo[MAX_PLANETS] = {
    {"Mercury", {"- Closest to Sun", "- Extreme temperatures ", "- No atmosphere"}},
    {"Venus", {"- Hottest planet", "- Acid clouds ", "- Retrograde rotation"}},
    {"Earth", {"- Liquid water Lovely Earth <3 ", "- Life exists", "- 1 moon"}},
    {"Mars", {"- Red Planet", "- Olympus Mons ", "- 2 moons"}},
    {"Jupiter", {"- Largest planet", "- Great Red Spot ", "- 79 moons"}},
    {"Saturn", {"- Ring system", "- Low density ", "- 62 moons"}},
    {"Uranus", {"- Ice giant", "- Sideways rotation ", "- 27 moons"}}
};

// Light properties
static const GLfloat lightAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f };   // Ambient light
static const GLfloat lightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };   // Diffuse light
static const GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };  // Specular light

// Star container
std::vector<Star> stars;  // Vector to hold star objects

// Function to load textures from files
void loadTextures() {
    glEnable(GL_TEXTURE_2D);  // Enable 2D texturing
    // Load each texture file
    for (int i = 0; i <= MAX_PLANETS + 1; ++i) {
        // Load image file into OpenGL texture
        textures[i] = SOIL_load_OGL_texture(
            textureFiles[i],          // Filename
            SOIL_LOAD_AUTO,           // Automatic format detection
            SOIL_CREATE_NEW_ID,       // Create new texture ID
            SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y  // Generate mipmaps and flip Y
        );
        // Error checking
        if (!textures[i]) {
            printf("Texture load failed: %s\n", SOIL_last_result());
            exit(EXIT_FAILURE);
        }
        // Set texture parameters
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // Minification filter
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);               // Magnification filter
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);            // S-coordinate wrapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);            // T-coordinate wrapping
    }
}

// Initialize starfield with random stars
void initStars(int count) {
    std::srand(std::time(0));  // Seed random number generator with current time
    stars.resize(count);       // Resize vector to hold requested number of stars
    // Initialize each star with random properties
    for (int i = 0; i < count; ++i) {
        // Random position in a large volume
        stars[i].x = (rand() % 2000 - 1000) / 10.0f;
        stars[i].y = (rand() % 2000 - 1000) / 10.0f;
        stars[i].z = (rand() % 2000 - 1000) / 10.0f;
        stars[i].size = (rand() % 5 + 1) / 10.0f;              // Random size
        stars[i].brightness = (rand() % 50 + 50) / 100.0f;     // Random brightness
        stars[i].flickerSpeed = (rand() % 50 + 50) / 1000.0f;  // Random flicker speed
    }
}

// Initialize OpenGL settings
void initGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Set clear color to black
    glEnable(GL_DEPTH_TEST);                // Enable depth testing
    glEnable(GL_LIGHTING);                  // Enable lighting
    glEnable(GL_LIGHT0);                    // Enable light source 0
    glEnable(GL_NORMALIZE);                 // Enable automatic normalization
    glEnable(GL_LINE_SMOOTH);               // Enable line anti-aliasing
    glEnable(GL_BLEND);                     // Enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Set blending function

    // Set up light source properties
    const GLfloat lightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f };  // Light at origin (sun)
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);    // Set ambient light
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);    // Set diffuse light
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);  // Set specular light
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);  // Set light position

    // Enable color tracking for materials
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    loadTextures();    // Load all textures
    initStars(500);    // Initialize 500 stars
}

// Window resize handler
void reshape(int w, int h) {
    if (h == 0) h = 1;  // Prevent divide by zero
    glViewport(0, 0, w, h);  // Set viewport to cover entire window
    glMatrixMode(GL_PROJECTION);  // Switch to projection matrix
    glLoadIdentity();             // Reset projection matrix
    // Set up perspective projection
    gluPerspective(75.0, (float)w / h, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW);  // Switch back to modelview matrix
}

// Function to draw text on screen
void drawText(float x, float y, const char* text) {
    // Switch to orthographic projection for 2D text
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);  // Disable lighting for text
    glColor3f(1, 1, 1);     // White text
    glRasterPos2f(x, y);     // Position text

    // Draw each character of the string
    for (const char* c = text; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    glEnable(GL_LIGHTING);  // Re-enable lighting
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Draw information box for selected planet
void drawInfoBox(int planetIndex) {
    if (planetIndex < 0 || planetIndex >= MAX_PLANETS) return;  // Validate index

    const PlanetInfo& info = planetInfo[planetIndex];  // Get planet info
    const int lineCount = 4;  // Name + 3 facts
    const float lineHeight = 20.0f;
    const float padding = 15.0f;

    // Get window dimensions
    int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    // Calculate box dimensions
    float boxWidth = 250.0f;
    float boxHeight = lineCount * lineHeight + padding * 2;
    float startX = 20.0f;
    float startY = windowHeight - 50.0f;

    // Draw background rectangle
    glColor4f(0.1f, 0.1f, 0.2f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(startX, startY);
    glVertex2f(startX + boxWidth, startY);
    glVertex2f(startX + boxWidth, startY - boxHeight);
    glVertex2f(startX, startY - boxHeight);
    glEnd();

    // Draw border
    glLineWidth(2.0f);
    glColor4f(0.4f, 0.4f, 0.8f, 0.9f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(startX, startY);
    glVertex2f(startX + boxWidth, startY);
    glVertex2f(startX + boxWidth, startY - boxHeight);
    glVertex2f(startX, startY - boxHeight);
    glEnd();

    // Draw planet name and facts
    float textY = startY - padding;
    drawText(startX + padding, textY, info.name);
    textY -= lineHeight;
    for (int i = 0; i < 3; i++) {
        drawText(startX + padding, textY, info.facts[i]);
        textY -= lineHeight;
    }

    // Draw help text
    drawText(startX + padding, textY - 10, "Press Q to return");
}

// Update camera position based on current target
void updateCamera() {
    if (camera.targetPlanet != -1) {
        // Camera is focused on a planet
        int idx = camera.targetPlanet;
        double angle = orbitAngles[idx];
        double distance = planetDistances[idx];
        // Calculate planet position
        double px = -distance * sin(angle);
        double pz = distance * cos(angle);

        // Set camera position behind and above the planet
        float followDistance = 15.0f;
        float followHeight = 8.0f;
        camera.x = px - followDistance * cos(angle);
        camera.y = followHeight;
        camera.z = pz - followDistance * sin(angle);

        // Set look-at target to planet
        camera.tx = px;
        camera.ty = 0;
        camera.tz = pz;
    }
    else if (camera.isMoving) {
        // Camera is moving to default position
        float speed = 0.1f;
        camera.x += (camera.tx - camera.x) * speed;
        camera.y += (camera.ty - camera.y) * speed;
        camera.z += (camera.tz - camera.z) * speed;

        // Check if camera reached target
        if (fabs(camera.x - camera.tx) < 0.1f &&
            fabs(camera.y - camera.ty) < 0.1f &&
            fabs(camera.z - camera.tz) < 0.1f) {
            camera.isMoving = false;
        }
    }
}

// Keyboard input handler
void keyboard(unsigned char key, int x, int y) {
    int planetIndex = key - '1';  // Convert key to planet index (1-7)
    if (planetIndex >= 0 && planetIndex < MAX_PLANETS) {
        // Focus camera on selected planet
        camera.targetPlanet = planetIndex;
        camera.isMoving = false;
    }
    else if (key == '0' || key == 'q' || key == 'Q') {
        // Return to default view
        camera.targetPlanet = -1;
        camera.tx = 0;
        camera.ty = 5;
        camera.tz = 0;
        camera.x = 0;
        camera.y = 15;
        camera.z = 60;
        camera.isMoving = true;
    }
}

// Draw background (galaxy texture)
void drawBackground() {
    // Switch to orthographic projection for background
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Disable lighting and depth test for background
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[MAX_PLANETS + 1]);  // Galaxy texture

    // Draw fullscreen quad with texture
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(-1, -1);
    glTexCoord2f(1, 1); glVertex2f(1, -1);
    glTexCoord2f(1, 0); glVertex2f(1, 1);
    glTexCoord2f(0, 0); glVertex2f(-1, 1);
    glEnd();

    // Restore state
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Draw starfield
void drawStars() {
    static float time = 0.0f;  // Animation timer
    time += 0.01f;             // Increment timer

    // Save current state
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);     // Stars emit their own light
    glEnable(GL_BLEND);         // Enable blending for transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for bright effect

    // Draw each star
    for (const auto& star : stars) {
        // Calculate flickering effect using sine wave
        float flicker = 0.5f + 0.5f * sin(time * star.flickerSpeed);
        float alpha = star.brightness * flicker;

        // Set color and size with flicker effect
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glPointSize(star.size * (1.0f + flicker * 0.5f));

        // Draw star as a point
        glBegin(GL_POINTS);
        glVertex3f(star.x, star.y, star.z);
        glEnd();
    }

    // Draw some brighter stars
    glPointSize(3.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 20; i++) {
        glVertex3f(stars[i].x * 1.5f, stars[i].y * 1.5f, stars[i].z * 1.5f);
    }
    glEnd();

    // Restore state
    glPopAttrib();
}

// Draw a planet's orbit ring
void drawOrbitRing(double radius) {
    // Save current state
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);      // Orbits don't need lighting
    glColor4f(1.0f, 1.0f, 1.0f, 0.4f);  // Semi-transparent white
    glLineWidth(1.5f);           // Set line width
    glEnable(GL_LINE_SMOOTH);    // Anti-aliased lines

    // Draw circle for orbit path
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; ++i) {
        double angle = 2 * PI * i / 360;
        glVertex3d(radius * cos(angle), 0.0, radius * sin(angle));
    }
    glEnd();

    // Restore state
    glPopAttrib();
}

// Draw Saturn's rings
void drawSaturnRings(double size) {
    glPushMatrix();
    // Tilt rings for more realistic appearance
    glRotatef(25.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(saturnRingAngle, 0.0f, 0.0f, 1.0f);  // Rotate rings over time

    // Multiple layers for rings with different properties
    const int layers = 5;
    const float colors[][4] = {
        {0.9f, 0.85f, 0.7f, 0.6f}, {0.8f, 0.75f, 0.6f, 0.5f},
        {0.7f, 0.6f, 0.5f, 0.4f}, {0.6f, 0.55f, 0.5f, 0.3f},
        {0.5f, 0.45f, 0.4f, 0.2f}
    };

    // Draw each ring layer
    for (int i = 0; i < layers; ++i) {
        glPushMatrix();
        glColor4fv(colors[i]);  // Set ring color
        // Draw torus (donut shape) for ring
        glutSolidTorus(size * 0.02 * (i + 1), size * (2.5 + i * 0.4), 64, 128);
        glPopMatrix();
    }

    glPopMatrix();
    saturnRingAngle = fmod(saturnRingAngle + 0.3f, 360.0f);  // Update rotation angle
}

// Draw a textured sphere (planet or sun)
void drawTexturedSphere(GLuint tex, double rad, bool isSun = false) {
    glEnable(GL_TEXTURE_2D);  // Enable texturing
    glBindTexture(GL_TEXTURE_2D, tex);  // Bind specified texture
    GLUquadric* quad = gluNewQuadric();  // Create quadric object
    gluQuadricTexture(quad, GL_TRUE);    // Enable texture coordinates
    gluQuadricNormals(quad, GLU_SMOOTH); // Generate smooth normals

    // Special handling for sun (emits light)
    if (isSun) {
        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);  // Rotate for correct texture mapping
        GLfloat emit[] = { 1.0f, 1.0f, 0.9f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, emit);  // Make sun glow
    }

    // Draw the sphere
    gluSphere(quad, rad, 40, 40);

    // Reset emission if this was the sun
    if (isSun) {
        GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, black);  // Turn off emission
        glPopMatrix();
    }

    // Clean up
    gluDeleteQuadric(quad);
    glDisable(GL_TEXTURE_2D);
}

// Draw a planet with its orbit and rotation
void drawPlanet(double dist, double size, double orbitPeriod, double rotPeriod, int idx) {
    if (idx >= MAX_PLANETS) return;  // Validate index

    // Update orbital and rotational angles based on time
    double orbitSpeed = 2 * PI / orbitPeriod;
    orbitAngles[idx] = fmod(orbitAngles[idx] + orbitSpeed * 0.016, 2 * PI);
    spinAngles[idx] = fmod(spinAngles[idx] + (360.0 / rotPeriod) * 0.016, 360.0);

    // Calculate planet position in orbit
    double x = -dist * sin(orbitAngles[idx]);
    double z = dist * cos(orbitAngles[idx]);

    // Set up planet transformation
    glPushMatrix();
    glTranslatef(x, 0.0f, z);  // Move to orbit position
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);  // Rotate for correct texture mapping
    glRotatef(spinAngles[idx], 0.0f, 0.0f, 1.0f);  // Apply planet rotation

    // Set material properties
    GLfloat matAmbDiff[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat matSpec[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matAmbDiff);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT, GL_SHININESS, 10.0f);

    // Draw the planet
    drawTexturedSphere(textures[idx + 1], size);

    // Draw rings if this planet has them (Saturn)
    if (hasVisibleRings[idx]) {
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        drawSaturnRings(size);
        glPopAttrib();
    }

    glPopMatrix();
}

// Main display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Clear buffers
    drawBackground();  // Draw background first

    updateCamera();  // Update camera position

    // Set up view transformation
    glLoadIdentity();
    gluLookAt(camera.x, camera.y, camera.z,  // Eye position
        camera.tx, camera.ty, camera.tz,    // Look-at point
        0.0, 1.0, 0.0);                     // Up vector

    // Draw stars first (as background elements)
    drawStars();

    // Draw Sun at center
    glPushMatrix();
    drawTexturedSphere(textures[0], 3.0, true);  // Sun is larger and emits light
    glPopMatrix();

    // Draw all planets and their orbits
    for (int i = 0; i < MAX_PLANETS; i++) {
        drawOrbitRing(planetDistances[i]);  // Draw orbit path
        drawPlanet(planetDistances[i], planetSizes[i],  // Draw planet
            orbitalPeriods[i], rotationalPeriods[i], i);
    }

    // Draw info box if a planet is selected
    if (camera.targetPlanet != -1) {
        drawInfoBox(camera.targetPlanet);
    }

    glutSwapBuffers();  // Swap front and back buffers
}

// Timer callback for animation
void updateScene(int val) {
    glutPostRedisplay();  // Trigger redisplay
    glutTimerFunc(16, updateScene, 0);  // Set up next frame (~60fps)
}

// Special key handler (arrow keys, etc.)
void specialKeys(int key, int x, int y) {
    if (camera.targetPlanet == -1) {  // Only allow movement in default view
        switch (key) {
        case GLUT_KEY_LEFT:  camera.x -= 0.5f; break;      // Move left
        case GLUT_KEY_RIGHT: camera.x += 0.5f; break;     // Move right
        case GLUT_KEY_UP:    camera.y -= 0.5f; break;       // Move up
        case GLUT_KEY_DOWN:  camera.y += 0.5f; break;     // Move down
        case GLUT_KEY_PAGE_UP:   camera.z -= 1.0f; break;  // Zoom in
        case GLUT_KEY_PAGE_DOWN: camera.z += 1.0f; break; // Zoom out
        }
        // Clamp zoom distance
        camera.z = fmax(fmin(camera.z, 150.0f), 10.0f);
    }
}

// Main function
int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);
    // Set up display mode with double buffering, depth buffer, RGB color, and multisampling
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB | GLUT_MULTISAMPLE);
    glutInitWindowSize(1920, 1080);  // Set window size to HD
    glutCreateWindow("Solar System Viewer with Info Boxes");  // Create window
    glutFullScreen();  // Start in fullscreen mode

    initGL();  // Initialize OpenGL settings

    // Register callback functions
    glutDisplayFunc(display);    // Main display function
    glutReshapeFunc(reshape);    // Window resize handler
    glutKeyboardFunc(keyboard);  // Keyboard input handler
    glutSpecialFunc(specialKeys); // Special key handler
    glutTimerFunc(0, updateScene, 0);  // Start animation timer

    glutMainLoop();  // Enter main event loop
    return 0;
}
