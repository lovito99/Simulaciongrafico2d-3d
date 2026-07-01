#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdlib.h>

#include "render_escena.h"
#include "tipos.h"

/* Estado global de la animacion */
EstadoAnim ESTADO = {
    .vehiculo_tx  = 0.0f,
    .angulo_rueda = 0.0f,
    .nube_tx      = 500.0f,
    .shear_senal  = 0.0f,
    .pausa        = 0,

    .objeto_sel   = OBJ_VEHICULO_ROJO,

    .bus_tx       = 960.0f,
    .bus_angulo   = 0.0f,

    .ed_naranja_tx      = 0.0f,
    .ed_naranja_ty      = 0.0f,
    .ed_naranja_escala  = 1.0f,
    .ed_naranja_angulo  = 0.0f,
    .ed_naranja_shear   = 0.0f,
    .ed_naranja_reflejo = 0,

    .ed_azul_tx      = 0.0f,
    .ed_azul_ty      = 0.0f,
    .ed_azul_escala  = 1.0f,
    .ed_azul_angulo  = 0.0f,
    .ed_azul_shear   = 0.0f,
    .ed_azul_reflejo = 0,

    .semaforo_fase  = 0,
    .semaforo_ticks = 180,
};

static float shear_dir = 1.0f;

/* Pasos por fotograma (60 fps) para teclas mantenidas */
#define PASO_TX    1.8f
#define PASO_TY    1.8f
#define PASO_VEH   2.5f
#define PASO_ESC   0.012f
#define PASO_ANG   0.012f
#define PASO_SH    0.010f

static void setup_proyeccion(int ancho, int alto)
{
    glViewport(0, 0, ancho, alto);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, ANCHO_VENTANA, 0.0, ALTO_VENTANA, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void cb_tamano(GLFWwindow *win, int ancho, int alto)
{
    (void)win;
    setup_proyeccion(ancho, alto);
}

/* -------------------------------------------------------------------------
   Teclas de disparo unico (toggle): se gestionan en el callback
   ------------------------------------------------------------------------- */
static void cb_teclado(GLFWwindow *win, int key, int scancode, int action, int mods)
{
    (void)scancode; (void)mods;
    if (action != GLFW_PRESS) return;   /* solo primer pulsado */

    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(win, GLFW_TRUE);
        break;
    case GLFW_KEY_SPACE:
        ESTADO.pausa = !ESTADO.pausa;
        break;

    /* Seleccion de objeto */
    case GLFW_KEY_1: ESTADO.objeto_sel = OBJ_VEHICULO_ROJO; break;
    case GLFW_KEY_2: ESTADO.objeto_sel = OBJ_BUS_AZUL;      break;
    case GLFW_KEY_3: ESTADO.objeto_sel = OBJ_ED_NARANJA;    break;
    case GLFW_KEY_4: ESTADO.objeto_sel = OBJ_ED_AZUL;       break;
    case GLFW_KEY_TAB:
        ESTADO.objeto_sel = (ESTADO.objeto_sel + 1) % 4;
        break;

    /* Reflexion: toggle */
    case GLFW_KEY_R:
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA)
            ESTADO.ed_naranja_reflejo = !ESTADO.ed_naranja_reflejo;
        else if (ESTADO.objeto_sel == OBJ_ED_AZUL)
            ESTADO.ed_azul_reflejo = !ESTADO.ed_azul_reflejo;
        break;

    /* Reset */
    case GLFW_KEY_0:
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
    }
}

/* -------------------------------------------------------------------------
   Teclas mantenidas: se leen con glfwGetKey() cada fotograma
   (no dependen del retardo de repeticion del sistema operativo)
   ------------------------------------------------------------------------- */
