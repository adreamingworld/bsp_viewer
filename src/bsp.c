#include <config.h>

#include "error.h"
#include "bsp.h"

extern unsigned int *g_lm_texture_ids;
int g_bezier_steps = 4;

/***
Functions
***/

int bspLoad(struct bsp  *bsp, char *filename)
{
	FILE *fp=0;
	int i=0;
	unsigned int magic = 0;
	int version=0;

	fp = fopen(filename, "rb");
	if (!fp) {
		error(-1, "Failed to open bsp file.");
		return -1;
	}

	/*Magic number*/
	fread(&magic, 4, 1, fp);
	if (magic != 0x50534249) {
		error(-1, "probably not a BSP file");
	}
	printf("Magic number correct: %.4s\n", (char *) &magic);

	fread(&version, 4, 1, fp);
	printf("Version: %i\n", version);
	if (version != 46) {
		error(-1, "only version 46 supported.");
	}

	for (i=0; i<17; i++) {
		unsigned int position = 0;
		struct directory_entry *ent = 0;

		ent = &bsp->directory[i];
		fread(&bsp->directory[i].offset, 4, 1, fp);
		fread(&bsp->directory[i].length, 4, 1, fp);

		// Load lump
		position = ftell(fp);
		fseek(fp, ent->offset, SEEK_SET);
		ent->data = malloc(ent->length);
		fread(ent->data, 1, ent->length, fp);
		fseek(fp, position, SEEK_SET);

		//printf("lump [%02i] %7i, %7i\n", i, ent->offset, ent->length);
	}

	return 0;
}

/***
Curves
	1	2	3
C0	+---c---+
	|   |   |
	|   |   |
C1	c---c---c
	|   |   |
	|   |   |
C2	+---c---+

	0	1	2
	0.--1---2.--3---4.
	|   |   |   |   |
	|   |   |   |   |
	5---6---7---8---9
	|   |   |   |   |
	|   |   |   |   |
	10.-11--12.-13--14.
	|   |   |   |   |
	|   |   |   |   |
	15--16--17--18--19
	|   |   |   |   |
	|   |   |   |   |
	20.-21--22.-23--24.
***/
#define LERP(a,b,t) (a+(b-a)*t)

void curve(float c[3], struct bsp_vertex *v, float t)
{
	float cp[3], sp[3], ep[3];
	float s2c[3]; // start to control
	float c2e[3]; // control to end

	// control point
	cp[0] = v[1].position[0];
	cp[1] = v[1].position[1];
	cp[2] = v[1].position[2];
	// start point
	sp[0] = v[0].position[0];
	sp[1] = v[0].position[1];
	sp[2] = v[0].position[2];
      	// end point
	ep[0] = v[2].position[0];
	ep[1] = v[2].position[1];
	ep[2] = v[2].position[2];

	s2c[0] = LERP(sp[0], cp[0], t);
	s2c[1] = LERP(sp[1], cp[1], t);
	s2c[2] = LERP(sp[2], cp[2], t);
	c2e[0] = LERP(cp[0], ep[0], t);
	c2e[1] = LERP(cp[1], ep[1], t);
	c2e[2] = LERP(cp[2], ep[2], t);

	c[0] = LERP(s2c[0], c2e[0], t);
	c[1] = LERP(s2c[1], c2e[1], t);
	c[2] = LERP(s2c[2], c2e[2], t);
}

void texlerp(float tc0[2], float tc1[2], float tc2[2], float tc3[2]
		, float fx, float fy, float *s, float *t)
{
	/***
	0-----1
	|     |
	|     |
     fy |-----| fy
	|     |
	3-----2
	***/

	float lefty,righty;
	float topx, bottomx;

	lefty = LERP(tc0[1], tc3[1], fy);
	righty = LERP(tc1[1], tc2[1], fy);
	topx = LERP(tc0[0], tc1[0], fx);
	bottomx = LERP(tc3[0], tc2[0], fx);
	*s = LERP(topx, bottomx, fy);
	*t = LERP(lefty, righty, fx);
}

