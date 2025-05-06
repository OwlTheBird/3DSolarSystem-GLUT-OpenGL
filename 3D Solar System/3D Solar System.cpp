#include <GL/glut.h>
#include <SOIL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

static const double PI = 3.14159;
#define MAX_PLANETS 7

struct CameraState {
    float x, y, z;         // Camera position
    float tx, ty, tz;      // Look-at target
    int targetPlanet;      // -1 means no target
    bool isMoving;
} camera = { 0, 5, 60, 0, 0, 0, -1, false };

struct PlanetInfo {
    const char* name;
    const char* facts;
};

static double orbitAngles[MAX_PLANETS] = { 0 };
static double spinAngles[MAX_PLANETS] = { 0 };
static float saturnRingAngle = 0.0f;

static GLuint textures[MAX_PLANETS + 2];
static const char* textureFiles[MAX_PLANETS + 2] = {
    "Sun.jpg", "Mercury.jpg", "Venus.jpg", "Earth.jpg",
    "Mars.jpg", "Jupiter.jpg", "Saturn.jpg", "Uranus.jpg",
    "galaxy.jpg"
};

static const double planetDistances[] = { 6.0, 10.0, 14.0, 20.0, 30.0, 40.0, 50.0 };
static const double planetSizes[] = { 0.3, 0.6, 0.8, 1.0, 1.8, 1.5, 1.2 };
static const double orbitalPeriods[] = { 3.0, 6.0, 8.0, 12.0, 24.0, 30.0, 40.0 };
static const double rotationalPeriods[] = { 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0 };
static const bool hasVisibleRings[MAX_PLANETS] = { false, false, false, false, false, true, false };

static const PlanetInfo planetInfo[MAX_PLANETS] = {
    {"Mercury", "Closest to Sun\nTemperature extremes\nNo moons"},
    {"Venus", "Hottest planet\nAcid clouds\nRetrograde rotation"},
    {"Earth", "Our home\nLiquid water\n1 moon"},
    {"Mars", "Red Planet\nTallest volcano\n2 moons"},
    {"Jupiter", "Largest planet\nGreat Red Spot\n79 moons"},
    {"Saturn", "Ring system\nLow density\n62 moons"},
    {"Uranus", "Ice giant\nSideways rotation\n27 moons"}
};

static const GLfloat lightAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
static const GLfloat lightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