static void procesar_teclas_continuas(GLFWwindow *win)
{
#define PULSADA(k) (glfwGetKey(win, (k)) == GLFW_PRESS)

    /* --- Traslacion con flechas --- */
    if (PULSADA(GLFW_KEY_LEFT)) {
        switch (ESTADO.objeto_sel) {
        case OBJ_VEHICULO_ROJO: ESTADO.vehiculo_tx  -= PASO_VEH; break;
        case OBJ_BUS_AZUL:     ESTADO.bus_tx        -= PASO_VEH; break;
        case OBJ_ED_NARANJA:   ESTADO.ed_naranja_tx -= PASO_TX;  break;
        case OBJ_ED_AZUL:      ESTADO.ed_azul_tx    -= PASO_TX;  break;
        }
    }
    if (PULSADA(GLFW_KEY_RIGHT)) {
        switch (ESTADO.objeto_sel) {
        case OBJ_VEHICULO_ROJO: ESTADO.vehiculo_tx  += PASO_VEH; break;
        case OBJ_BUS_AZUL:     ESTADO.bus_tx        += PASO_VEH; break;
        case OBJ_ED_NARANJA:   ESTADO.ed_naranja_tx += PASO_TX;  break;
        case OBJ_ED_AZUL:      ESTADO.ed_azul_tx    += PASO_TX;  break;
        }
    }
    if (PULSADA(GLFW_KEY_UP)) {
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) ESTADO.ed_naranja_ty += PASO_TY;
        else if (ESTADO.objeto_sel == OBJ_ED_AZUL) ESTADO.ed_azul_ty  += PASO_TY;
    }
    if (PULSADA(GLFW_KEY_DOWN)) {
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) ESTADO.ed_naranja_ty -= PASO_TY;
        else if (ESTADO.objeto_sel == OBJ_ED_AZUL) ESTADO.ed_azul_ty  -= PASO_TY;
    }

    /* --- Escala: E agrandar, S achicar --- */
    if (PULSADA(GLFW_KEY_E)) {
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_escala += PASO_ESC;
            if (ESTADO.ed_naranja_escala > 3.0f) ESTADO.ed_naranja_escala = 3.0f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_escala += PASO_ESC;
            if (ESTADO.ed_azul_escala > 3.0f) ESTADO.ed_azul_escala = 3.0f;
        }
    }
    if (PULSADA(GLFW_KEY_S)) {
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_escala -= PASO_ESC;
            if (ESTADO.ed_naranja_escala < 0.1f) ESTADO.ed_naranja_escala = 0.1f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_escala -= PASO_ESC;
            if (ESTADO.ed_azul_escala < 0.1f) ESTADO.ed_azul_escala = 0.1f;
        }
    }

    /* --- Rotacion: Q antihoraria, W horaria --- */
    if (PULSADA(GLFW_KEY_Q)) {
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) ESTADO.ed_naranja_angulo += PASO_ANG;
        else if (ESTADO.objeto_sel == OBJ_ED_AZUL) ESTADO.ed_azul_angulo  += PASO_ANG;
    }
    if (PULSADA(GLFW_KEY_W)) {
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) ESTADO.ed_naranja_angulo -= PASO_ANG;
        else if (ESTADO.objeto_sel == OBJ_ED_AZUL) ESTADO.ed_azul_angulo  -= PASO_ANG;
    }

    /* --- Shear: D mas, F menos --- */
    if (PULSADA(GLFW_KEY_D)) {
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_shear += PASO_SH;
            if (ESTADO.ed_naranja_shear > 1.5f) ESTADO.ed_naranja_shear = 1.5f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_shear += PASO_SH;
            if (ESTADO.ed_azul_shear > 1.5f) ESTADO.ed_azul_shear = 1.5f;
        }
    }
    if (PULSADA(GLFW_KEY_F)) {
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_shear -= PASO_SH;
            if (ESTADO.ed_naranja_shear < -1.5f) ESTADO.ed_naranja_shear = -1.5f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_shear -= PASO_SH;
            if (ESTADO.ed_azul_shear < -1.5f) ESTADO.ed_azul_shear = -1.5f;
        }
    }

#undef PULSADA
}

static void actualizar(GLFWwindow *win)
{
    /* Teclas mantenidas: siempre activas (incluso en pausa) */
    procesar_teclas_continuas(win);

    if (ESTADO.pausa) return;

    /* TRASLACION vehiculo rojo */
    ESTADO.vehiculo_tx += 1.8f;
    if (ESTADO.vehiculo_tx > 1050.0f)
        ESTADO.vehiculo_tx = -300.0f;

    /* ROTACION ruedas vehiculo rojo */
    ESTADO.angulo_rueda -= 0.07f;
    if (ESTADO.angulo_rueda < -6.2832f)
        ESTADO.angulo_rueda += 6.2832f;

    /* TRASLACION bus azul */
    ESTADO.bus_tx -= 1.5f;
    if (ESTADO.bus_tx < -260.0f)
        ESTADO.bus_tx = 960.0f;

    /* ROTACION ruedas bus */
    ESTADO.bus_angulo += 0.07f;
    if (ESTADO.bus_angulo > 6.2832f)
        ESTADO.bus_angulo -= 6.2832f;

    /* TRASLACION nube */
    ESTADO.nube_tx -= 0.5f;
    if (ESTADO.nube_tx < -130.0f)
        ESTADO.nube_tx = 960.0f;

    /* SHEAR senal de transito */
    ESTADO.shear_senal += 0.003f * shear_dir;
    if (ESTADO.shear_senal >=  0.40f) shear_dir = -1.0f;
    if (ESTADO.shear_senal <= -0.40f) shear_dir =  1.0f;

    /* SEMAFORO: ciclo rojo(3s) -> amarillo(1.5s) -> verde(3s) */
    if (--ESTADO.semaforo_ticks <= 0) {
        ESTADO.semaforo_fase = (ESTADO.semaforo_fase + 1) % 3;
        switch (ESTADO.semaforo_fase) {
        case 0: ESTADO.semaforo_ticks = 180; break;
        case 1: ESTADO.semaforo_ticks =  90; break;
        case 2: ESTADO.semaforo_ticks = 180; break;
        }
    }
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    GLFWwindow *ventana = glfwCreateWindow(
        ANCHO_VENTANA, ALTO_VENTANA,
        "Entrega 2 - Transformaciones Geometricas 2D",
        NULL, NULL);
    if (!ventana) { glfwTerminate(); return 1; }

    glfwMakeContextCurrent(ventana);
    glfwSwapInterval(1);

    glewInit();

    glfwSetFramebufferSizeCallback(ventana, cb_tamano);
    glfwSetKeyCallback(ventana, cb_teclado);

    glClearColor(0.09f, 0.12f, 0.16f, 1.0f);
    glPointSize(1.0f);
    setup_proyeccion(ANCHO_VENTANA, ALTO_VENTANA);

    const double PASO_SIM = 1.0 / 60.0;
    double acumulador = 0.0;
    double t_ant = glfwGetTime();

    while (!glfwWindowShouldClose(ventana)) {
        glfwPollEvents();   /* primero: procesar eventos y callbacks */

        double ahora = glfwGetTime();
        acumulador += ahora - t_ant;
        t_ant = ahora;

        while (acumulador >= PASO_SIM) {
            actualizar(ventana);   /* incluye lectura de teclas mantenidas */
            acumulador -= PASO_SIM;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        dibujar_escena_animada(&ESTADO);
        glfwSwapBuffers(ventana);
    }

    glfwDestroyWindow(ventana);
    glfwTerminate();
    return 0;
}
