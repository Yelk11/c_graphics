#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>
#include <stdbool.h>

#define WIDTH 120
#define HEIGHT 40
#define CUBE_SIZE 15.0
#define DEPTH 100.0

// 3D point structure
typedef struct {
    float x, y, z;
} Point3D;

// 2D point structure
typedef struct {
    float x, y;
} Point2D;

// Cube vertices
Point3D vertices[8] = {
    {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
    {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
};

// Cube faces (each face is defined by 4 vertex indices)
int faces[6][4] = {
    {0, 1, 2, 3}, // front
    {5, 4, 7, 6}, // back
    {4, 0, 3, 7}, // left
    {1, 5, 6, 2}, // right
    {4, 5, 1, 0}, // bottom
    {3, 2, 6, 7}  // top
};

float zbuffer[HEIGHT][WIDTH];
wchar_t buffer[HEIGHT][WIDTH];

// Clear screen buffer
void clear_buffers() {
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            zbuffer[y][x] = INFINITY;
            buffer[y][x] = L' ';
        }
    }
}

// Project 3D point to 2D
Point2D project(Point3D p) {
    float scale = DEPTH / (p.z + DEPTH);
    Point2D p2d = {
        (p.x * scale * CUBE_SIZE) + WIDTH/2,
        (p.y * scale * CUBE_SIZE) + HEIGHT/2
    };
    return p2d;
}

// Rotate a point around Y axis
Point3D rotate_y(Point3D p, float angle) {
    Point3D r = {
        p.x * cos(angle) - p.z * sin(angle),
        p.y,
        p.x * sin(angle) + p.z * cos(angle)
    };
    return r;
}

// Rotate a point around X axis
Point3D rotate_x(Point3D p, float angle) {
    Point3D r = {
        p.x,
        p.y * cos(angle) - p.z * sin(angle),
        p.y * sin(angle) + p.z * cos(angle)
    };
    return r;
}

void draw_filled_triangle(Point2D p1, Point2D p2, Point2D p3, float z1, float z2, float z3, wchar_t shade) {
    // Sort points by y coordinate
    if (p1.y > p2.y) { Point2D tp = p1; p1 = p2; p2 = tp; float tz = z1; z1 = z2; z2 = tz; }
    if (p1.y > p3.y) { Point2D tp = p1; p1 = p3; p3 = tp; float tz = z1; z1 = z3; z3 = tz; }
    if (p2.y > p3.y) { Point2D tp = p2; p2 = p3; p3 = tp; float tz = z2; z2 = z3; z3 = tz; }

    int total_height = p3.y - p1.y;
    if (total_height == 0) return;

    for (int y = p1.y; y <= p3.y; y++) {
        bool second_half = y > p2.y || p2.y == p1.y;
        int segment_height = second_half ? p3.y - p2.y : p2.y - p1.y;
        if (segment_height == 0) continue;

        float alpha = (float)(y - p1.y) / total_height;
        float beta = second_half ? 
            (float)(y - p2.y) / segment_height :
            (float)(y - p1.y) / segment_height;

        Point2D A = {
            p1.x + (p3.x - p1.x) * alpha,
            p1.y + (p3.y - p1.y) * alpha
        };
        Point2D B = second_half ?
            (Point2D){p2.x + (p3.x - p2.x) * beta, p2.y + (p3.y - p2.y) * beta} :
            (Point2D){p1.x + (p2.x - p1.x) * beta, p1.y + (p2.y - p1.y) * beta};

        float zA = z1 + (z3 - z1) * alpha;
        float zB = second_half ? z2 + (z3 - z2) * beta : z1 + (z2 - z1) * beta;

        if (A.x > B.x) { 
            Point2D tp = A; A = B; B = tp;
            float tz = zA; zA = zB; zB = tz;
        }

        for (int x = A.x; x <= B.x; x++) {
            float phi = B.x == A.x ? 1.0 : (float)(x - A.x) / (B.x - A.x);
            float z = zA + (zB - zA) * phi;
            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                if (z < zbuffer[y][x]) {
                    zbuffer[y][x] = z;
                    buffer[y][x] = shade;
                }
            }
        }
    }
}

int main() {
    setlocale(LC_ALL, "");
    printf("\033[?25l"); // Hide cursor
    float angle = 0.0;

    // Shading characters from darkest to lightest
    wchar_t shades[] = L"░▒▓█";

    while(1) {
        clear_buffers();
        Point3D rotated[8];
        Point2D projected[8];
        float z_coords[8];

        // Rotate and project vertices
        for(int i = 0; i < 8; i++) {
            rotated[i] = rotate_y(vertices[i], angle);
            rotated[i] = rotate_x(rotated[i], angle * 0.5);
            projected[i] = project(rotated[i]);
            z_coords[i] = rotated[i].z;
        }

        // Draw faces
        for(int f = 0; f < 6; f++) {
            float avg_z = (z_coords[faces[f][0]] + z_coords[faces[f][1]] + 
                         z_coords[faces[f][2]] + z_coords[faces[f][3]]) / 4.0;
            
            // Calculate face normal for basic lighting
            Point3D v1 = rotated[faces[f][1]];
            Point3D v2 = rotated[faces[f][2]];
            Point3D v3 = rotated[faces[f][0]];
            
            // Calculate normal
            float nx = (v2.y - v1.y) * (v3.z - v1.z) - (v2.z - v1.z) * (v3.y - v1.y);
            float ny = (v2.z - v1.z) * (v3.x - v1.x) - (v2.x - v1.x) * (v3.z - v1.z);
            float nz = (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x);
            
            // Simple lighting calculation
            float light = (nx + ny - nz) / sqrt(nx*nx + ny*ny + nz*nz);
            int shade_idx = (light + 1.0) * 1.5;
            if (shade_idx < 0) shade_idx = 0;
            if (shade_idx >= 4) shade_idx = 3;

            // Draw two triangles for each face
            draw_filled_triangle(
                projected[faces[f][0]], projected[faces[f][1]], projected[faces[f][2]],
                z_coords[faces[f][0]], z_coords[faces[f][1]], z_coords[faces[f][2]],
                shades[shade_idx]
            );
            draw_filled_triangle(
                projected[faces[f][0]], projected[faces[f][2]], projected[faces[f][3]],
                z_coords[faces[f][0]], z_coords[faces[f][2]], z_coords[faces[f][3]],
                shades[shade_idx]
            );
        }

        // Print frame
        printf("\033[H");
        for(int y = 0; y < HEIGHT; y++) {
            for(int x = 0; x < WIDTH; x++) {
                putwchar(buffer[y][x]);
            }
            putwchar(L'\n');
        }

        angle += 0.05;
        usleep(50000);
    }

    return 0;
} 