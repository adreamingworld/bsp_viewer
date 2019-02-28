#include <config.h>

#include "error.h"
#include "bsp.h"

#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <IL/il.h>

#define SPEED (300.0)

unsigned int *g_lm_texture_ids=0;
extern int g_bezier_steps;
SDL_Window *g_window=0;
unsigned int g_il_image_id=0;

struct player
	{
	float x,y,z;
	float rx,ry,rz;
	};

void
check_sdl_error(int line)
	{
	printf("Checking errors at line: %i 	...", line);
	const char *error = SDL_GetError();
	if (*error != 0)
		{
		printf("SDL Error: %s\n", error);
		if (line != -1)
			printf(" + line: %i\n", line);
		SDL_ClearError();
		}
	else printf("none\n");
}

int setup_opengl(int w, int h, int x, int y)
{
	SDL_GLContext context = 0;
	
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
//w=1024;h=768;	
	g_window = SDL_CreateWindow("Quake 3 BSP Viewer", x, y, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);	
	context = SDL_GL_CreateContext(g_window);
	if (context == NULL) {
		error(-1, "Failed to create context.");
	}
	int d=0;
	SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &d);
	printf("Depth buffer: %i\n", d);
	int max_tunits=0;
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
	check_sdl_error(__LINE__);
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

