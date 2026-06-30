#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>

#include "render_escena.h"
#include "tipos.h"

/* Estado global de la animacion */
EstadoAnim ESTADO = {
    .vehiculo_tx  = 0.0f,
    .angulo_rueda = 0.0f,
    .nube_tx      = 500.0f,
    .arbol_escala = 1.0f,
    .shear_senal  = 0.0f,
    .pausa        = 0
};

static float arbol_dir = 1.0f;   /* direccion de oscilacion de escala */
static float shear_dir = 1.0f;   /* direccion de oscilacion de shear  */

static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    dibujar_escena_animada(&ESTADO);
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

static void timer(int valor)
{
    (void)valor;

    if (!ESTADO.pausa) {
        /* TRASLACION: vehiculo avanza por la carretera */
        ESTADO.vehiculo_tx += 1.8f;
        if (ESTADO.vehiculo_tx > 1050.0f)
            ESTADO.vehiculo_tx = -300.0f;

        /* ROTACION + COMPOSICION: ruedas giran en sentido al movimiento
         * Composicion: M_rueda = T(tx,0) * R_pivote(ang, cx, cy)        */
        ESTADO.angulo_rueda -= 0.07f;
        if (ESTADO.angulo_rueda < -6.2832f)
            ESTADO.angulo_rueda += 6.2832f;

        /* TRASLACION: nube se desplaza de derecha a izquierda */
        ESTADO.nube_tx -= 0.5f;
        if (ESTADO.nube_tx < -130.0f)
            ESTADO.nube_tx = 960.0f;

        /* ESCALA: arbol derecho oscila uniformemente entre 0.65 y 1.45 */
        ESTADO.arbol_escala += 0.004f * arbol_dir;
        if (ESTADO.arbol_escala >= 1.45f) arbol_dir = -1.0f;
        if (ESTADO.arbol_escala <= 0.65f) arbol_dir =  1.0f;

        /* DISTORSION: senal de transito con shear oscilante */
        ESTADO.shear_senal += 0.003f * shear_dir;
        if (ESTADO.shear_senal >=  0.40f) shear_dir = -1.0f;
        if (ESTADO.shear_senal <= -0.40f) shear_dir =  1.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);   /* ~60 fps */
}

static void teclado(unsigned char tecla, int x, int y)
{
    (void)x; (void)y;
    switch (tecla) {
        case ' ':             /* pausa / reanuda */
            ESTADO.pausa = !ESTADO.pausa;
            break;
        case 'r': case 'R':   /* avanzar ruedas manualmente */
            ESTADO.angulo_rueda -= 0.3f;
            break;
        case 'e': case 'E':   /* forzar escala hacia arriba */
            ESTADO.arbol_escala += 0.05f;
            if (ESTADO.arbol_escala > 1.6f) ESTADO.arbol_escala = 1.6f;
            break;
        case 's': case 'S':   /* forzar escala hacia abajo */
            ESTADO.arbol_escala -= 0.05f;
            if (ESTADO.arbol_escala < 0.3f) ESTADO.arbol_escala = 0.3f;
            break;
        case 'd': case 'D':   /* aumentar shear */
            ESTADO.shear_senal += 0.05f;
            if (ESTADO.shear_senal > 0.6f) ESTADO.shear_senal = 0.6f;
            break;
        case 'f': case 'F':   /* reducir shear */
            ESTADO.shear_senal -= 0.05f;
            if (ESTADO.shear_senal < -0.6f) ESTADO.shear_senal = -0.6f;
            break;
        case 27:              /* ESC - salir */
            exit(0);
    }
    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(ANCHO_VENTANA, ALTO_VENTANA);
    glutCreateWindow("Entrega 2 - Transformaciones geometricas 2D y animacion");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(teclado);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
