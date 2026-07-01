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

#define PASO_TX   8.0f
#define PASO_TY   8.0f
#define PASO_VEH  14.0f

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

static void cb_teclado(GLFWwindow *win, int key, int scancode, int action, int mods)
{
    (void)win; (void)scancode; (void)mods;
    if (action == GLFW_RELEASE) return;

    switch (key) {

    /* ---- Control global ---- */
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(win, GLFW_TRUE);
        break;
    case GLFW_KEY_SPACE:
        if (action == GLFW_PRESS) ESTADO.pausa = !ESTADO.pausa;
        break;

    /* ---- Seleccion de objeto ---- */
    case GLFW_KEY_1: ESTADO.objeto_sel = OBJ_VEHICULO_ROJO; break;
    case GLFW_KEY_2: ESTADO.objeto_sel = OBJ_BUS_AZUL;      break;
    case GLFW_KEY_3: ESTADO.objeto_sel = OBJ_ED_NARANJA;    break;
    case GLFW_KEY_4: ESTADO.objeto_sel = OBJ_ED_AZUL;       break;
    case GLFW_KEY_TAB:
        if (action == GLFW_PRESS)
            ESTADO.objeto_sel = (ESTADO.objeto_sel + 1) % 4;
        break;

    /* ---- Reset del objeto activo ---- */
    case GLFW_KEY_0:
        if (action == GLFW_PRESS) {
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
        }
        break;

    /* ---- Escala (E = agrandar, S = achicar) ---- */
    case GLFW_KEY_E:
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_escala += 0.05f;
            if (ESTADO.ed_naranja_escala > 3.0f) ESTADO.ed_naranja_escala = 3.0f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_escala += 0.05f;
            if (ESTADO.ed_azul_escala > 3.0f) ESTADO.ed_azul_escala = 3.0f;
        }
        break;
    case GLFW_KEY_S:
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_escala -= 0.05f;
            if (ESTADO.ed_naranja_escala < 0.1f) ESTADO.ed_naranja_escala = 0.1f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_escala -= 0.05f;
            if (ESTADO.ed_azul_escala < 0.1f) ESTADO.ed_azul_escala = 0.1f;
        }
        break;

    /* ---- Rotacion (Q = antihoraria, W = horaria) ---- */
    case GLFW_KEY_Q:
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA)
            ESTADO.ed_naranja_angulo += 0.05f;
        else if (ESTADO.objeto_sel == OBJ_ED_AZUL)
            ESTADO.ed_azul_angulo += 0.05f;
        break;
    case GLFW_KEY_W:
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA)
            ESTADO.ed_naranja_angulo -= 0.05f;
        else if (ESTADO.objeto_sel == OBJ_ED_AZUL)
            ESTADO.ed_azul_angulo -= 0.05f;
        break;

    /* ---- Distorsion shear (D = mas, F = menos) ---- */
    case GLFW_KEY_D:
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_shear += 0.04f;
            if (ESTADO.ed_naranja_shear > 1.5f) ESTADO.ed_naranja_shear = 1.5f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_shear += 0.04f;
            if (ESTADO.ed_azul_shear > 1.5f) ESTADO.ed_azul_shear = 1.5f;
        }
        break;
    case GLFW_KEY_F:
        if (ESTADO.objeto_sel == OBJ_ED_NARANJA) {
            ESTADO.ed_naranja_shear -= 0.04f;
            if (ESTADO.ed_naranja_shear < -1.5f) ESTADO.ed_naranja_shear = -1.5f;
        } else if (ESTADO.objeto_sel == OBJ_ED_AZUL) {
            ESTADO.ed_azul_shear -= 0.04f;
            if (ESTADO.ed_azul_shear < -1.5f) ESTADO.ed_azul_shear = -1.5f;
        }
        break;

    /* ---- Reflexion (R = alternar) ---- */
    case GLFW_KEY_R:
        if (action == GLFW_PRESS) {
            if (ESTADO.objeto_sel == OBJ_ED_NARANJA)
                ESTADO.ed_naranja_reflejo = !ESTADO.ed_naranja_reflejo;
            else if (ESTADO.objeto_sel == OBJ_ED_AZUL)
                ESTADO.ed_azul_reflejo = !ESTADO.ed_azul_reflejo;
        }
        break;

    /* ---- Traslacion con flechas ---- */
    case GLFW_KEY_LEFT:
        switch (ESTADO.objeto_sel) {
        case OBJ_VEHICULO_ROJO: ESTADO.vehiculo_tx   -= PASO_VEH; break;
        case OBJ_BUS_AZUL:     ESTADO.bus_tx         -= PASO_VEH; break;
        case OBJ_ED_NARANJA:   ESTADO.ed_naranja_tx  -= PASO_TX;  break;
        case OBJ_ED_AZUL:      ESTADO.ed_azul_tx     -= PASO_TX;  break;
        }
        break;
    case GLFW_KEY_RIGHT:
        switch (ESTADO.objeto_sel) {
        case OBJ_VEHICULO_ROJO: ESTADO.vehiculo_tx   += PASO_VEH; break;
        case OBJ_BUS_AZUL:     ESTADO.bus_tx         += PASO_VEH; break;
        case OBJ_ED_NARANJA:   ESTADO.ed_naranja_tx  += PASO_TX;  break;
        case OBJ_ED_AZUL:      ESTADO.ed_azul_tx     += PASO_TX;  break;
        }
        break;
    case GLFW_KEY_UP:
        switch (ESTADO.objeto_sel) {
        case OBJ_ED_NARANJA: ESTADO.ed_naranja_ty += PASO_TY; break;
        case OBJ_ED_AZUL:   ESTADO.ed_azul_ty    += PASO_TY; break;
        default: break;
        }
        break;
    case GLFW_KEY_DOWN:
        switch (ESTADO.objeto_sel) {
        case OBJ_ED_NARANJA: ESTADO.ed_naranja_ty -= PASO_TY; break;
        case OBJ_ED_AZUL:   ESTADO.ed_azul_ty    -= PASO_TY; break;
        default: break;
        }
        break;
    }
}