void drawPatch(int w, int h, struct bsp_vertex *verts, int n_verts)
{
	int pw = (w-1)/2;
	int ph = (h-1)/2;
	int i=0,j=0;
	float step_size = 1.0f /(g_bezier_steps);

	glBegin(GL_TRIANGLES);
	for (i=0; i<pw*ph; i++) {
		int px = i%pw;
		int py = i/pw;
		int pind_x = px*2;
		int pind_y = (py*2)*w;
		int index = pind_x+pind_y;
		struct bsp_vertex *v;
		struct bsp_vertex *ov[4]; /*original 4 verts at corners of patch*/
		ov[3] = &verts[index];
		ov[2] = &verts[index+2];
		ov[1] = &verts[index+2+(w*2)];
		ov[0] = &verts[index+(w*2)];

		v = &verts[index];
			//Texture coords
			float tc0[2];
			float tc1[2];
			float tc2[2];
			float tc3[2];
			float s[4],t[4];

			tc3[0] = ov[0]->texcoord[1][0];
			tc3[1] = ov[0]->texcoord[1][1];
			tc2[0] = ov[1]->texcoord[1][0];
			tc2[1] = ov[1]->texcoord[1][1];
			tc1[0] = ov[2]->texcoord[1][0];
			tc1[1] = ov[2]->texcoord[1][1];
			tc0[0] = ov[3]->texcoord[1][0];
			tc0[1] = ov[3]->texcoord[1][1];

		for (j=0; j<g_bezier_steps*g_bezier_steps; j++) {
			float fx[4];
			float fy[4];
			fx[0] = (j%g_bezier_steps) * step_size;
			fx[1] = (j%g_bezier_steps) * step_size+step_size;
			fx[2] = (j%g_bezier_steps) * step_size+step_size;
			fx[3] = (j%g_bezier_steps) * step_size;

			fy[0] = (j/g_bezier_steps) * step_size;
			fy[1] = (j/g_bezier_steps) * step_size;
			fy[2] = (j/g_bezier_steps) * step_size+step_size;
			fy[3] = (j/g_bezier_steps) * step_size+step_size;
			
			/*Calculate texcoords*/
			texlerp(tc0, tc1, tc2, tc3, fx[0], fy[0], &s[0], &t[0]);
			texlerp(tc0, tc1, tc2, tc3, fx[1], fy[1], &s[1], &t[1]);
			texlerp(tc0, tc1, tc2, tc3, fx[2], fy[2], &s[2], &t[2]);
			texlerp(tc0, tc1, tc2, tc3, fx[3], fy[3], &s[3], &t[3]);

			float c[3][3];
			float f[4][3];
			struct bsp_vertex cv[3];
			int k;
			for (k=0;k<4;k++) {
				float l_fx = fx[k];
				float l_fy = fy[k];
				curve(c[0], &v[0], l_fx);
				curve(c[1], &v[0+w], l_fx);
				curve(c[2], &v[0+w*2], l_fx);
				cv[0].position[0] = c[0][0];
				cv[0].position[1] = c[0][1];
				cv[0].position[2] = c[0][2];
				cv[1].position[0] = c[1][0];
				cv[1].position[1] = c[1][1];
				cv[1].position[2] = c[1][2];
				cv[2].position[0] = c[2][0];
				cv[2].position[1] = c[2][1];
				cv[2].position[2] = c[2][2];
				curve(f[k], cv, l_fy);
			}

			//Top
			glTexCoord2f(s[3], t[3]);
			glVertex3f(f[3][0], f[3][1], f[3][2]);
			glTexCoord2f(s[1], t[1]);
			glVertex3f(f[1][0], f[1][1], f[1][2]);
			glTexCoord2f(s[0], t[0]);
			glVertex3f(f[0][0], f[0][1], f[0][2]);
			// Bottom
			glTexCoord2f(s[3], t[3]);
			glVertex3f(f[3][0], f[3][1], f[3][2]);
			glTexCoord2f(s[2], t[2]);
			glVertex3f(f[2][0], f[2][1], f[2][2]);
			glTexCoord2f(s[1], t[1]);
			glVertex3f(f[1][0], f[1][1], f[1][2]);
		}
	}
	glEnd();
}

