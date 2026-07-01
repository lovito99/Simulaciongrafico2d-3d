#include "render_escena.h"

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "algoritmos.h"
#include "escena.h"
#include "transformaciones.h"

/* =============================================================================
   NUBE ANIMADA (poligono base centrado en el origen)
   ============================================================================= */

static const Poligono NUBE_BASE = {
    .puntos = {
        {-55,-10}, {-45, 6}, {-20,16}, {0,20},
        {20,16},   {45, 6},  {55,-10}, {35,-20}, {-35,-20}
    },
    .cantidad = 9
};
static const Color COLOR_NUBE   = {228, 238, 255};
static const Color COLOR_NUBE_2 = {200, 215, 245};

/* =============================================================================
   HELPERS DE INTERFAZ (dibujan fuera de VENTANA_RECORTE con GL directo)
   ============================================================================= */

static void panel_rect(float x, float y, float w, float h, Color c)
{
    glColor3ub(c.r, c.g, c.b);
    glBegin(GL_QUADS);
    glVertex2f(x,   y);
    glVertex2f(x+w, y);
    glVertex2f(x+w, y+h);
    glVertex2f(x,   y+h);
    glEnd();
}

static void panel_borde(float x, float y, float w, float h, Color c)
{
    glColor3ub(c.r, c.g, c.b);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x,   y);
    glVertex2f(x+w, y);
    glVertex2f(x+w, y+h);
    glVertex2f(x,   y+h);
    glEnd();
}

/* =============================================================================
   MATRIZ COMPUESTA PARA EDIFICIOS
   Orden de aplicacion al punto (derecha a izquierda):
     T(-cx,-cy)  -> SH  -> S  -> R  -> [Ref]  -> T(cx,cy)  -> T(tx,ty)
   ============================================================================= */

static Mat3 mat_edificio(
    float tx, float ty,
    float ang, float esc, float sh,
    int ref,
    float cx, float cy)
{
    Mat3 M = mat3_traslacion(-cx, -cy);
    M = mat3_mul(mat3_shear_x(sh),        M);
    M = mat3_mul(mat3_escala(esc, esc),   M);
    M = mat3_mul(mat3_rotacion(ang),      M);
    if (ref) M = mat3_mul(mat3_escala(-1.0f, 1.0f), M);
    M = mat3_mul(mat3_traslacion(cx, cy), M);
    M = mat3_mul(mat3_traslacion(tx, ty), M);
    return M;
}

/* =============================================================================
   EDIFICIO CON TRANSFORMACION MATRICIAL
   ============================================================================= */

static void dibujar_edificio_t(const Edificio *ed, Mat3 M)
{
    /* Cuerpo */
    Poligono cuerpo_p = rectangulo_a_poligono(ed->cuerpo);
    Poligono cuerpo_t = mat3_poligono(M, &cuerpo_p);
    dibujar_poligono_relleno(&cuerpo_t, ed->relleno, ed->contorno, VENTANA_RECORTE);

    /* Ventanas */
    const int mg_h = 16, mg_v = 22, sep_h = 14, sep_v = 18;
    const int aw = ed->cuerpo.ancho - 2*mg_h - (ed->columnas_ventanas-1)*sep_h;
    const int ah = ed->cuerpo.alto  - 2*mg_v - (ed->filas_ventanas-1)*sep_v;
    const int vw = aw / ed->columnas_ventanas;
    const int vh = ah / ed->filas_ventanas;

    for (int f = 0; f < ed->filas_ventanas; ++f)
        for (int cv = 0; cv < ed->columnas_ventanas; ++cv) {
            Rectangulo ven = {
                ed->cuerpo.x + mg_h + cv*(vw+sep_h),
                ed->cuerpo.y + mg_v + f*(vh+sep_v),
                vw, vh
            };
            Poligono vp = rectangulo_a_poligono(ven);
            Poligono vt = mat3_poligono(M, &vp);
            dibujar_poligono_relleno(&vt, ed->ventanas, ed->contorno, VENTANA_RECORTE);
        }
}

/* =============================================================================
   EDIFICIO ESTATICO (sin transformacion - edificio morado)
   ============================================================================= */

static void dibujar_edificio(const Edificio *edificio)
{
    dibujar_edificio_t(edificio, mat3_identidad());
}