void loadTextures() {
    glEnable(GL_TEXTURE_2D);
    for (int i = 0; i <= MAX_PLANETS + 1; ++i) {
        textures[i] = SOIL_load_OGL_texture(
            textureFiles[i],
            SOIL_LOAD_AUTO,
            SOIL_CREATE_NEW_ID,
            SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
        );
        if (!textures[i]) {
            printf("Texture load failed: %s\n", SOIL_last_result());
            exit(EXIT_FAILURE);
        }
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

void initGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const GLfloat lightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    loadTextures();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(75.0, (float)w / h, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

void drawText(float x, float y, const char* text) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glColor3f(1, 1, 1);
    glRasterPos2f(x, y);

    for (const char* c = text; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void updateCamera() {
    if (camera.targetPlanet != -1) {
        // Calculate current planet position
        int idx = camera.targetPlanet;
        double angle = orbitAngles[idx];
        double distance = planetDistances[idx];
        double px = -distance * sin(angle);
        double pz = distance * cos(angle);

        // Set camera to follow planet (no smoothing)
        float followDistance = 15.0f;
        float followHeight = 8.0f;

        camera.x = px - followDistance * cos(angle);
        camera.y = followHeight;
        camera.z = pz - followDistance * sin(angle);

        camera.tx = px;
        camera.ty = 0;
        camera.tz = pz;
    }
    else if (camera.isMoving) {
        // Smooth transition for non-following movement
        float speed = 0.1f;
        camera.x += (camera.tx - camera.x) * speed;
        camera.y += (camera.ty - camera.y) * speed;
        camera.z += (camera.tz - camera.z) * speed;

        if (fabs(camera.x - camera.tx) < 0.1f &&
            fabs(camera.y - camera.ty) < 0.1f &&
            fabs(camera.z - camera.tz) < 0.1f) {
            camera.isMoving = false;
        }
    }
}

void keyboard(unsigned char key, int x, int y) {
    int planetIndex = key - '1';
    if (planetIndex >= 0 && planetIndex < MAX_PLANETS) {
        camera.targetPlanet = planetIndex;
        camera.isMoving = false;
    }
    else if (key == '0' || key == 'q' || key == 'Q') {
        camera.targetPlanet = -1;
        camera.tx = 0;
        camera.ty = 5;
        camera.tz = 60;
        camera.isMoving = true;
    }
}

void drawBackground() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

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

void drawOrbitRing(double radius) {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
    glLineWidth(1.5f);
    glEnable(GL_LINE_SMOOTH);

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; ++i) {
        double angle = 2 * PI * i / 360;
        glVertex3d(radius * cos(angle), 0.0, radius * sin(angle));
    }
    glEnd();

    glPopAttrib();
}

void drawSaturnRings(double size) {
    glPushMatrix();
    glRotatef(25.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(saturnRingAngle, 0.0f, 0.0f, 1.0f);

    const int layers = 5;
    const float colors[][4] = {
        {0.9f, 0.85f, 0.7f, 0.6f}, {0.8f, 0.75f, 0.6f, 0.5f},
        {0.7f, 0.6f, 0.5f, 0.4f}, {0.6f, 0.55f, 0.5f, 0.3f},
        {0.5f, 0.45f, 0.4f, 0.2f}
    };

    for (int i = 0; i < layers; ++i) {
        glPushMatrix();
        glColor4fv(colors[i]);
        glutSolidTorus(size * 0.02 * (i + 1), size * (2.5 + i * 0.4), 64, 128);
        glPopMatrix();
    }

    glPopMatrix();
    saturnRingAngle = fmod(saturnRingAngle + 0.3f, 360.0f);
}

void drawTexturedSphere(GLuint tex, double rad, bool isSun = false) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    GLUquadric* quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluQuadricNormals(quad, GLU_SMOOTH);

    if (isSun) {
        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        GLfloat emit[] = { 1.0f, 1.0f, 0.9f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, emit);
    }

    gluSphere(quad, rad, 40, 40);

    if (isSun) {
        GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, black);
        glPopMatrix();
    }

    gluDeleteQuadric(quad);
    glDisable(GL_TEXTURE_2D);
}

void drawPlanet(double dist, double size, double orbitPeriod, double rotPeriod, int idx) {
    if (idx >= MAX_PLANETS) return;

    // Update orbital position
    double orbitSpeed = 2 * PI / orbitPeriod;
    orbitAngles[idx] = fmod(orbitAngles[idx] + orbitSpeed * 0.016, 2 * PI);

    // Update rotation
    spinAngles[idx] = fmod(spinAngles[idx] + (360.0 / rotPeriod) * 0.016, 360.0);

    // Calculate current position
    double x = -dist * sin(orbitAngles[idx]);
    double z = dist * cos(orbitAngles[idx]);

    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(spinAngles[idx], 0.0f, 0.0f, 1.0f);

    GLfloat matAmbDiff[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat matSpec[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matAmbDiff);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT, GL_SHININESS, 10.0f);

    drawTexturedSphere(textures[idx + 1], size);

    if (hasVisibleRings[idx]) {
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);

        if (idx == 5) { // Saturn
            drawSaturnRings(size);
        }

        glPopAttrib();
    }

    glPopMatrix();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawBackground();

    updateCamera();

    glLoadIdentity();
    gluLookAt(camera.x, camera.y, camera.z,
        camera.tx, camera.ty, camera.tz,
        0.0, 1.0, 0.0);

    // Draw Sun
    glPushMatrix();
    drawTexturedSphere(textures[0], 3.0, true);
    glPopMatrix();

    // Draw planets and orbits
    for (int i = 0; i < MAX_PLANETS; i++) {
        drawOrbitRing(planetDistances[i]);
        drawPlanet(planetDistances[i], planetSizes[i],
            orbitalPeriods[i], rotationalPeriods[i], i);
    }

    // Draw info text
    if (camera.targetPlanet != -1) {
        char info[256];
        snprintf(info, sizeof(info), "%s\n%s (Press Q to exit)",
            planetInfo[camera.targetPlanet].name,
            planetInfo[camera.targetPlanet].facts);
        drawText(20, glutGet(GLUT_WINDOW_HEIGHT) - 40, info);
    }

    glutSwapBuffers();
}

void updateScene(int val) {
    glutPostRedisplay();
    glutTimerFunc(16, updateScene, 0);
}

void specialKeys(int key, int x, int y) {
    if (camera.targetPlanet == -1) {
        switch (key) {
        case GLUT_KEY_LEFT:  camera.x -= 0.5f; break;
        case GLUT_KEY_RIGHT: camera.x += 0.5f; break;
        case GLUT_KEY_UP:    camera.y -= 0.5f; break;
        case GLUT_KEY_DOWN:  camera.y += 0.5f; break;
        case GLUT_KEY_PAGE_UP:   camera.z -= 1.0f; break;
        case GLUT_KEY_PAGE_DOWN: camera.z += 1.0f; break;
        }
        camera.z = fmax(fmin(camera.z, 150.0f), 10.0f);
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB | GLUT_MULTISAMPLE);
    glutInitWindowSize(1920, 1080);
    glutCreateWindow("Solar System Viewer with Dynamic Camera");
    glutFullScreen();

    initGL();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(0, updateScene, 0);

    glutMainLoop();
    return 0;
}