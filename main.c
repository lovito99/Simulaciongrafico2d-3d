#include <GL/glut.h>

#include "render_escena.h"
#include "tipos.h"

static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    dibujar_escena_urbana();
    glutSwapBuffers();
}

static void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, ANCHO_VENTANA, 0.0, ALTO_VENTANA);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void init(void)
{
    glClearColor(0.09f, 0.12f, 0.16f, 1.0f);
    glPointSize(1.0f);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(ANCHO_VENTANA, ALTO_VENTANA);
    glutCreateWindow("Entrega 1 - Escena urbana 2D");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMainLoop();
    return 0;
}