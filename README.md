# 3D Solar System Simulation

A real-time, interactive 3D visualization of the inner solar system—featuring the Sun and seven planets—orbiting under realistic relative distances, sizes, and motions. Built with OpenGL, GLUT, and SOIL for texture mapping, this program demonstrates lighting, texture mapping, and simple orbital mechanics in C/C++.

# Key Features

☀️ Sun as a light source with emissive material to illuminate the scene

🌍 Textured spheres for the Sun and each of the seven planets (Mercury through Uranus) plus a starscape background

🪐 Orbital motion driven by configurable orbital and rotational periods

💫 Visible ring systems for Saturn and Uranus, rendered with solid tori

🔄 Smooth animation via GLUT timer callbacks (approx. 60 FPS)

🎮 Camera controls using arrow keys and page-up/down for intuitive navigation

# Dependencies

OpenGL (GLUT)

SOIL (Simple OpenGL Image Library)

Standard C/C++ libraries (math, stdio, stdlib)