/* =============================================================================
   BUS AZUL CON TRANSFORMACION MATRICIAL
   ============================================================================= */

static void dibujar_bus_t(const Vehiculo *v, Mat3 M, float ang)
{
    /* Cuerpo principal */
    Poligono cuerpo_p = rectangulo_a_poligono(v->cuerpo);
    Poligono cuerpo_t = mat3_poligono(M, &cuerpo_p);
    dibujar_poligono_relleno(&cuerpo_t, v->color_cuerpo, v->contorno, VENTANA_RECORTE);

    /* Frente / parabrisas (lado izquierdo: frente del bus hacia la izquierda) */
    Poligono cabina_t = mat3_poligono(M, &v->cabina);
    dibujar_poligono_relleno(&cabina_t, v->color_cabina, v->contorno, VENTANA_RECORTE);

    /* Ventanas laterales: 5 ventanas a lo largo del cuerpo */
    {
        const Color c_vent     = {185, 220, 255};
        const Color c_cont_ven = {18,  55,  115};
        const int bx0 = v->cuerpo.x + 52;
        const int by  = v->cuerpo.y + 6;
        const int bw  = 28, bh = 16, bgap = 7;
        for (int i = 0; i < 5; i++) {
            Rectangulo ven = {bx0 + i*(bw+bgap), by, bw, bh};
            Poligono vp = rectangulo_a_poligono(ven);
            Poligono vt = mat3_poligono(M, &vp);
            dibujar_poligono_relleno(&vt, c_vent, c_cont_ven, VENTANA_RECORTE);
        }
    }

    /* Ruedas con espolones (muestran rotacion) */
    for (int i = 0; i < 2; i++) {
        Punto centro = mat3_punto(M, v->ruedas[i]);
        dibujar_circulo_punto_medio(centro, v->radio_rueda, v->color_rueda, VENTANA_RECORTE);
        dibujar_circulo_punto_medio(centro, v->radio_rueda / 2, (Color){55,55,55}, VENTANA_RECORTE);

        const Color c_espolon = {180, 180, 180};
        for (int j = 0; j < 2; j++) {
            float a = ang + (float)j * 1.5708f;
            Punto tip = {
                centro.x + (int)roundf((float)v->radio_rueda * cosf(a)),
                centro.y + (int)roundf((float)v->radio_rueda * sinf(a))
            };
            dibujar_linea_bresenham(centro, tip, c_espolon, VENTANA_RECORTE);
        }
    }
}

/* =============================================================================
   VEHICULO ROJO CON TRANSFORMACION MATRICIAL
   ============================================================================= */

static void dibujar_vehiculo_t(const Vehiculo *v, Mat3 M, float ang_rueda)
{
    Poligono cuerpo_p = rectangulo_a_poligono(v->cuerpo);
    Poligono cuerpo_t = mat3_poligono(M, &cuerpo_p);
    dibujar_poligono_relleno(&cuerpo_t, v->color_cuerpo, v->contorno, VENTANA_RECORTE);

    Poligono cabina_t = mat3_poligono(M, &v->cabina);
    dibujar_poligono_relleno(&cabina_t, v->color_cabina, v->contorno, VENTANA_RECORTE);

    Punto sep0 = mat3_punto(M, (Punto){v->cuerpo.x + 82, v->cuerpo.y});
    Punto sep1 = mat3_punto(M, (Punto){v->cuerpo.x + 82, v->cuerpo.y + v->cuerpo.alto});
    dibujar_linea_bresenham(sep0, sep1, v->contorno, VENTANA_RECORTE);

    for (int i = 0; i < 2; i++) {
        Punto centro = mat3_punto(M, v->ruedas[i]);
        dibujar_circulo_punto_medio(centro, v->radio_rueda, v->color_rueda, VENTANA_RECORTE);
        dibujar_circulo_punto_medio(centro, v->radio_rueda / 2, (Color){60,60,60}, VENTANA_RECORTE);

        const Color c_espolon = {190, 190, 190};
        for (int j = 0; j < 2; j++) {
            float a = ang_rueda + (float)j * 1.5708f;
            Punto tip = {
                centro.x + (int)roundf((float)v->radio_rueda * cosf(a)),
                centro.y + (int)roundf((float)v->radio_rueda * sinf(a))
            };
            dibujar_linea_bresenham(centro, tip, c_espolon, VENTANA_RECORTE);
        }
    }
}

