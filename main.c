#include <GL/glut.h>
#include <stdbool.h>
#include <stdio.h>

enum {
    INSIDE = 0,
    LEFT = 1,
    RIGHT = 2,
    BOTTOM = 4,
    TOP = 8
};

typedef struct {
    float x0;
    float y0;
    float x1;
    float y1;
} Line;

typedef struct {
    float xmin;
    float ymin;
    float xmax;
    float ymax;
} ClipWindow;

static const ClipWindow g_clip_window = {150.0f, 120.0f, 450.0f, 360.0f};

static const Line g_lines[] = {
    {80.0f, 80.0f, 520.0f, 400.0f},
    {180.0f, 150.0f, 420.0f, 320.0f},
    {50.0f, 300.0f, 200.0f, 420.0f},
    {250.0f, 50.0f, 250.0f, 430.0f},
    {480.0f, 100.0f, 560.0f, 220.0f},
    {100.0f, 200.0f, 500.0f, 200.0f}
};

static const int g_line_count = (int)(sizeof(g_lines) / sizeof(g_lines[0]));

/* Parte 1: codigos de region del algoritmo de Cohen-Sutherland. */
static int compute_out_code(float x, float y, ClipWindow clip_window)
{
    int code = INSIDE;

    if (x < clip_window.xmin) {
        code |= LEFT;
    } else if (x > clip_window.xmax) {
        code |= RIGHT;
    }

    if (y < clip_window.ymin) {
        code |= BOTTOM;
    } else if (y > clip_window.ymax) {
        code |= TOP;
    }

    return code;
}

/* Parte 2: bucle de recorte, rechazo/aceptacion e intersecciones. */
static bool cohen_sutherland_clip(Line input, ClipWindow clip_window, Line *output)
{
    float x0 = input.x0;
    float y0 = input.y0;
    float x1 = input.x1;
    float y1 = input.y1;

    int code0 = compute_out_code(x0, y0, clip_window);
    int code1 = compute_out_code(x1, y1, clip_window);
    bool accept = false;

    while (true) {
        if ((code0 | code1) == 0) {
            accept = true;
            break;
        }

        if ((code0 & code1) != 0) {
            break;
        }

        int code_out = (code0 != 0) ? code0 : code1;
        float x = 0.0f;
        float y = 0.0f;

        if ((code_out & TOP) != 0) {
            x = x0 + (x1 - x0) * (clip_window.ymax - y0) / (y1 - y0);
            y = clip_window.ymax;
        } else if ((code_out & BOTTOM) != 0) {
            x = x0 + (x1 - x0) * (clip_window.ymin - y0) / (y1 - y0);
            y = clip_window.ymin;
        } else if ((code_out & RIGHT) != 0) {
            y = y0 + (y1 - y0) * (clip_window.xmax - x0) / (x1 - x0);
            x = clip_window.xmax;
        } else if ((code_out & LEFT) != 0) {
            y = y0 + (y1 - y0) * (clip_window.xmin - x0) / (x1 - x0);
            x = clip_window.xmin;
        }

        if (code_out == code0) {
            x0 = x;
            y0 = y;
            code0 = compute_out_code(x0, y0, clip_window);
        } else {
            x1 = x;
            y1 = y;
            code1 = compute_out_code(x1, y1, clip_window);
        }
    }

    if (!accept) {
        return false;
    }

    output->x0 = x0;
    output->y0 = y0;
    output->x1 = x1;
    output->y1 = y1;
    return true;
}

static void draw_text(float x, float y, const char *text)
{
    const unsigned char *cursor = (const unsigned char *)text;

    glRasterPos2f(x, y);
    while (*cursor != '\0') {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *cursor);
        ++cursor;
    }
}

static void draw_line(Line line)
{
    glBegin(GL_LINES);
    glVertex2f(line.x0, line.y0);
    glVertex2f(line.x1, line.y1);
    glEnd();
}

static void draw_clip_window(ClipWindow clip_window)
{
    glBegin(GL_LINE_LOOP);
    glVertex2f(clip_window.xmin, clip_window.ymin);
    glVertex2f(clip_window.xmax, clip_window.ymin);
    glVertex2f(clip_window.xmax, clip_window.ymax);
    glVertex2f(clip_window.xmin, clip_window.ymax);
    glEnd();
}

static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.95f, 0.85f, 0.15f);
    glLineWidth(2.0f);
    draw_clip_window(g_clip_window);

    glColor3f(0.65f, 0.65f, 0.65f);
    glLineWidth(1.0f);
    for (int index = 0; index < g_line_count; ++index) {
        draw_line(g_lines[index]);
    }

    glLineWidth(3.0f);
    for (int index = 0; index < g_line_count; ++index) {
        Line clipped_line;

        if (cohen_sutherland_clip(g_lines[index], g_clip_window, &clipped_line)) {
            glColor3f(0.15f, 0.85f, 0.35f);
            draw_line(clipped_line);
        } else {
            glColor3f(0.95f, 0.25f, 0.25f);
            glPointSize(6.0f);
            glBegin(GL_POINTS);
            glVertex2f(g_lines[index].x0, g_lines[index].y0);
            glVertex2f(g_lines[index].x1, g_lines[index].y1);
            glEnd();
        }
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    draw_text(20.0f, 450.0f, "Entrega 1: Recorte de lineas con Cohen-Sutherland");
    draw_text(20.0f, 430.0f, "Gris: linea original  |  Verde: segmento aceptado  |  Rojo: segmento rechazado");

    glutSwapBuffers();
}

static void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 600.0, 0.0, 480.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void init(void)
{
    glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(900, 700);
    glutCreateWindow("Cohen-Sutherland en C");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMainLoop();

    return 0;
}