static void actualizar(void)
{
    if (ESTADO.pausa) return;

    /* TRASLACION vehiculo rojo */
    ESTADO.vehiculo_tx += 1.8f;
    if (ESTADO.vehiculo_tx > 1050.0f)
        ESTADO.vehiculo_tx = -300.0f;

    /* ROTACION ruedas vehiculo rojo */
    ESTADO.angulo_rueda -= 0.07f;
    if (ESTADO.angulo_rueda < -6.2832f)
        ESTADO.angulo_rueda += 6.2832f;

    /* TRASLACION bus azul (carril contrario) */
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

    /* DISTORSION (SHEAR) senal de transito */
    ESTADO.shear_senal += 0.003f * shear_dir;
    if (ESTADO.shear_senal >=  0.40f) shear_dir = -1.0f;
    if (ESTADO.shear_senal <= -0.40f) shear_dir =  1.0f;

    /* SEMAFORO: ciclo rojo(3s) -> amarillo(1.5s) -> verde(3s) */
    if (--ESTADO.semaforo_ticks <= 0) {
        ESTADO.semaforo_fase = (ESTADO.semaforo_fase + 1) % 3;
        switch (ESTADO.semaforo_fase) {
        case 0: ESTADO.semaforo_ticks = 180; break; /* rojo:     3 s */
        case 1: ESTADO.semaforo_ticks =  90; break; /* amarillo: 1.5 s */
        case 2: ESTADO.semaforo_ticks = 180; break; /* verde:    3 s */
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
    glfwSwapInterval(1);   /* VSync: limita a frecuencia del monitor */

    glewInit();

    glfwSetFramebufferSizeCallback(ventana, cb_tamano);
    glfwSetKeyCallback(ventana, cb_teclado);

    glClearColor(0.09f, 0.12f, 0.16f, 1.0f);
    glPointSize(1.0f);
    setup_proyeccion(ANCHO_VENTANA, ALTO_VENTANA);

    /* Bucle principal con paso de simulacion fijo a 60 fps */
    const double PASO_SIM = 1.0 / 60.0;
    double acumulador = 0.0;
    double t_ant = glfwGetTime();

    while (!glfwWindowShouldClose(ventana)) {
        double ahora = glfwGetTime();
        acumulador += ahora - t_ant;
        t_ant = ahora;

        while (acumulador >= PASO_SIM) {
            actualizar();
            acumulador -= PASO_SIM;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        dibujar_escena_animada(&ESTADO);
        glfwSwapBuffers(ventana);
        glfwPollEvents();
    }

    glfwDestroyWindow(ventana);
    glfwTerminate();
    return 0;
}
