#include <config.h>

#include "error.h"
#include "bsp.h"

extern unsigned int *g_lm_texture_ids;
int g_bezier_steps = 4;

/***
Functions
***/

int
bspLoad(struct bsp  *bsp, char *filename)
	{
	FILE *fp=0;
	int i=0;
	unsigned int magic = 0;
	int version=0;

	fp = fopen(filename, "rb");

	if (!fp) error(-1, "Failed to open bsp file.");

	/*Magic number*/
	fread(&magic, 4, 1, fp);

	if (magic != 0x50534249) error(-1, "probably not a BSP file");
	printf("Magic number correct: %.4s\n", (char *) &magic);

	fread(&version, 4, 1, fp);
	printf("Version: %i\n", version);
	if (version != 46) error(-1, "only version 46 supported.");

	for (i=0; i<17; i++)
		{
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

void
get_point_on_patch(float patch[3][3][5], float x, float y, float point[3])
	{
	float Bu[3];
	float Bv[3];
	int i,j;

	Bu[0] = (1-x)*(1-x);
	Bu[1] = (2*x)*(1-x);
	Bu[2] = x*x;

	Bv[0] = (1-y)*(1-y);
	Bv[1] = (2*y)*(1-y);
	Bv[2] = y*y;

	for (i=0; i<3; i++)
		{
		for (j=0; j<3; j++)
			{
			point[0] += patch[i][j][0] * Bu[i] * Bv[j];
			point[1] += patch[i][j][1] * Bu[i] * Bv[j];
			point[2] += patch[i][j][2] * Bu[i] * Bv[j];
			point[3] += patch[i][j][3] * Bu[i] * Bv[j];
			point[4] += patch[i][j][4] * Bu[i] * Bv[j];
			}
		}

	}
/***
Good reference for Bezier patches.
https://www.gamedev.net/articles/programming/math-and-physics/bezier-patches-r1584/
***/
void
drawPatchFaces(float p[3][3][5])
	{
	int steps = g_bezier_steps;
	float step_size = 1.0f/steps;
	int i, j;

	for (i=0; i<steps*steps; i++)
		{
			float fx = (i%steps) * step_size;
			float fy = (i/steps) * step_size;
			float p00[5] = {0};
			float p10[5] = {0};
			float p11[5] = {0};
			float p01[5] = {0};

			get_point_on_patch(p, fx,fy, p00);
			get_point_on_patch(p, fx+step_size,fy, p10);
			get_point_on_patch(p, fx+step_size,fy+step_size, p11);
			get_point_on_patch(p, fx,fy+step_size, p01);

			/* Top triangle*/
			glTexCoord2f(p00[3], p00[4]);
			glVertex3f(p00[0],p00[1],p00[2]);
			glTexCoord2f(p01[3], p01[4]);
			glVertex3f(p01[0],p01[1],p01[2]);
			glTexCoord2f(p10[3], p10[4]);
			glVertex3f(p10[0],p10[1],p10[2]);
			/* Bottom triangle*/
			glTexCoord2f(p10[3], p10[4]);
			glVertex3f(p10[0],p10[1],p10[2]);
			glTexCoord2f(p01[3], p01[4]);
			glVertex3f(p01[0],p01[1],p01[2]);
			glTexCoord2f(p11[3], p11[4]);
			glVertex3f(p11[0],p11[1],p11[2]);
		}
	}

void
drawPatch(int w, int h, struct bsp_vertex *verts, int n_verts)
	{
	/* Patch is 3x3 with x,y,z*/
	float patch[3][3][5];
	/* Calculate how many patches we need to deal with */
	int pw = (w-1)/2;
	int ph = (h-1)/2;
	int i, j, x, y;

	/* Process each patch */
	glBegin(GL_TRIANGLES);
	for (i=0; i<pw*ph; i++)
		{
		int px = i%pw;
		int py = i/pw;
		int index = (px*2) + ((py*2)*w);
			/* Get patch */
			for (j=0; j<3*3; j++)
				{
				float *v;
				x = j%3;
				y = j/3;
				patch[x][y][0] = verts[index + x + (y*w)].position[0];
				patch[x][y][1] = verts[index + x + (y*w)].position[1];
				patch[x][y][2] = verts[index + x + (y*w)].position[2];
				patch[x][y][3] = verts[index + x + (y*w)].texcoord[1][0];
				patch[x][y][4] = verts[index + x + (y*w)].texcoord[1][1];
				}
				drawPatchFaces(patch);

		}
	glEnd();
	glColor3f(1,1,1);
	}


void
drawBspFace(struct bsp_face *face, struct bsp *bsp)
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
		{
		glBindTexture(GL_TEXTURE_2D, g_lm_texture_ids[face->lm_index]);
		}
	else glBindTexture(GL_TEXTURE_2D,0);

	switch (face->type)
		{
		case 3: /*Model mesh has a normal*/
			normals = 1;
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
		case 1:
			glBegin(GL_TRIANGLES);
			for (i=0; i<total; i++)
				{
				struct bsp_vertex *vertex;
				int offset = meshverts[face->meshvert+i];
				vertex = &vertices[base + offset];
				if (normals) glNormal3f(vertex->normal[0], vertex->normal[1], vertex->normal[2]);
				glTexCoord2f(vertex->texcoord[1][0],vertex->texcoord[1][1] );
				glVertex3f(vertex->position[0], vertex->position[1], vertex->position[2] );
				}
			glEnd();
			if (normals)
				{
				glDisable(GL_LIGHTING);
				}
			break;
		case 2: /*Patch - is not in the mesh verts?*/
			drawPatch(face->size[0],face->size[1], &vertices[face->vertex], face->n_vertexes);
			break;
		}
	}

/*Get string in quotes*/
int
get_string(char *string, char *dest)
	{
	int count=0;
	int started=0;
	int d_pos = 0;

	while (count < 128)
		{
		char c = string[count];
		count++;

		if (c=='"')
			{
			if (started) {break;}
			started = 1;
			continue;
			}

		if (started) 
			{
			dest[d_pos]=c;
			dest[d_pos+1]=0;
			d_pos++;
			}
		}

	return count;
	}

int 
bspLoadEntities(struct bsp *bsp, struct map *map)
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

	while (pos < entity_string_length)
		{
		unsigned int count=0;
		char c=0;

		/*Get rid of whitespace between strings*/
		while (isspace(entity_string[pos++]));
		c = entity_string[--pos];

		/*Look for opening \{*/
		if (c == '{') 
			{ /*New entity*/
			in_entity=1; 
			pos++;

			entity_count++;
			map->entities = realloc(map->entities, sizeof(struct entity)*entity_count);
			current_entity = &map->entities[entity_count-1];
			current_entity->n_properties = 0;
			current_entity->properties = malloc(sizeof(struct entity_property));
			}

		if (c == '}') in_entity=0;
		
		if (in_entity) 
			{
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
			}

		else pos++;
		}

	map->n_entities = entity_count;

	return 0;
	}

struct entity_property*
entityGetPropertyByName(struct entity *e, char *name)
	{
	int i;

	for (i=0; i<e->n_properties; i++)
		{
		struct entity_property *prop;
		prop = &e->properties[i];
		if (!strcmp(prop->name, name)) return prop;
		}
	return 0;
	}