/* REFLEXION del vehiculo rojo en el piso humedo */
static void dibujar_vehiculo_reflexion(const Vehiculo *v, float tx, float y_pivot)
{
    Mat3 M_tras = mat3_traslacion(tx, 0.0f);
    Mat3 M_ref  = mat3_reflexion_eje_x(y_pivot);
    Mat3 M      = mat3_mul(M_ref, M_tras);

    Vehiculo v_dim = *v;
    v_dim.color_cuerpo = (Color){
        (unsigned char)(v->color_cuerpo.r * 0.30f),
        (unsigned char)(v->color_cuerpo.g * 0.30f),
        (unsigned char)(v->color_cuerpo.b * 0.30f)
    };
    v_dim.color_cabina = (Color){
        (unsigned char)(v->color_cabina.r * 0.30f),
        (unsigned char)(v->color_cabina.g * 0.30f),
        (unsigned char)(v->color_cabina.b * 0.30f)
    };
    v_dim.contorno = (Color){40, 10, 10};

    Poligono cuerpo_p = rectangulo_a_poligono(v->cuerpo);
    Poligono cuerpo_t = mat3_poligono(M, &cuerpo_p);
    dibujar_poligono_relleno(&cuerpo_t, v_dim.color_cuerpo, v_dim.contorno, VENTANA_RECORTE);

    Poligono cabina_t = mat3_poligono(M, &v->cabina);
    dibujar_poligono_relleno(&cabina_t, v_dim.color_cabina, v_dim.contorno, VENTANA_RECORTE);

    for (int i = 0; i < 2; i++) {
        Punto c = mat3_punto(M, v->ruedas[i]);
        dibujar_circulo_punto_medio(c, v->radio_rueda, (Color){10,10,10}, VENTANA_RECORTE);
    }
}

/* =============================================================================
   SENAL DE TRANSITO CON DISTORSION (SHEAR)
   ============================================================================= */

static void dibujar_senal_t(const SenalTransito *senal, float shx)
{
    const float cx = 769.0f, cy = 239.0f;

    const Color c_ref = {80, 20, 20};
    dibujar_contorno_poligono(&senal->forma, c_ref, VENTANA_RECORTE);

    Mat3 M = mat3_shear_x_pivote(shx, cx, cy);
    Poligono forma_t = mat3_poligono(M, &senal->forma);
    dibujar_poligono_relleno(&forma_t, senal->color_senal, senal->contorno, VENTANA_RECORTE);

    dibujar_linea_bresenham(senal->poste.inicio, senal->poste.fin,
                            senal->color_poste, VENTANA_RECORTE);
}

/* =============================================================================
   ARBOLES (estaticos)
   ============================================================================= */

static void dibujar_arbol_t(const Arbol *arbol, Mat3 M)
{
    Poligono tronco_p = rectangulo_a_poligono(arbol->tronco);
    Poligono tronco_t = mat3_poligono(M, &tronco_p);
    Poligono copa_t   = mat3_poligono(M, &arbol->copa);

    dibujar_poligono_relleno(&tronco_t, arbol->color_tronco, arbol->contorno, VENTANA_RECORTE);
    dibujar_poligono_relleno(&copa_t,   arbol->color_copa,   arbol->contorno, VENTANA_RECORTE);
}

/* =============================================================================
   ELEMENTOS DE ESCENA ESTATICOS
   ============================================================================= */

