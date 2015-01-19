#include <math.h>
#include "toolkit.h"

struct texpair
{
    const char *mFilename;
    GLuint mHandle;
    int mClamp;
	int mWidth, mHeight;
	unsigned char *mData;
};

int gScreenWidth = 0;
int gScreenHeight = 0;
UIState gUIState = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
texpair * gTextureStore = NULL;
int gTextureStoreSize = 0;


void initvideo(int argc)
{
    const SDL_VideoInfo *info = NULL;
    int bpp = 0;
    int flags = 0;

    info = SDL_GetVideoInfo();

    if (!info) 
    {
        fprintf(stderr, "Video query failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(0);
    }

#ifdef _DEBUG
    int fsflag = 0;
#else
#ifdef FULLSCREEN_BY_DEFAULT
    int fsflag = 1;
#else
    int fsflag = 0;
#endif
#endif

    if (argc > 1) fsflag = !fsflag;

    if (fsflag) 
    {
        gScreenWidth = info->current_w;
        gScreenHeight = info->current_h;
        bpp = info->vfmt->BitsPerPixel;
        flags = SDL_OPENGL | SDL_FULLSCREEN;
    }
    else
    {
        if (argc == 0)
        {
            // window was resized
        }
        else
        {
            gScreenWidth = DESIRED_WINDOW_WIDTH;
            gScreenHeight = DESIRED_WINDOW_HEIGHT;
        }
        bpp = info->vfmt->BitsPerPixel;
        flags = SDL_OPENGL;
#ifdef RESIZABLE_WINDOW
        flags |= SDL_RESIZABLE;
#endif
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if (SDL_SetVideoMode(gScreenWidth, gScreenHeight, bpp, flags) == 0) 
    {
        fprintf( stderr, "Video mode set failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(0);
    }
   
    glViewport( 0, 0, gScreenWidth, gScreenHeight );

    reload_textures();    
}


static unsigned char * do_loadtexture(const char * aFilename, int &aWidth, int &aHeight, int clamp = 1, unsigned char *aData = 0)
{
	// Load texture using stb
	unsigned char *data = NULL;
	int i, j;
	int w, h, n;
	
	if (!aData)
	{
		data = stbi_load(aFilename, &w, &h, &n, 4);
		
		if (data == NULL)
			return 0;

		unsigned int * src = (unsigned int*)data;

		// mark all pixels with alpha = 0 to black
		for (i = 0; i < h; i++)
		{
			for (j = 0; j < w; j++)
			{
				if ((src[i * w + j] & 0xff000000) == 0)
					src[i * w + j] = 0;
			}
		}
		aWidth = w;
		aHeight = h;
	}
	else
	{
		data = aData;
		w = aWidth;
		h = aHeight;
	}

    // Tell OpenGL to read the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)data);
	glGenerateMipmap(GL_TEXTURE_2D);

    //// and cleanup.
	//stbi_image_free(data);

    if (clamp)
    {
        // Set up texture parameters
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }
    else
    {
        // Set up texture parameters
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
	return data;
}

char * mystrdup(const char *aString)
{
	int len = strlen(aString);
	char * d = new char[len+1];
	memcpy(d, aString, len);
	d[len] = 0;
	return d;
}

GLuint load_texture(char * aFilename, int clamp)
{
    // First check if we have loaded this texture already
    int i;
    for (i = 0; i < gTextureStoreSize; i++)
    {
        if (stricmp(gTextureStore[i].mFilename, aFilename) == 0)
            return gTextureStore[i].mHandle;
    }

    // Create OpenGL texture handle and bind it to use

    GLuint texname;
    glGenTextures(1,&texname);
    glBindTexture(GL_TEXTURE_2D,texname);

	gTextureStoreSize++;

	texpair * t = (texpair *)realloc(gTextureStore, sizeof(texpair) * gTextureStoreSize);
	if (t != NULL)
	{
		gTextureStore = t;
		gTextureStore[gTextureStoreSize - 1].mFilename = mystrdup(aFilename);
		gTextureStore[gTextureStoreSize - 1].mHandle = texname;
		gTextureStore[gTextureStoreSize - 1].mClamp = clamp;
	}
	
	gTextureStore[gTextureStoreSize - 1].mData = do_loadtexture(aFilename, gTextureStore[gTextureStoreSize - 1].mWidth, gTextureStore[gTextureStoreSize - 1].mHeight, clamp, 0);

    return texname;
}

extern void progress();
void reload_textures()
{
    // bind the textures to the same texture names as the last time.
    int i;
    for (i = 0; i < gTextureStoreSize; i++)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureStore[i].mHandle);
		do_loadtexture(gTextureStore[i].mFilename,
			gTextureStore[i].mWidth,
			gTextureStore[i].mHeight,
			gTextureStore[i].mClamp,
			gTextureStore[i].mData);		            
		progress();
    }
}

