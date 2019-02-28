#ifndef BSP_H
#define BSP_H

#include <GL/gl.h>
#include <ctype.h>
#include <string.h>

#define LIGHTEN (2.5)

enum {
	ENTITIES,
	TEXTURES,
	PLANES,
	NODES,
	LEAVES,
	LEAFFACES,
	LEAFBRUSHES,
	MODELS,
	BRUSHES,
	BRUSHSIDES,
	VERTEXES,
	MESHVERTS,
	EFFECTS,
	FACES,
	LIGHTMAPS,
	LIGHTVOLS,
	VISDATA
};

struct bsp_plane {
	float normal[3];
	float dist;
};

struct texture {
	char name[64];
	int flags;
	int contents;
};

struct bsp_node {
	int plane;
	int children[2]; /*negative if leaf*/
	int mins[3];
	int maxs[3];
};

struct bsp_vertex {
	float position[3];
	float texcoord[2][2]; // 0=surface, 1=lightmap
	float normal[3];
	unsigned char color[4];
};

struct bsp_leaf {
	int cluster;
	int area;
	int mins[3];
	int maxs[3];
	int leafface;
	int n_leaffaces;
	int leafbrush;
	int n_leafbrushes;
};

struct bsp_face {
	int texture;
	int effect;
	int type;
	int vertex;
	int n_vertexes;
	int meshvert;
	int n_meshverts;
	int lm_index;
	int lm_start[2];
	int lm_size[2];
	float lm_origin[3];
	float lm_vecs[2][3];
	float normal[3];
	int size[2];
};

struct directory_entry {
	int offset;
	int length;
	void *data;
} ;

struct bsp{
	struct directory_entry directory[17];
} ;

/* Structs for drawing - not from BSP files */
struct leaf {
	float **inds; // verts[shader_index] - vertices indices arranged per shader
	unsigned int *n_indices; // indices count per shader
};

/* Entities */
struct entity_property {
	char *name;
	char *value;
};

struct entity {
	struct entity_property *properties;
	unsigned int n_properties;
};

struct map {
	struct leaf *leaves; /*Same order as indexed in BSP file*/
	struct entity *entities;
	unsigned int n_entities;
};

/***
FUNCTIONS
***/

int bspLoad(struct bsp  *bsp, char *filename);
#define LERP(a,b,t) (a+(b-a)*t)
void curve(float c[3], struct bsp_vertex *v, float t);
void texlerp(float tc0[2], float tc1[2], float tc2[2], float tc3[2]
		, float fx, float fy, float *s, float *t);
void drawPatch(int w, int h, struct bsp_vertex *verts, int n_verts);
void drawBspFace(struct bsp_face *face, struct bsp *bsp);
/*Get string in quotes*/
int get_string(char *string, char *dest);
int bspLoadEntities(struct bsp *bsp, struct map *map);
struct entity_property *entityGetPropertyByName(struct entity *e, char *name);

#endif /* BSP_H */
