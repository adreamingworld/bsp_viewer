#include <SDL.h>
#include <SDL_opengl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <IL/il.h>
#include <IL/ilut.h>

#define SCALE 1
#define SPEED (200.0*SCALE)

SDL_Window *window;
unsigned int *lm_texture_ids;
unsigned int il_image_id;

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

struct texture {
	char name[64];
	int flags;
	int contents;
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

struct player {
	float x,y,z;
	float rx,ry,rz;
};

/***
Functions
***/

void checkSDLError(int line)
{
	printf("Checking errors at line: %i 	...", line);
	const char *error = SDL_GetError();
	if (*error != 0)
	{
		printf("SDL Error: %s\n", error);
		if (line != -1)
			printf(" + line: %i\n", line);
		SDL_ClearError();
	}else printf("none\n");
}

void error(int e, char *string)
{
	fprintf(stderr, "Error: %s \n", string);
	exit(e);
}

int bspLoad(struct bsp  *bsp, char *filename)
{
	FILE *fp;
	int i;
	char magic[4];
	int version;

	fp = fopen(filename, "rb");
	if (!fp) {
		error(-1, "Failed to open bsp file.");
		return -1;
	}

	/*Magic number*/
	fread(magic, 4, 1, fp);
	fread(&version, 4, 1, fp);
	printf("Magic number: %.4s\n", magic);
	printf("Version: %i\n", version);

	for (i=0; i<17; i++) {
		unsigned int position;
		struct directory_entry *ent;
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

int setup_opengl(int w, int h)
{
	int flags;
	SDL_GLContext context = 0;
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
//w=1024;h=768;	
	window = SDL_CreateWindow("Quake 3 BSP Viewer", 0, 0, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);	
	context = SDL_GL_CreateContext(window);
	if (context == NULL) {
		error(-1, "Failed to create context.");
	}
	int d;
	SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &d);
	printf("Depth buffer: %i\n", d);
	int max_tunits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_tunits);
	printf("Max Texture Units: %i\n", max_tunits);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float ah = (float)h/(float)w;
	glFrustum(-1, 1, -ah, ah, 1, 5000);
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	checkSDLError(__LINE__);
	return 0;
}

void drawBspFace(struct bsp_face *face, struct bsp *bsp)
{
	int *vert_offset = 0;
	int total;
	int i;
	int base;
	int *meshverts;
	struct bsp_vertex *vertices;

	vertices = bsp->directory[VERTEXES].data;
	meshverts = bsp->directory[MESHVERTS].data;

	base = face->vertex;

	total = face->n_meshverts;
	/*Draw all meshverts*/
	switch (face->type) {
		case 1:
		case 3:
			if (face->lm_index >= 0)
				glBindTexture(GL_TEXTURE_2D, lm_texture_ids[face->lm_index]);
			else glBindTexture(GL_TEXTURE_2D,0);
			glBegin(GL_TRIANGLES);
			for (i=0; i<total; i++) {
				struct bsp_vertex *vertex;
				int offset = meshverts[face->meshvert+i];
				vertex = &vertices[base + offset];
				glTexCoord2f(vertex->texcoord[1][0],vertex->texcoord[1][1] );
				glVertex3f(vertex->position[0], vertex->position[1], vertex->position[2] );
			}
			glEnd();
			break;
		case 2: /*Patch - is not in the mesh verts?*/
			
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
	unsigned int entity_string_length;
	unsigned pos = 0;
	char *entity_string;
	unsigned int in_entity = 0;
	unsigned int entity_count = 0;

	entity_string = bsp->directory[ENTITIES].data;
	entity_string_length = bsp->directory[ENTITIES].length;

	/*initialize entities pointer*/
	map->entities = malloc(sizeof(struct entity));

	while (pos < entity_string_length) {
		struct entity *current_entity;
		unsigned int count;
		char c;

		/*Get rid of whitespace between strings*/
		while (isspace(entity_string[pos++]));
		c = entity_string[--pos];

		/*Look for opening {*/
		if (c == '{') { /*New entity*/
			in_entity=1; pos++;

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
			
			property->name = malloc(strlen(prop));
			property->value = malloc(strlen(value));
			strcpy(property->name, prop);
			strcpy(property->value, value);
		}else pos++;

	}

	map->n_entities = entity_count;

	return 0;
}

int playerMove(struct player *p, float x, float y, float z, float rx, float ry, float rz)
{
	p->x = x;
	p->y = y;
	p->z = z;
	p->rx = rx;
	p->ry = ry;
	p->rz = rz;
		
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

/* Spawn player at deathmatch spawn point index spawn_dest
	if index is invalid then loop back to 0th spawn point
*/
int spawnPlayer(struct player* p, struct map *m, int spawn_dest)
{
	int i;
	int found=0;
	printf("Spawning\n");

	for (i=0; i<m->n_entities; i++) {
		struct entity *e = &m->entities[i];
		int j;

		struct entity_property *prop;
		prop = entityGetPropertyByName(e, "classname");
		if (!prop) {
			printf("Entity has no class name!!\n");
			continue;
		}
		if (!strcmp(prop->value, "info_player_deathmatch")) {
			if (spawn_dest == found) {
				float x=0,y=0,z=0;
				float rz=0;
				prop = entityGetPropertyByName(e, "origin");
				if (prop) {
					sscanf(prop->value, "%f %f %f", &x, &y, &z);
					printf("origin: %s\n", prop->value);
				} else printf("origin: none, defaulting to 0,0,0\n");

				prop = entityGetPropertyByName(e, "angle");
				if (prop) {
					sscanf(prop->value, "%f", &rz);
					printf("angle: %s\n", prop->value);
				} else printf("angle: none, defaulting to 0\n");

				z+=26; /*Height of the player's eyes?*/
				/*We have to add 90 degrees for some reason, wrong matrix?*/
				playerMove(p, x*SCALE,y*SCALE,z*SCALE, 0,0,rz+90);
				return spawn_dest;
			}
			found++;
		}
	}

	if (spawn_dest != 0) {
		spawn_dest=0;
		spawnPlayer(p, m, spawn_dest);
		return spawn_dest;
	}

	printf("Failed to spawn\n");
	return 0;
}

void setup_icon(SDL_Window *w)
{
	SDL_Surface *surface;
	char current_filename[] = "resources/icon.bmp";
	char installed_filename[] = DATA_PATH"/icon.bmp";

	surface = SDL_LoadBMP(installed_filename);

	if (!surface) {
		printf("Not installed? Trying in working directory\n");
		surface = SDL_LoadBMP(current_filename);
		if (!surface) return;
	} else printf("Package is installed\n");

	SDL_SetWindowIcon(w, surface);
}
void take_screenshot(SDL_Window *window)
{
	int w,h;
	void *pixels;
	
	/* Get image of OpenGL display */
	SDL_GetWindowSize(window, &w, &h);
	pixels = malloc(w*h*3);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	/* Use IL to save png */
  	// w,h,depth(3d image), channels
	ilTexImage(w, h, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, pixels);
	ilSaveImage("screenshot.png");
	printf("Screenshot saved\n");

	/* Free image data */
	free(pixels);

}

int main(int argc, char *argv[])
{
	unsigned int spawn_point = 0;
	SDL_Event event;
	int quit = 0;
	struct bsp bsp;
	struct map map;
	char *filename ;
	int i;
	struct player player={0};

	float identity[16] = {
				-1,0,0,0,
				0,0,1,0,
				0,1,0,0,
				0,0,0,1,
				};	

	filename = argv[1]; 

	printf("BSP Viewer\n");
	bspLoad(&bsp, filename);

FILE *fp_ents;
fp_ents = fopen("entities.txt", "wb");
fprintf(fp_ents, "%s", bsp.directory[ENTITIES].data);
fclose(fp_ents);
bspLoadEntities(&bsp, &map);

	if (SDL_Init(SDL_INIT_VIDEO) <0) {
		error(-1, "Failed to init video");
	}

	SDL_DisplayMode dm;
	SDL_GetCurrentDisplayMode(0, &dm);
printf("Screen: %ix%i\n", dm.w, dm.h);
	ilEnable(IL_FILE_OVERWRITE);
	il_image_id = ilGenImage();
	ilBindImage(il_image_id);
	ilInit();
	setup_opengl(dm.w, dm.h);
	setup_icon(window);

	int n_faces ;

	n_faces = bsp.directory[FACES].length/sizeof(struct bsp_face);

	/*Load lightmaps into textures*/
	unsigned int tex_id=0;

	unsigned int n_lightmaps;

	n_lightmaps = bsp.directory[LIGHTMAPS].length/(128*128*3);
	lm_texture_ids = malloc(sizeof(unsigned int) * n_lightmaps);

	printf("Lightmap Count: %u\n", n_lightmaps);
	glGenTextures(n_lightmaps, lm_texture_ids);		/*Generate*/

	for (i=0; i<bsp.directory[LIGHTMAPS].length / (128*128*3); i++) {
		void *data = bsp.directory[LIGHTMAPS].data + (128*128*3)*i;
		glBindTexture(GL_TEXTURE_2D, lm_texture_ids[i]); 	/*Bind*/
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		/*Lighten*/
		int j;
		unsigned char* c=data;
		for (j=0; j<128*128*3;j++) {
			float i_c = c[j];
			i_c *= 2.3;
			if (i_c>255) i_c = 255;
			c[j] = i_c;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, data); /*Load data*/
	}

	/*List textures*/
	for (i=0; i<bsp.directory[TEXTURES].length/sizeof(struct texture); i++) {
		struct texture *t;
		t = &((struct texture *)bsp.directory[TEXTURES].data)[i];
	//	printf("Texture[%i]: %s\n", i, t->name);
	}

	int shift = 0;
	float time_delta = 0;
	unsigned int last_time;
	spawnPlayer(&player, &map, spawn_point++); 
	//playerMove(&player, 0,0,0,0,0,0);

	while (!quit) {
		last_time = SDL_GetTicks();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_s: spawn_point = spawnPlayer(&player, &map, spawn_point); spawn_point++; break;
						case SDLK_F1: 
							take_screenshot(window);

							break;
						case SDLK_ESCAPE: quit = 1; break;
						case SDLK_LSHIFT: shift=1; break;
					}
					break;
				case SDL_KEYUP:
					switch(event.key.keysym.sym) {
						case SDLK_LSHIFT: shift=0; break;
					}
					break;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLoadMatrixf(identity);
		glRotatef(-player.rx, 1,0,0);
		glRotatef(-player.ry, 0,1,0);
		glRotatef(-player.rz, 0,0,1);
		glTranslatef(-player.x,-player.y,-player.z);

		float mat[16];

		glGetFloatv(GL_MODELVIEW_MATRIX, mat);

		int mx,my;
		unsigned int mouse_state;
		mouse_state = SDL_GetRelativeMouseState(&mx, &my);
		if (mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
			player.rz -= mx/8.0;
			player.rx += my/8.0;

			if (player.rz>360) player.rz-=360;
			if (player.rx>360) player.rx-=360;
		}
		if (mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			/* ModelView Matrix has the backward vector?*/
			player.x -= time_delta*(SPEED*mat[2]);
			player.y -= time_delta*(SPEED*mat[6]);
			player.z -= time_delta*(SPEED*mat[10]);
		}
		//if (shift) {
		if (mouse_state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) {
			player.x += time_delta*(SPEED*mat[2]);
			player.y += time_delta*(SPEED*mat[6]);
			player.z += time_delta*(SPEED*mat[10]);
		}

		int n_leaves;
		n_leaves = bsp.directory[LEAVES].length/sizeof(struct bsp_leaf);
		for (i=0; i<n_leaves; i++) {
			struct bsp_leaf *leaves;
			struct bsp_leaf *leaf;
			struct bsp_face *faces;
			int *leaffaces = bsp.directory[LEAFFACES].data;
			int j;
	
			leaves = bsp.directory[LEAVES].data;
			faces = bsp.directory[FACES].data;
			leaf = &leaves[i];
			if (leaf->cluster <0) {
				continue;
			}
			for (j=0; j<leaf->n_leaffaces;j++) {
				int face_index;
				face_index = leaffaces[leaf->leafface+j];
				drawBspFace(&faces[face_index], &bsp);
			}
		}

		SDL_GL_SwapWindow(window);

		SDL_Delay(10);
		time_delta = (SDL_GetTicks() - last_time)/1000.0;
	}

	ilDeleteImage(il_image_id);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