void drawBspFace(struct bsp_face *face, struct bsp *bsp)
{
	int total=0;
	int i=0;
	int base=0;
	int *meshverts=0;
	struct bsp_vertex *vertices=0;
	int normals=0;

	vertices = bsp->directory[VERTEXES].data;
	meshverts = bsp->directory[MESHVERTS].data;

	base = face->vertex;

	total = face->n_meshverts;
	/*Draw all meshverts*/
	if (face->lm_index >= 0)
		glBindTexture(GL_TEXTURE_2D, g_lm_texture_ids[face->lm_index]);
	else glBindTexture(GL_TEXTURE_2D,0);

	switch (face->type) {
		case 3: /*Model mesh has a normal*/
			normals = 1;
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
		case 1:
			glBegin(GL_TRIANGLES);
			for (i=0; i<total; i++) {
				struct bsp_vertex *vertex;
				int offset = meshverts[face->meshvert+i];
				vertex = &vertices[base + offset];
				if (normals) glNormal3f(vertex->normal[0], vertex->normal[1], vertex->normal[2]);
				glTexCoord2f(vertex->texcoord[1][0],vertex->texcoord[1][1] );
				glVertex3f(vertex->position[0], vertex->position[1], vertex->position[2] );
			}
			glEnd();
			if (normals) {
				glDisable(GL_LIGHTING);
			}
			break;
		case 2: /*Patch - is not in the mesh verts?*/
			drawPatch(face->size[0],face->size[1], &vertices[face->vertex], face->n_vertexes);
			
			break;
	}
}

/*Get string in quotes*/
int get_string(char *string, char *dest)
{
	int count=0;
	int started=0;
	int d_pos = 0;
	while (count < 128) {
		char c = string[count];
		count++;
		if (c=='"') {
			if (started) {break;}
			started = 1;
			continue;
		}
		if (started) {
			dest[d_pos]=c;
			dest[d_pos+1]=0;
			d_pos++;
		}
	}
	return count;
}

int bspLoadEntities(struct bsp *bsp, struct map *map)
{
	struct entity *current_entity=0;
	unsigned int entity_string_length=0;
	unsigned pos = 0;
	char *entity_string;
	unsigned int in_entity = 0;
	unsigned int entity_count = 0;

	entity_string = bsp->directory[ENTITIES].data;
	entity_string_length = bsp->directory[ENTITIES].length;

	/*initialize entities pointer*/
	map->entities = malloc(sizeof(struct entity));

	while (pos < entity_string_length) {
		unsigned int count=0;
		char c=0;

		/*Get rid of whitespace between strings*/
		while (isspace(entity_string[pos++]));
		c = entity_string[--pos];

		/*Look for opening {*/
		if (c == '{') { /*New entity*/
			in_entity=1; 
			pos++;

			entity_count++;
			map->entities = realloc(map->entities, sizeof(struct entity)*entity_count);
			current_entity = &map->entities[entity_count-1];
			current_entity->n_properties = 0;
			current_entity->properties = malloc(sizeof(struct entity_property));
		}

		if (c == '}') {
			in_entity=0;
		}

		if (in_entity) {
			char prop[128];
			char value[128];
			struct entity_property *property;

			current_entity->n_properties++;
			current_entity->properties = realloc(current_entity->properties, sizeof(struct entity_property)*current_entity->n_properties);
			property = &current_entity->properties[current_entity->n_properties-1];

			count = get_string(entity_string+pos, prop);
			pos += count;
			if (prop[0]==0) continue; /*we have an '{'*/
			count = get_string(entity_string+pos, value);
			pos += count;
			//printf("%s = %s\n", prop, value);
			
			property->name = malloc(strlen(prop) + 1);
			property->value = malloc(strlen(value) + 1);
			strcpy(property->name, prop);
			strcpy(property->value, value);
		}else pos++;

	}

	map->n_entities = entity_count;

	return 0;
}

struct entity_property *entityGetPropertyByName(struct entity *e, char *name)
{
	int i;

	for (i=0; i<e->n_properties; i++) {
		struct entity_property *prop;
		prop = &e->properties[i];
		if (!strcmp(prop->name, name)) {
			return prop;
		}
	}
	return 0;
}