static void dibujar_casa(const Casa *casa)
{
    const Rectangulo puerta   = {casa->cuerpo.x + 54, casa->cuerpo.y, 25, 58};
    const Rectangulo vent_izq = {casa->cuerpo.x + 18, casa->cuerpo.y + 46, 28, 26};
    const Rectangulo vent_der = {casa->cuerpo.x + 88, casa->cuerpo.y + 46, 28, 26};
    const Color c_puerta = {129, 84, 51};
    const Color c_vent   = {186, 228, 247};

    dibujar_rectangulo(casa->cuerpo, casa->color_cuerpo, casa->contorno, VENTANA_RECORTE);
    dibujar_poligono_relleno(&casa->techo, casa->color_techo, casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(puerta,   c_puerta, casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(vent_izq, c_vent,   casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(vent_der, c_vent,   casa->contorno, VENTANA_RECORTE);
}

static void dibujar_carretera(const Carretera *carretera)
{
    dibujar_rectangulo(carretera->cuerpo, carretera->relleno, carretera->contorno, VENTANA_RECORTE);
    for (int i = 0; i < carretera->cantidad_divisores; ++i)
        dibujar_linea_bresenham(carretera->divisores[i].inicio,
                                carretera->divisores[i].fin,
                                carretera->color_divisor, VENTANA_RECORTE);
}

static void dibujar_semaforo(const Semaforo *semaforo)
{
    dibujar_rectangulo(semaforo->poste, semaforo->color_poste, semaforo->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(semaforo->caja,  semaforo->color_caja,  semaforo->contorno, VENTANA_RECORTE);
    for (int i = 0; i < 3; ++i)
        dibujar_circulo_punto_medio(semaforo->luces[i], semaforo->radio,
                                    semaforo->colores_luces[i], VENTANA_RECORTE);
}

static void dibujar_sol(const Sol *sol)
{
    for (int i = 0; i < sol->cantidad_rayos; ++i)
        dibujar_linea_bresenham(sol->rayos[i].inicio, sol->rayos[i].fin,
                                sol->color, VENTANA_RECORTE);
    dibujar_circulo_punto_medio(sol->centro, sol->radio, sol->color, VENTANA_RECORTE);
}

static void dibujar_nube(float tx, float ty, const Color *color, float escala)
{
    Mat3 M = mat3_mul(mat3_traslacion(tx, ty), mat3_escala(escala, escala));
    Poligono nube_t = mat3_poligono(M, &NUBE_BASE);
    dibujar_poligono_relleno(&nube_t, *color, *color, VENTANA_RECORTE);
}

static void dibujar_borde_ventana(void)
{
    const Color borde = {245, 245, 245};
    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymin},
                            (Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymin}, borde, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymin},
                            (Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymax}, borde, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymax},
                            (Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymax}, borde, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymax},
                            (Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymin}, borde, VENTANA_RECORTE);
}

/* =============================================================================
   ETIQUETAS DE TRANSFORMACION EN LA ESCENA (espanol)
   ============================================================================= */

