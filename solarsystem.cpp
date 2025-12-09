#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <limits>
#include <sstream>
#include <vector>

// Math namespace
namespace {
// Pi constant
constexpr float PI = 3.1415f;

// Degrees to radians
inline float deg2rad(float d) {
    // Convert degrees
    return d * PI / 180.f;
}

// Random 0-1
inline float frand01() {
    // Normalize random
    return std::rand() / float(RAND_MAX);
}

// Random range
inline float frandRange(float a, float b) {
    // Interpolate range
    return a + (b - a) * frand01();
}
}

// Vector structure
struct Vector {
    // Components
    float x, y, z;
};

// Add vectors
inline Vector add(Vector a, Vector b) {
    // Sum components
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

// Subtract vectors
inline Vector sub(Vector a, Vector b) {
    // Difference components
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

// Multiply vector
inline Vector mul(Vector a, float s) {
    // Scale components
    return {a.x * s, a.y * s, a.z * s};
}

// Dot product
inline float dot(Vector a, Vector b) {
    // Sum Products
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Cross Product
inline Vector cross(Vector a, Vector b) {
    // Cross formula
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// Normalize
inline Vector norm(Vector v) {
    // Calculate length
    float L = std::sqrt(dot(v, v));
    // Check length
    if (L > 1e-6f) {
        // Return normalized
        return mul(v, 1.0f / L);
    }
    // Default direction
    return Vector{0, 0, -1};
}

// Normalize
inline void normalize(Vector& v) {
    // Calculate length
    float L = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    // Check length
    if (L > 1e-8f) {
        // Normalize x, y, and z
        v.x /= L;
        v.y /= L;
        v.z /= L;
    }
}

// Camera yaw
constexpr float camYaw   = 0.f;
// Camera pitch
constexpr float camPitch = -5.f;
// Previous time
int    gPrevMillis  = 0;
// Elapsed seconds
double gElapsedS    = 0.0;
// Camera position
Vector gCamPos      = { 0.0f, 8.0f, 85.0f };
// Movement speed
float  gMoveSpeed   = 25.0f;


// Forward key
bool kForward = false;
// Backward key
bool kBackward = false;
// Up key
bool kUp = false;
// Down key
bool kDown = false;
// Right key
bool kRight = false;
// Left key
bool kLeft = false;


// Orbit visibility
bool gShowOrbits = true;

// Sphere quadric
GLUquadric* gQuad = nullptr;

// Moon structure
struct Moon {
    // Moon properties
    float radius, orbitR, speed, angle;
    // Moon colour
    float colour[3];
};

// Planet
struct Planet {
    // Planet size
    float radius, orbitRX, orbitRY;
    // Planet motion
    float speed, angle, tilt;
    // Planet colour
    float colour[3];
    // Planet moons
    std::vector<Moon> moons;
};

// Planets
std::vector<Planet> gPlanets;

// Star
struct Star {
    // Position xyz
    float x,y,z;
    // Point size
    float size;
    // Colour phases
    float phaseR, phaseG, phaseB;
    // Twinkle parameters
    float speed, base, amp;
};

// Stars
std::vector<Star> gStars;
// Star count
constexpr int   starCount = 200;
// Minimum radius
constexpr float starMinR = 80.f;
// Maximum radius
constexpr float starMaxR = 150.f;


// Triangle structure
struct Tri {
    // Vertices
    int a, b, c;
};
// Enterprise vertices
std::vector<Vector> gEntVerts;
// Enterprise triangles
std::vector<Tri> gEntTris;
// Enterprise scale
float gEntScale = 1.0f;


// Camera basis
static inline void cameraBasis(Vector& fwd, Vector& right, Vector& up) {
    // Yaw radians
    const float cy = deg2rad(camYaw);
    // Pitch radians
    const float cp = deg2rad(camPitch);
    // Forward direction
    fwd = { std::sin(cy)*std::cos(cp), std::sin(cp), -std::cos(cy)*std::cos(cp) };
    // Normalize forward
    fwd = norm(fwd);
    // World up
    const Vector worldUp{0,1,0};
    // Right vector
    right = norm(cross(fwd, worldUp));
    // Up vector
    up    = norm(cross(right, fwd));
}

// Integrate camera
static inline void integrateCamera(float dt) {
    // Declare vectors
    Vector fwd, right, up;
    // Get basis
    cameraBasis(fwd, right, up);

    // No Velocity
    Vector vel{0, 0, 0};

    // Forward pressed
    if (kForward) {
        // Add forward
        vel = add(vel, fwd);
    }
    // Backward pressed
    if (kBackward) {
        // Subtract forward
        vel = sub(vel, fwd);
    }
    // Right pressed
    if (kRight) {
        // Add right
        vel = add(vel, right);
    }
    // Left pressed
    if (kLeft) {
        // Subtract right
        vel = sub(vel, right);
    }
    // Up pressed
    if (kUp) {
        // Add up
        vel = add(vel, up);
    }
    // Down pressed
    if (kDown) {
        // Subtract up
        vel = sub(vel, up);
    }

    // Check velocity
    if (dot(vel, vel) > 1e-8f) {
        // Scale velocity
        vel = mul(norm(vel), gMoveSpeed * dt);
        // Update position
        gCamPos = add(gCamPos, vel);
    }
}

// Set material
static inline void setMaterial(const float rgb[3], float emission = 0.0f) {
    // Diffuse colour
    const GLfloat diffuse[]  = { rgb[0], rgb[1], rgb[2], 1.0f };
    // Ambient colour
    const GLfloat ambient[]  = { rgb[0]*0.2f, rgb[1]*0.2f, rgb[2]*0.2f, 1.0f };
    // Specular colour
    const GLfloat specular[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    // Shininess value
    const GLfloat shininess  = 8.0f;
    // Emissive colour
    const GLfloat emissive[] = { rgb[0]*emission, rgb[1]*emission, rgb[2]*emission, 1.0f };
    // Set ambient
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  ambient);
    // Set diffuse
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  diffuse);
    // Set specular
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    // Set shininess
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    // Set emission
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissive);
}

// Draw sphere
static inline void drawSphere(float r, const float rgb[3], float emission = 0.0f) {
    // Apply material
    setMaterial(rgb, emission);
    // Render sphere
    gluSphere(gQuad, r, 36, 24);
}

// Draw orbit
static inline void drawOrbitRingLines(float rx, float rz, int segments = 256, float y = 0.0f) {
    // Disable lighting
    glDisable(GL_LIGHTING);
    // Line width
    glLineWidth(1.0f);
    // Begin lines
    glBegin(GL_LINES);

    // Loop segments
    for (int i = 0; i < segments; ++i) {
        // Segment start
        const float t0 = float(i) / segments;
        // Segment end
        const float t1 = float(i + 1) / segments;
        // Start angle
        const float a0 = t0 * 2.f * PI;
        // End angle
        const float a1 = t1 * 2.f * PI;
        // Start x
        const float x0 = rx * std::cos(a0);
        // Start z
        const float z0 = rz * std::sin(a0);
        // End x
        const float x1 = rx * std::cos(a1);
        // End z
        const float z1 = rz * std::sin(a1);

        // First vertex
        glVertex3f(x0, y, z0);
        // Second vertex
        glVertex3f(x1, y, z1);
    }

    // End lines
    glEnd();
    // Re-enable lighting
    glEnable(GL_LIGHTING);
}

// Build moons 
static Moon makeMoon(float radius, float orbitR, float speed, float angle,
                     float r, float g, float b) {
    Moon moon;
    moon.radius = radius;
    moon.orbitR = orbitR;
    moon.speed  = speed;
    moon.angle  = angle;
    moon.colour[0] = r;
    moon.colour[1] = g;
    moon.colour[2] = b;
    return moon;
}

// Build planets
static Planet makePlanet(float radius, float orbitRX, float orbitRY,
                         float speed, float angle, float tilt,
                         float r, float g, float b) {
    Planet planet;
    planet.radius = radius;
    planet.orbitRX = orbitRX;
    planet.orbitRY = orbitRY;
    planet.speed = speed;
    planet.angle = angle;
    planet.tilt = tilt;
    planet.colour[0] = r;
    planet.colour[1] = g;
    planet.colour[2] = b;
    planet.moons.clear();
    return planet;
}


// Initialize system
static void initSystem() {

    // Create planet1
    Planet planet1 = makePlanet(1.4f, 11.5f, 11.5f, 38.f,  10.f,  5.f,  1.0f,0.0f,1.0f);
    // Add moon
    planet1.moons.push_back(makeMoon(0.30f,2.0f, 90.f,0.f,0.9f,0.9f,1.0f));

    // Create planet2
    Planet planet2 = makePlanet(1.9f, 16.1f, 16.1f, 26.f, 120.f, 8.f,  0.0f,1.0f,0.8f);

    // Create planet3
    Planet planet3 = makePlanet(1.2f, 23.f, 18.4f, 18.f,  60.f, 3.f,  0.0f,0.5f,1.0f);

    // Create planet4
    Planet planet4 = makePlanet(2.3f, 27.6f, 27.6f, 12.f, 210.f, 23.f, 0.0f,1.0f,0.0f);
    // Add moon1
    planet4.moons.push_back(makeMoon(0.35f,2.5f,75.f,0.f,0.9f,0.9f,0.95f));
    // Add moon2
    planet4.moons.push_back(makeMoon(0.28f,3.6f,52.f,0.f,0.8f,0.9f,1.0f));

    // Create planet5
    Planet planet5 = makePlanet(2.0f, 34.5f, 34.5f, 9.f, 300.f,10.f, 1.0f,0.0f,0.0f);
    // Add moon
    planet5.moons.push_back(makeMoon(0.32f,2.2f,100.f,0.f,1.0f,0.85f,0.85f));

    // Create planet6
    Planet planet6 = makePlanet(3.1f, 43.7f, 43.7f, 6.f,  30.f, 2.f, 1.0f,0.5f,0.0f);

    // Store planets
    gPlanets.clear();
    gPlanets.reserve(6);
    gPlanets.push_back(planet1);
    gPlanets.push_back(planet2);
    gPlanets.push_back(planet3);
    gPlanets.push_back(planet4);
    gPlanets.push_back(planet5);
    gPlanets.push_back(planet6);
}


// Initialize stars
static void initStars() {
    // Clear stars
    gStars.clear();
    // Reserve space
    gStars.reserve(starCount);

    // Generate stars
    for (int i = 0; i < starCount; ++i) {
        // Random u
        const float u = frandRange(-1.f, 1.f);
        // Random theta
        const float th = frandRange(0.f, 2.f * PI);
        // Random radius
        const float r = frandRange(starMinR, starMaxR);
        // Sphere mapping
        const float s = std::sqrt(std::max(0.f, 1.f - u * u));
        // Calculate x
        const float x = r * s * std::cos(th);
        // Calculate y
        const float y = r * u;
        // Calculate z
        const float z = r * s * std::sin(th);

        // Create star
        Star st{};
        // Set x
        st.x = x;
        // Set y
        st.y = y;
        // Set z
        st.z = z;
        // Random size
        st.size = frandRange(1.5f, 3.5f);
        // Red phase
        st.phaseR = frandRange(0.f, 2.f * PI);
        // Green phase
        st.phaseG = frandRange(0.f, 2.f * PI);
        // Blue phase
        st.phaseB = frandRange(0.f, 2.f * PI);
        // Twinkle speed
        st.speed = frandRange(0.6f, 1.8f);
        // Base brightness
        st.base = frandRange(0.25f, 0.55f);
        // Brightness change
        st.amp = frandRange(0.35f, 0.75f);

        // Add star
        gStars.push_back(st);
    }
}

// Draw stars
static void drawStars(double t) {
    // Disable lighting
    glDisable(GL_LIGHTING);
    // Size buckets
    const float buckets[] = {1.5f, 2.0f, 2.5f, 3.0f, 3.5f};

    // Each size
    for (float sz : buckets) {
        // Set point size
        glPointSize(sz);
        // Begin points
        glBegin(GL_POINTS);

        // Each star
        for (const auto& s : gStars) {
            // Check size match
            if (std::fabs(s.size - sz) > 0.3f) {
                // Skip star
                continue;
            }

            // Twinkle value
            const float tw = s.base + s.amp * (0.5f * (std::sin(float(t) * s.speed + s.phaseR) + 1.0f)) * 0.9f;
            // Red
            const float r = std::min(1.f, std::max(0.f, tw * (0.6f + 0.4f * std::sin(float(t) * s.speed * 1.1f + s.phaseR))));
            // Green
            const float g = std::min(1.f, std::max(0.f, tw * (0.6f + 0.4f * std::sin(float(t) * s.speed * 0.9f + s.phaseG))));
            // Blue
            const float b = std::min(1.f, std::max(0.f, tw * (0.6f + 0.4f * std::sin(float(t) * s.speed * 1.3f + s.phaseB))));

            // Set colour
            glColor3f(r, g, b);
            // Draw point
            glVertex3f(s.x, s.y, s.z);
        }
        // End points
        glEnd();
    }
    // Enable lighting
    glEnable(GL_LIGHTING);
}

// Load Enterprise
static bool loadEnterprise() {
    // Clear vertices
    gEntVerts.clear();
    // Clear triangles
    gEntTris.clear();

    // Open file
    std::ifstream in("enterprise.txt");
    // Line buffer
    std::string line;

    // Read lines
    while (std::getline(in, line)) {
        // Check empty
        if (line.empty()) {
            // Skip line
            continue;
        }

        // Parse line
        std::istringstream ss(line);
        // Line type
        char type;
        // Read type
        ss >> type;

        // Vertex line
        if (type == 'v') {
            // Create vertex
            Vector v{};
            // Read coordinates
            ss >> v.x >> v.y >> v.z;
            // Add vertex
            gEntVerts.push_back(v);
        }
        // Face line
        else if (type == 'f') {
            // Create triangle
            Tri t{};
            // Read indices
            ss >> t.a >> t.b >> t.c;
            // Add triangle
            gEntTris.push_back(t);
        }
    }

    // Minimum bounds
    Vector mn{std::numeric_limits<float>::max(),
          std::numeric_limits<float>::max(),
          std::numeric_limits<float>::max()};

    // Maximum bounds
    Vector mx{-mn.x, -mn.y, -mn.z};

    // Each vertex
    for (auto& v : gEntVerts) {
        // Min x
        mn.x = std::min(mn.x, v.x);
        // Min y
        mn.y = std::min(mn.y, v.y);
        // Min z
        mn.z = std::min(mn.z, v.z);
        // Max x
        mx.x = std::max(mx.x, v.x);
        // Max y
        mx.y = std::max(mx.y, v.y);
        // Max z
        mx.z = std::max(mx.z, v.z);
    }

    // Calculate center
    Vector center = {(mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f, (mn.z + mx.z) * 0.5f};
    // X size
    const float sx = mx.x - mn.x;
    // Y size
    const float sy = mx.y - mn.y;
    // Z size
    const float sz = mx.z - mn.z;
    // Longest axis
    const float longest = std::max(sx, std::max(sy, sz));
    // Calculate scale
    gEntScale = (longest > 1e-4f) ? (6.0f / longest) : 1.0f;

    // Each vertex
    for (auto& v : gEntVerts) {
        // Center x
        v.x -= center.x;
        // Center y
        v.y -= center.y;
        // Center z
        v.z -= center.z;
    }

    // Success
    return true;
}

// Draw Enterprise
static void drawEnterpriseMesh() {

    // Save matrix
    glPushMatrix();
    // Position mesh
    glTranslatef(0.0f, -3.0f, -18.0f);
    // Scale mesh
    glScalef(gEntScale, gEntScale, gEntScale);
    // Rotate Y
    glRotatef(10.f * std::sin(gElapsedS*0.4), 0,1,0);
    // Rotate X
    glRotatef(5.f  * std::sin(gElapsedS*0.7), 1,0,0);

    // Hull colour
    const float hull[3] = {0.75f, 0.80f, 0.95f};
    // Apply material
    setMaterial(hull);

    // Begin triangles
    glBegin(GL_TRIANGLES);
    // Each triangle
    for (const auto& t : gEntTris) {

        // Index A
        const int ia = std::max(0, t.a - 1);
        // Index B
        const int ib = std::max(0, t.b - 1);
        // Index C
        const int ic = std::max(0, t.c - 1);
        // Bounds check
        if (ia >= (int)gEntVerts.size() || ib >= (int)gEntVerts.size() || ic >= (int)gEntVerts.size())
            // Skip triangle
            continue;
        // Vertex A
        const Vector& A = gEntVerts[ia];
        // Vertex B
        const Vector& B = gEntVerts[ib];
        // Vertex C
        const Vector& C = gEntVerts[ic];
        // Calculate normal
        Vector N = cross(sub(B,A), sub(C,A));
        // Normalize normal
        normalize(N);
        // Set normal
        glNormal3f(N.x, N.y, N.z);
        // Vertex A
        glVertex3f(A.x, A.y, A.z);
        // Vertex B
        glVertex3f(B.x, B.y, B.z);
        // Vertex C
        glVertex3f(C.x, C.y, C.z);
    }
    // End triangles
    glEnd();
    // Restore matrix
    glPopMatrix();
}

// Print controls
static void printControls() {

    // Header
    std::printf("CONTROLS\n");
    // Separator
    std::printf("-------------------------------\n");
    // R key
    std::printf("R           : toggle rings\n");
    // Up arrow
    std::printf("Up Arrow    : move up\n");
    // Down arrow
    std::printf("Down Arrow  : move down\n");
    // Right arrow
    std::printf("Right Arrow : move right\n");
    // Left arrow
    std::printf("Left Arrow  : move left\n");
    // Page up
    std::printf("Page Up     : move forward\n");
    // Page down
    std::printf("Page Down   : move backward\n");
    // Separator
    std::printf("-------------------------------\n");
}

// Initialize OpenGL
static void initGL() {
    // Enable depth
    glEnable(GL_DEPTH_TEST);
    // Enable culling
    glEnable(GL_CULL_FACE);
    // Cull back
    glCullFace(GL_BACK);

    // Enable lighting
    glEnable(GL_LIGHTING);
    // Enable light0
    glEnable(GL_LIGHT0);

    // Light position
    const GLfloat lightPos[] = {0,0,0,1};
    // Light colour
    const GLfloat lightCol[] = {1.5f,1.425f,1.275f,1};
    // Set position
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    // Set diffuse
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightCol);
    // Set specular
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightCol);

    // Background colour
    glClearColor(0,0,0,1);

    // Create quadric
    gQuad = gluNewQuadric();
    // Smooth normals
    gluQuadricNormals(gQuad, GLU_SMOOTH);
    // Fill style
    gluQuadricDrawStyle(gQuad, GLU_FILL);

    // Seed random
    std::srand(unsigned(std::time(nullptr)));

    // Initialize planets
    initSystem();
    // Initialize stars
    initStars();
    // Load model
    loadEnterprise();
    // Print controls
    printControls();
    // Initialize time
    gPrevMillis = glutGet(GLUT_ELAPSED_TIME);
}

// Update angles
static void updateAngles(float dt) {
    // Each planet
    for (auto& p : gPlanets) {
        // Update angle
        p.angle += p.speed * dt;
        // Wrap angle
        if (p.angle >= 360.f) {
            // Subtract 360
            p.angle -= 360.f;
        }

        // Each moon
        for (auto& m : p.moons) {
            // Update angle
            m.angle += m.speed * dt;
            // Wrap angle
            if (m.angle >= 360.f) {
                // Subtract 360
                m.angle -= 360.f;
            }
        }
    }
    // Update elapsed
    gElapsedS += dt;
    // Update camera
    integrateCamera(dt);
}

// Setup camera
static void setupCamera() {
    // Projection mode
    glMatrixMode(GL_PROJECTION);
    // Reset matrix
    glLoadIdentity();
    // Set perspective
    gluPerspective(65, (double)glutGet(GLUT_WINDOW_WIDTH)/(double)glutGet(GLUT_WINDOW_HEIGHT), 0.1, 500.0);
    // Modelview mode
    glMatrixMode(GL_MODELVIEW);
    // Reset matrix
    glLoadIdentity();

    // Get basis
    Vector fwd, right, up; cameraBasis(fwd,right,up);

    // Calculate target
    const Vector target = add(gCamPos, fwd);

    // Set view
    gluLookAt(gCamPos.x, gCamPos.y, gCamPos.z,
              target.x,   target.y,   target.z,
              up.x, up.y, up.z);
}

// Render scene
static void drawScene() {

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Setup camera
    setupCamera();

    // Light position
    const GLfloat lightPos[] = {0,0,0,1};
    // Update light
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // Draw stars
    drawStars(gElapsedS);

    // Check orbits
    if (gShowOrbits) {
        // Disable lighting
        glDisable(GL_LIGHTING);
        // Set colour
        glColor3f(0.8f, 0.8f, 0.8f);

        // Each planet
        for (const auto& p : gPlanets) {

            // Orbit X radius
            const float rx = p.orbitRX;
            // Orbit Z radius
            const float rz = p.orbitRY;
            // Draw ring
            drawOrbitRingLines(rx, rz, 256, 0.0f);
        }
        // Re-enable lighting
        glEnable(GL_LIGHTING);
    }
    // Sun scope
    {
        // Save matrix
        glPushMatrix();
        // Sun colour
        const float sun[3] = {1.0f, 0.95f, 0.2f};
        // Draw sun
        drawSphere(4.0f, sun, 0.8f);
        // Restore matrix
        glPopMatrix();
    }

    // Each planet
    for (auto& p : gPlanets) {

        // Convert angle
        const float ang = deg2rad(p.angle);
        // Calculate x
        const float x = p.orbitRX * std::cos(ang);
        // Calculate z
        const float z = p.orbitRY * std::sin(ang);

        // Save matrix
        glPushMatrix();
        // Position planet
        glTranslatef(x, 0.0f, z);
        // Tilt planet
        glRotatef(p.tilt, 0,0,1);
        // Draw planet
        drawSphere(p.radius, p.colour);

        // Each moon
        for (auto& m : p.moons) {

            // Save matrix
            glPushMatrix();
            // Convert angle
            const float ma = deg2rad(m.angle);
            // Calculate x
            const float mx = m.orbitR * std::cos(ma);
            // Calculate z
            const float mz = m.orbitR * std::sin(ma);

            // Check orbits
            if (gShowOrbits) {
                // Disable lighting
                glDisable(GL_LIGHTING);
                // Set colour
                glColor3f(1.0f, 1.0f, 1.0f);
                // Draw ring
                drawOrbitRingLines(m.orbitR, m.orbitR, 96, 0.0f);
                // Re-enable lighting
                glEnable(GL_LIGHTING);
            }

            // Position moon
            glTranslatef(mx, 0.0f, mz);
            // Draw moon
            drawSphere(m.radius, m.colour);
            // Restore matrix
            glPopMatrix();
        }
        // Restore matrix
        glPopMatrix();
    }

    // Draw Enterprise
    drawEnterpriseMesh();
    // Swap buffers
    glutSwapBuffers();
}

// Display callback
static void display() {
    // Get time
    const int now = glutGet(GLUT_ELAPSED_TIME);
    // Calculate delta
    const float dt = (now - gPrevMillis) / 1000.0f;
    // Update previous
    gPrevMillis = now;
    // Update angles
    updateAngles(dt);
    // Render scene
    drawScene();
}

// Idle callback
static void idle() {
    // Request redisplay
    glutPostRedisplay();
}

// Reshape callback
static void reshape(int w, int h) {
    // Set viewport
    glViewport(0, 0, w, h);
}

// Keyboard callback
static void keyboard(unsigned char key, int, int) {
    // Check R
    if (key == 'r' || key == 'R') {
        // Toggle orbits
        gShowOrbits = !gShowOrbits;
    }
}

// Handle special
static void handleSpecialKey(int key, bool pressed) {
    // Page up
    if (key == GLUT_KEY_PAGE_UP) {
        // Set forward
        kForward = pressed;
    }
    // Page down
    if (key == GLUT_KEY_PAGE_DOWN) {
        // Set backward
        kBackward = pressed;
    }
    // Up arrow
    if (key == GLUT_KEY_UP) {
        // Set up
        kUp = pressed;
    }
    // Down arrow
    if (key == GLUT_KEY_DOWN) {
        // Set down
        kDown = pressed;
    }
    // Right arrow
    if (key == GLUT_KEY_RIGHT) {
        // Set right
        kRight = pressed;
    }
    // Left arrow
    if (key == GLUT_KEY_LEFT) {
        // Set left
        kLeft = pressed;
    }
}

// Special keys
static void specialKeys(int key, int, int) {
    // Handle press
    handleSpecialKey(key, true);
}

// Special keys up
static void specialKeysUp(int key, int, int) {
    // Handle release
    handleSpecialKey(key, false);
}

// Main function
int main(int argc, char** argv) {

    // Initialize GLUT
    glutInit(&argc, argv);
    // Set display mode
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    // Set Window Size
    glutInitWindowSize(1280, 720);
    // Create window
    glutCreateWindow("Solar System");
    // Initialize OpenGL
    initGL();

    // Register display
    glutDisplayFunc(display);
    // Register idle
    glutIdleFunc(idle);
    // Register Reshape
    glutReshapeFunc(reshape);
    // Register keyboard
    glutKeyboardFunc(keyboard);
    // Register special
    glutSpecialFunc(specialKeys);
    // Register special up
    glutSpecialUpFunc(specialKeysUp);
    // Start loop
    glutMainLoop();

    // Success
    return 0;
}
