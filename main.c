#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>

#include "render_escena.h"
#include "tipos.h"

/* Estado global de la animacion */
EstadoAnim ESTADO = {
    /* Auto-animacion */
    .vehiculo_tx  = 0.0f,
    .angulo_rueda = 0.0f,
    .nube_tx      = 500.0f,
    .shear_senal  = 0.0f,
    .pausa        = 0,

    /* Seleccion inicial */
    .objeto_sel   = OBJ_VEHICULO_ROJO,

    /* Bus azul: empieza fuera de pantalla por la derecha */
    .bus_tx       = 960.0f,
    .bus_angulo   = 0.0f,

    /* Edificio naranja: estado inicial neutro */
    .ed_naranja_tx      = 0.0f,
    .ed_naranja_ty      = 0.0f,
    .ed_naranja_escala  = 1.0f,
    .ed_naranja_angulo  = 0.0f,
    .ed_naranja_shear   = 0.0f,
    .ed_naranja_reflejo = 0,

    /* Edificio azul: estado inicial neutro */
    .ed_azul_tx      = 0.0f,
    .ed_azul_ty      = 0.0f,
    .ed_azul_escala  = 1.0f,
    .ed_azul_angulo  = 0.0f,
    .ed_azul_shear   = 0.0f,
    .ed_azul_reflejo = 0,
};

static float shear_dir = 1.0f;

/* Pasos de control manual */
#define PASO_TX   8.0f
#define PASO_TY   8.0f
#define PASO_VEH  14.0f

static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    dibujar_escena_animada(&ESTADO);
    glutSwapBuffers();
}

static void reshape(int ancho, int alto)
{
    glViewport(0, 0, ancho, alto);
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
        /* TRASLACION vehiculo rojo: izquierda -> derecha */
        ESTADO.vehiculo_tx += 1.8f;
        if (ESTADO.vehiculo_tx > 1050.0f)
            ESTADO.vehiculo_tx = -300.0f;

        /* ROTACION ruedas vehiculo rojo (composicion: T*R_pivote) */
        ESTADO.angulo_rueda -= 0.07f;
        if (ESTADO.angulo_rueda < -6.2832f)
            ESTADO.angulo_rueda += 6.2832f;

        /* TRASLACION bus azul: derecha -> izquierda (carril contrario) */
        ESTADO.bus_tx -= 1.5f;
        if (ESTADO.bus_tx < -260.0f)
            ESTADO.bus_tx = 960.0f;

        /* ROTACION ruedas bus (CCW porque avanza hacia la izquierda) */
        ESTADO.bus_angulo += 0.07f;
        if (ESTADO.bus_angulo > 6.2832f)
            ESTADO.bus_angulo -= 6.2832f;

        /* TRASLACION nube */
        ESTADO.nube_tx -= 0.5f;
        if (ESTADO.nube_tx < -130.0f)
            ESTADO.nube_tx = 960.0f;

        /* DISTORSION (SHEAR) senal de transito - automatica */
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

    /* ---- Control global ---- */
    case ' ':
        ESTADO.pausa = !ESTADO.pausa;
        break;
    case 27:          /* ESC */
        exit(0);

    /* ---- Seleccion de objeto (teclas 1-4 o TAB) ---- */
    case '1': ESTADO.objeto_sel = OBJ_VEHICULO_ROJO; break;
    case '2': ESTADO.objeto_sel = OBJ_BUS_AZUL;      break;
    case '3': ESTADO.objeto_sel = OBJ_ED_NARANJA;    break;
    case '4': ESTADO.objeto_sel = OBJ_ED_AZUL;       break;
    case '\t':
        ESTADO.objeto_sel = (ESTADO.objeto_sel + 1) % 4;
        break;

    /* ---- Reset del objeto activo ---- */
    case '0':
        switch (ESTADO.objeto_sel) {
        case OBJ_VEHICULO_ROJO:
            ESTADO.vehiculo_tx = 0.0f;
            break;
        case OBJ_BUS_AZUL:
            ESTADO.bus_tx = 960.0f;
            break;
        case OBJ_ED_NARANJA:
            ESTADO.ed_naranja_tx = ESTADO.ed_naranja_ty = 0.0f;
            ESTADO.ed_naranja_escala  = 1.0f;
            ESTADO.ed_naranja_angulo  = 0.0f;
            ESTADO.ed_naranja_shear   = 0.0f;
            ESTADO.ed_naranja_reflejo = 0;
            break;
        case OBJ_ED_AZUL:
            ESTADO.ed_azul_tx = ESTADO.ed_azul_ty = 0.0f;
            ESTADO.ed_azul_escala  = 1.0f;
            ESTADO.ed_azul_angulo  = 0.0f;
            ESTADO.ed_azul_shear   = 0.0f;
            ESTADO.ed_azul_reflejo = 0;
            break;
        }
        break;

    /* ---- Escala (E = agrandar, S = achicar) ---- */
    case 'e': case 'E':
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_escala += 0.05f;
            if (ESTADO.ed_naranja_escala > 3.0f) ESTADO.ed_naranja_escala = 3.0f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_escala += 0.05f;
            if (ESTADO.ed_azul_escala > 3.0f) ESTADO.ed_azul_escala = 3.0f;
        }
        break;
    case 's': case 'S':
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_escala -= 0.05f;
            if (ESTADO.ed_naranja_escala < 0.1f) ESTADO.ed_naranja_escala = 0.1f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_escala -= 0.05f;
            if (ESTADO.ed_azul_escala < 0.1f) ESTADO.ed_azul_escala = 0.1f;
        }
        break;

    /* ---- Rotacion ([ = antihoraria, ] = horaria) ---- */
    case '[':
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA)
            ESTADO.ed_naranja_angulo += 0.05f;
        else if (ESTADO.objeto_sel == OBJ_ED_AZUL)
            ESTADO.ed_azul_angulo += 0.05f;
        break;
    case ']':
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA)
            ESTADO.ed_naranja_angulo -= 0.05f;
        else if (ESTADO.objeto_sel == OBJ_ED_AZUL)
            ESTADO.ed_azul_angulo -= 0.05f;
        break;

    /* ---- Distorsion shear (D = mas, F = menos) ---- */
    case 'd': case 'D':
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_shear += 0.04f;
            if (ESTADO.ed_naranja_shear > 1.5f) ESTADO.ed_naranja_shear = 1.5f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_shear += 0.04f;
            if (ESTADO.ed_azul_shear > 1.5f) ESTADO.ed_azul_shear = 1.5f;
        }
        break;
    case 'f': case 'F':
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_shear -= 0.04f;
            if (ESTADO.ed_naranja_shear < -1.5f) ESTADO.ed_naranja_shear = -1.5f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_shear -= 0.04f;
            if (ESTADO.ed_azul_shear < -1.5f) ESTADO.ed_azul_shear = -1.5f;
        }
        break;

    /* ---- Reflexion (R = alternar) ---- */
    case 'r': case 'R':
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA)
            ESTADO.ed_naranja_reflejo = !ESTADO.ed_naranja_reflejo;
        else if (ESTADO.objeto_sel == OBJ_ED_AZUL)
            ESTADO.ed_azul_reflejo = !ESTADO.ed_azul_reflejo;
        break;
    }

    glutPostRedisplay();
}