/* Spawn player at deathmatch spawn point index spawn_dest
	if index is invalid then loop back to 0th spawn point
*/
int spawnPlayer(struct player* p, struct map *m, int spawn_dest)
{
	int i=0;
	int found=0;
	printf("Spawning\n");

	for (i=0; i<m->n_entities; i++) {
		struct entity *e = &m->entities[i];
		struct entity_property *prop = 0;

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
				playerMove(p, x,y,z, 0,0,rz+90);
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
	SDL_Surface *surface = 0;
	char current_filename[] = "resources/icon.bmp";
	char installed_filename[] = DATA_PATH"/icon.bmp";

	surface = SDL_LoadBMP(installed_filename);

	if (!surface) {
		surface = SDL_LoadBMP(current_filename);
		if (!surface) return;
		printf("Package is not installed, running from working directory.\n");
	} else printf("Package is installed\n");

	SDL_SetWindowIcon(w, surface);
}
void take_screenshot(SDL_Window *window)
{
	int w=0,h=0;
	void *pixels=0;
	
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
/***
	A = point
	B = point on the plane
	P = plane normal
	dotProduct(A-B, P);

	n = normal
***/
int infrontOfPlane(float n[3], float pos[3], float dist)
{
	float result = (pos[0])*n[0] + (pos[1])*n[1] + (pos[2])*n[2] - dist;
	if (result > 0) return 1;
	return 0;
}

int findCluster(struct bsp *bsp, float x, float y, float z)
{
	struct bsp_node* nodes = 0;
	struct bsp_plane *planes = 0;
	struct bsp_leaf *leaves = 0;
	unsigned int n_nodes = 0;
	struct bsp_node *node = 0;
	struct bsp_leaf *leaf = 0;
	float pos[3] = {x,y,z};

	leaves = bsp->directory[LEAVES].data;
	planes = bsp->directory[PLANES].data;
	nodes = bsp->directory[NODES].data;
	n_nodes = bsp->directory[NODES].length/sizeof(struct bsp_node);
	node = &nodes[0];

	while(1) {
		struct bsp_plane *plane = 0;

		plane = &planes[node->plane];

		if (infrontOfPlane(plane->normal, pos, plane->dist)) {
			if (node->children[0] >= 0) {
				node = &nodes[node->children[0]];
			} else {
				leaf = &leaves[-(node->children[0]+1)];
				break;
			}
		} else {
			if (node->children[1] >= 0) {
				node = &nodes[node->children[1]];
			} else {
				leaf = &leaves[-(node->children[1]+1)];
				break;
			}
		}
	}
	return leaf->cluster;
}

int clusterIsVisible(int current_cluster, int test_cluster, void *visdata) 
{
//	int n_vecs = *((int *)visdata);
	int sz_vecs = *((int *)(visdata+4));
	unsigned char *vecs = (unsigned char *)(visdata+8);

	if (1<<(current_cluster%8) & vecs[test_cluster*sz_vecs + (current_cluster/8)])
		return 1;

	return 0;
}

int main(int argc, char *argv[])
{
	int pvs_enabled=1;
	SDL_Event event = {0};
	unsigned int spawn_point = 0;
	int quit = 0;
	struct bsp bsp = {0};
	struct map map = {0};
	char *filename = 0;
	int i = 0;
	struct player player={0};

	float identity[16] = {
				-1,0,0,0,
				0,0,1,0,
				0,1,0,0,
				0,0,0,1,
				};	

	filename = argv[1]; 

	printf(PACKAGE_STRING"\n");
	bspLoad(&bsp, filename);

FILE *fp_ents = 0;
fp_ents = fopen("entities.txt", "wb");
fprintf(fp_ents, "%s", bsp.directory[ENTITIES].data);
fclose(fp_ents);
bspLoadEntities(&bsp, &map);

	if (SDL_Init(SDL_INIT_VIDEO) <0) {
		error(-1, "Failed to init video");
	}

	ilInit();
	ilEnable(IL_FILE_OVERWRITE);
	g_il_image_id = ilGenImage();
	ilBindImage(g_il_image_id);

	/* Set up which display to use */
	SDL_DisplayMode dm;
	int display_index = 0;
	if (SDL_GetCurrentDisplayMode(display_index, &dm) != 0) {
		char e_string[128];
		sprintf(e_string, "Display[%02i] does not exist.", display_index);
		error(-1, e_string);
	}

	printf("Screen[%i]: %ix%i\n", display_index, dm.w, dm.h);
	SDL_Rect b_rect;
	SDL_GetDisplayUsableBounds(display_index, &b_rect);
	setup_opengl(dm.w, dm.h, b_rect.x, b_rect.y);
	setup_icon(g_window);

	int n_faces=0;

	n_faces = bsp.directory[FACES].length/sizeof(struct bsp_face);
	/*Load lightmaps into textures*/

	unsigned int n_lightmaps=0;

	n_lightmaps = bsp.directory[LIGHTMAPS].length/(128*128*3);
	g_lm_texture_ids = malloc(sizeof(unsigned int) * n_lightmaps);

	printf("Lightmap Count: %u\n", n_lightmaps);
	glGenTextures(n_lightmaps, g_lm_texture_ids);		/*Generate*/

	for (i=0; i<bsp.directory[LIGHTMAPS].length / (128*128*3); i++) {
		int j=0;
		void *data = bsp.directory[LIGHTMAPS].data + (128*128*3)*i;
		unsigned char* c=data;

		glBindTexture(GL_TEXTURE_2D, g_lm_texture_ids[i]); 	/*Bind*/
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		/*Lighten*/
		for (j=0; j<128*128*3;j++) {
			float i_c = c[j];
			i_c *= LIGHTEN;
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
	int current_cluster = 0;

	while (!quit) {
		last_time = SDL_GetTicks();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_p: pvs_enabled = !pvs_enabled; break;
						case SDLK_UP: g_bezier_steps++; break;
						case SDLK_DOWN: g_bezier_steps--; if (g_bezier_steps < 1) g_bezier_steps = 1; break;
						case SDLK_s: spawn_point = spawnPlayer(&player, &map, spawn_point); spawn_point++; break;
						case SDLK_F1: 
							take_screenshot(g_window);

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
		unsigned int mouse_state = 0;
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
		if (pvs_enabled)
			current_cluster = findCluster(&bsp, player.x, player.y, player.z);

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
			/*If pvs_enables and we are not outside 
			  of a cluster and cluster is not visible 
			  then just skip it.
			*/
			if (pvs_enabled && current_cluster != -1 && !clusterIsVisible(current_cluster, leaf->cluster, bsp.directory[VISDATA].data)) {
				continue;
			}
			for (j=0; j<leaf->n_leaffaces;j++) {
				int face_index;
				face_index = leaffaces[leaf->leafface+j];
				drawBspFace(&faces[face_index], &bsp);
			}
		}

		SDL_GL_SwapWindow(g_window);

		SDL_Delay(10);
		time_delta = (SDL_GetTicks() - last_time)/1000.0;
	}

	ilDeleteImage(g_il_image_id);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
	return 0;
}