static void dibujar_etiquetas(const EstadoAnim *e)
{
    const Color amarillo = {250, 220,  60};
    const Color cian     = { 80, 220, 210};
    const Color rosa     = {240, 100, 200};
    const Color naranja  = {240, 160,  50};
    const Color verde    = {100, 230, 120};
    const Color lila     = {200, 140, 255};

    /* Vehiculo rojo: traslacion + rotacion + composicion */
    dibujar_texto(250.0f, 165.0f, "TRASLACION + ROTACION + COMPOSICION", amarillo);

    /* Bus azul */
    dibujar_texto(300.0f, 192.0f, "BUS AZUL: TRASLACION (carril contrario)", cian);

    /* Senal: distorsion shear automatica */
    dibujar_texto(690.0f, 285.0f, "SHEAR", rosa);

    /* Nube: traslacion */
    dibujar_texto(340.0f, 560.0f, "TRASLACION (nube)", naranja);

    /* Reflexion del vehiculo en el piso */
    dibujar_texto(65.0f, 72.0f, "REFLEXION EJE-X (piso)", (Color){200,80,80});

    /* Edificio naranja: mostrar transformaciones activas */
    {
        char buf[64] = "ESCALA";
        int activos = 0;
        if (e->ed_naranja_angulo != 0.0f)  { if (activos++) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + ROT"); else snprintf(buf, sizeof(buf), "ROT"); activos=1; }
        if (e->ed_naranja_shear  != 0.0f)  { if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + SHEAR"); else { snprintf(buf, sizeof(buf), "SHEAR"); activos=1; } }
        if (e->ed_naranja_reflejo)          { if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + REFLEJO"); else { snprintf(buf, sizeof(buf), "REFLEJO"); activos=1; } }
        if (e->ed_naranja_tx != 0.0f || e->ed_naranja_ty != 0.0f) {
            if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + TRASL"); else { snprintf(buf, sizeof(buf), "TRASLACION"); activos=1; }
        }
        if (activos > 1) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " [COMPOSICION]");
        (void)activos;
        dibujar_texto(675.0f, 510.0f, buf, verde);
    }

    /* Edificio azul: mostrar transformaciones activas */
    {
        char buf[64] = "";
        int activos = 0;
        if (e->ed_azul_escala != 1.0f) { snprintf(buf, sizeof(buf), "ESCALA"); activos++; }
        if (e->ed_azul_angulo != 0.0f) { if (activos++) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + ROT"); else { snprintf(buf, sizeof(buf), "ROT"); activos=1; } }
        if (e->ed_azul_shear  != 0.0f) { if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + SHEAR"); else { snprintf(buf, sizeof(buf), "SHEAR"); activos=1; } }
        if (e->ed_azul_reflejo)         { if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + REFLEJO"); else { snprintf(buf, sizeof(buf), "REFLEJO"); activos=1; } }
        if (e->ed_azul_tx != 0.0f || e->ed_azul_ty != 0.0f) {
            if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + TRASL"); else { snprintf(buf, sizeof(buf), "TRASLACION"); activos=1; }
        }
        if (activos > 1) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " [COMPOSICION]");
        if (activos > 0)
            dibujar_texto(65.0f, 510.0f, buf, lila);
    }
}

/* =============================================================================
   PANEL HUD: SELECTOR DE OBJETO + CONTROLES (espanol)
   ============================================================================= */

static void dibujar_panel_hud(const EstadoAnim *e)
{
    /* --- Colores del panel --- */
    const Color fondo_panel  = {18,  22,  38};
    const Color blanco       = {255, 255, 255};
    const Color gris_claro   = {180, 185, 200};
    const Color amarillo_hud = {245, 215,  60};
    const Color verde_hud    = { 80, 210, 120};
    const Color rojo_hud     = {220,  80,  80};

    /* Colores de cada objeto (sel = activo, unsel = inactivo) */
    const Color tab_col_sel[4]   = {{110,25,25},  {20,60,130},  {120,65,18},  {28,48,115}};
    const Color tab_col_unsel[4] = {{ 50,12,12},  {12,32, 70},  { 58,30, 8},  {14,24, 58}};
    const Color tab_texto_sel[4] = {{255,120,120},{120,190,255},{255,180,80}, {130,160,255}};
    const char *tab_nombre[4]    = {
        "1. VEHICULO ROJO",
        "2. BUS AZUL",
        "3. ED. NARANJA",
        "4. ED. AZUL"
    };

    /* ===== PANEL SUPERIOR (y=621..700) ===== */
    panel_rect(0.0f, 621.0f, 900.0f, 79.0f, fondo_panel);

    /* Titulo */
    dibujar_texto(65.0f, 682.0f, "ESCENA URBANA 2D  -  ENTREGA 2: TRANSFORMACIONES GEOMETRICAS", blanco);

    /* Estado pausa / ESC */
    if (e->pausa)
        dibujar_texto(700.0f, 682.0f, "[PAUSADO]", rojo_hud);
    else
        dibujar_texto(700.0f, 682.0f, "[ANIMANDO]", verde_hud);
    dibujar_texto(790.0f, 682.0f, "ESC=Salir", gris_claro);

    /* ---- Tabs de seleccion (y=650..672) ---- */
    const float tab_x0  = 65.0f;
    const float tab_w   = 190.0f;
    const float tab_gap = 5.0f;
    const float tab_y   = 649.0f;
    const float tab_h   = 23.0f;

    for (int i = 0; i < 4; i++) {
        float tx = tab_x0 + (float)i * (tab_w + tab_gap);
        Color bg  = (e->objeto_sel == i) ? tab_col_sel[i]   : tab_col_unsel[i];
        Color txt = (e->objeto_sel == i) ? tab_texto_sel[i] : gris_claro;
        panel_rect(tx, tab_y, tab_w, tab_h, bg);
        if (e->objeto_sel == i)
            panel_borde(tx, tab_y, tab_w, tab_h, tab_texto_sel[i]);
        dibujar_texto(tx + 6.0f, tab_y + 7.0f, tab_nombre[i], txt);
    }

    /* Instruccion de seleccion */
    dibujar_texto(65.0f, 636.0f, "TAB o teclas 1-4 para seleccionar objeto activo", gris_claro);

    /* ===== PANEL INFERIOR (y=0..59) ===== */
    panel_rect(0.0f, 0.0f, 900.0f, 59.0f, fondo_panel);

    /* Linea separadora superior del panel inferior */
    glColor3ub(40, 50, 80);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 59.0f);
    glVertex2f(900.0f, 59.0f);
    glEnd();

    /* Controles especificos segun objeto seleccionado */
    switch (e->objeto_sel) {

    case OBJ_VEHICULO_ROJO:
        dibujar_texto(65.0f, 44.0f, "VEHICULO ROJO  |  CONTROLES:", amarillo_hud);
        dibujar_texto(65.0f, 28.0f, "Flecha Izq/Der = Mover manualmente    ESPACIO = Pausar / Reanudar animacion", blanco);
        dibujar_texto(65.0f, 12.0f, "0 = Reiniciar posicion", gris_claro);
        break;

    case OBJ_BUS_AZUL:
        dibujar_texto(65.0f, 44.0f, "BUS AZUL  |  CONTROLES:", (Color){120,190,255});
        dibujar_texto(65.0f, 28.0f, "Flecha Izq/Der = Mover manualmente    ESPACIO = Pausar / Reanudar animacion", blanco);
        dibujar_texto(65.0f, 12.0f, "0 = Reiniciar posicion", gris_claro);
        break;

    case OBJ_ED_NARANJA: {
        dibujar_texto(65.0f, 44.0f, "EDIFICIO NARANJA  |  CONTROLES:", (Color){255,180,80});
        dibujar_texto(65.0f, 28.0f,
            "Flechas = Trasladar  |  E/S = Escala +/-  |  [/] = Rotar  |  D/F = Shear  |  R = Reflexion  |  0 = Reset",
            blanco);
        char vals[120];
        snprintf(vals, sizeof(vals),
            "Escala=%.2f  Angulo=%.2f  Shear=%.2f  Tx=%.0f  Ty=%.0f  Reflexion=%s",
            e->ed_naranja_escala, e->ed_naranja_angulo, e->ed_naranja_shear,
            e->ed_naranja_tx, e->ed_naranja_ty,
            e->ed_naranja_reflejo ? "SI" : "NO");
        dibujar_texto(65.0f, 12.0f, vals, gris_claro);
        break;
    }

    case OBJ_ED_AZUL: {
        dibujar_texto(65.0f, 44.0f, "EDIFICIO AZUL  |  CONTROLES:", (Color){130,160,255});
        dibujar_texto(65.0f, 28.0f,
            "Flechas = Trasladar  |  E/S = Escala +/-  |  [/] = Rotar  |  D/F = Shear  |  R = Reflexion  |  0 = Reset",
            blanco);
        char vals[120];
        snprintf(vals, sizeof(vals),
            "Escala=%.2f  Angulo=%.2f  Shear=%.2f  Tx=%.0f  Ty=%.0f  Reflexion=%s",
            e->ed_azul_escala, e->ed_azul_angulo, e->ed_azul_shear,
            e->ed_azul_tx, e->ed_azul_ty,
            e->ed_azul_reflejo ? "SI" : "NO");
        dibujar_texto(65.0f, 12.0f, vals, gris_claro);
        break;
    }
    }
}

/* =============================================================================
   FUNCION PRINCIPAL: dibujar_escena_animada
   ============================================================================= */

void dibujar_escena_animada(const EstadoAnim *estado)
{
    const Color c_cielo  = {121, 193, 244};
    const Color c_cesped = {121, 183,  96};

    /* --- Fondo --- */
    dibujar_rectangulo(ESCENA_URBANA.cielo,  c_cielo,  c_cielo,  VENTANA_RECORTE);
    dibujar_rectangulo(ESCENA_URBANA.cesped, c_cesped, c_cesped, VENTANA_RECORTE);
    dibujar_sol(&ESCENA_URBANA.sol);

    /* --- TRASLACION: nubes --- */
    dibujar_nube(estado->nube_tx, 575.0f, &COLOR_NUBE, 1.0f);
    {
        float tx2 = estado->nube_tx - 320.0f;
        if (tx2 < -130.0f) tx2 += 1090.0f;
        dibujar_nube(tx2, 540.0f, &COLOR_NUBE_2, 0.75f);
    }

    /* --- Edificio morado: estatico (no se transforma) --- */
    dibujar_edificio(&ESCENA_URBANA.edificios[1]);

    /* --- Edificio azul: TODAS las transformaciones manuales ---
     *   COMPOSICION: M = T(tx,ty) * T(cx,cy) * R * S * SH * [Ref] * T(-cx,-cy)  */
    {
        const float cx = 20.0f  + 170.0f / 2.0f;   /* 105 */
        const float cy = 140.0f + 350.0f / 2.0f;   /* 315 */
        Mat3 M = mat_edificio(
            estado->ed_azul_tx, estado->ed_azul_ty,
            estado->ed_azul_angulo, estado->ed_azul_escala,
            estado->ed_azul_shear, estado->ed_azul_reflejo,
            cx, cy);
        dibujar_edificio_t(&ESCENA_URBANA.edificios[0], M);
    }

    /* --- Edificio naranja: TODAS las transformaciones manuales ---
     *   COMPOSICION: M = T(tx,ty) * T(cx,cy) * R * S * SH * [Ref] * T(-cx,-cy) */
    {
        const float cx = 700.0f + 190.0f / 2.0f;   /* 795 */
        const float cy = 140.0f + 360.0f / 2.0f;   /* 320 */
        Mat3 M = mat_edificio(
            estado->ed_naranja_tx, estado->ed_naranja_ty,
            estado->ed_naranja_angulo, estado->ed_naranja_escala,
            estado->ed_naranja_shear, estado->ed_naranja_reflejo,
            cx, cy);
        dibujar_edificio_t(&ESCENA_URBANA.edificios[2], M);
    }

    /* --- Casa y elementos fijos --- */
    dibujar_casa(&ESCENA_URBANA.casa);
    dibujar_semaforo(&ESCENA_URBANA.semaforo);

    /* --- Arboles estaticos --- */
    dibujar_arbol_t(&ESCENA_URBANA.arboles[0], mat3_identidad());
    dibujar_arbol_t(&ESCENA_URBANA.arboles[1], mat3_identidad());

    /* --- DISTORSION (SHEAR): senal de transito, automatica ---
     *   M = T(cx,cy) * SH_x(shx) * T(-cx,-cy)                     */
    dibujar_senal_t(&ESCENA_URBANA.senal, estado->shear_senal);

    /* --- Carretera --- */
    dibujar_carretera(&ESCENA_URBANA.carretera);

    /* --- REFLEXION EJE-X: vehiculo rojo reflejado en el piso ---
     *   M = Ref_x(88) * T(tx,0)                                     */
    dibujar_vehiculo_reflexion(&ESCENA_URBANA.vehiculo,
                               estado->vehiculo_tx, 88.0f);
    {
        const Color c_eje = {160, 60, 60};
        dibujar_linea_bresenham(
            (Punto){VENTANA_RECORTE.xmin, 88},
            (Punto){VENTANA_RECORTE.xmax, 88},
            c_eje, VENTANA_RECORTE);
    }

    /* --- TRASLACION + ROTACION + COMPOSICION: vehiculo rojo ---
     *   M_cuerpo = T(tx, 0)
     *   M_rueda  = T(tx, 0) * R_pivote(ang, cx_rueda, cy_rueda)     */
    {
        Mat3 M_v = mat3_traslacion(estado->vehiculo_tx, 0.0f);
        dibujar_vehiculo_t(&ESCENA_URBANA.vehiculo, M_v, estado->angulo_rueda);
    }

    /* --- TRASLACION: bus azul (carril contrario, derecha -> izquierda) ---
     *   M_bus = T(bus_tx, 0)                                         */
    {
        Mat3 M_b = mat3_traslacion(estado->bus_tx, 0.0f);
        dibujar_bus_t(&BUS_AZUL, M_b, estado->bus_angulo);
    }

    /* --- Borde de la ventana de recorte --- */
    dibujar_borde_ventana();

    /* --- Etiquetas de transformacion en la escena --- */
    dibujar_etiquetas(estado);

    /* --- Panel HUD (selector + controles) --- */
    dibujar_panel_hud(estado);
}