static void teclas_especiales(int tecla, int x, int y)
{
    (void)x; (void)y;

    switch (tecla) {
    case GLUT_KEY_LEFT:
        switch (ESTADO.objeto_sel) {
        case OBJ_VEHICULO_ROJO: ESTADO.vehiculo_tx   -= PASO_VEH; break;
        case OBJ_BUS_AZUL:     ESTADO.bus_tx         -= PASO_VEH; break;
        case OBJ_ED_NARANJA:   ESTADO.ed_naranja_tx  -= PASO_TX;  break;
        case OBJ_ED_AZUL:      ESTADO.ed_azul_tx     -= PASO_TX;  break;
        }
        break;

    case GLUT_KEY_RIGHT:
        switch (ESTADO.objeto_sel) {
        case OBJ_VEHICULO_ROJO: ESTADO.vehiculo_tx   += PASO_VEH; break;
        case OBJ_BUS_AZUL:     ESTADO.bus_tx         += PASO_VEH; break;
        case OBJ_ED_NARANJA:   ESTADO.ed_naranja_tx  += PASO_TX;  break;
        case OBJ_ED_AZUL:      ESTADO.ed_azul_tx     += PASO_TX;  break;
        }
        break;

    case GLUT_KEY_UP:
        switch (ESTADO.objeto_sel) {
        case OBJ_ED_NARANJA: ESTADO.ed_naranja_ty += PASO_TY; break;
        case OBJ_ED_AZUL:   ESTADO.ed_azul_ty    += PASO_TY; break;
        default: break;
        }
        break;

    case GLUT_KEY_DOWN:
        switch (ESTADO.objeto_sel) {
        case OBJ_ED_NARANJA: ESTADO.ed_naranja_ty -= PASO_TY; break;
        case OBJ_ED_AZUL:   ESTADO.ed_azul_ty    -= PASO_TY; break;
        default: break;
        }
        break;
    }

    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(ANCHO_VENTANA, ALTO_VENTANA);
    glutCreateWindow("Entrega 2 - Transformaciones Geometricas 2D");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(teclado);
    glutSpecialFunc(teclas_especiales);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
