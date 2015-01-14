#include <math.h>
#include "toolkit.h"

struct texpair
{
    const char *mFilename;
    GLuint mHandle;
    int mClamp;
};

int gScreenWidth = 0;
int gScreenHeight = 0;
UIState gUIState = {0,0,0,0,0,0,0,0,0,0};
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
   
#ifdef DESIRED_ASPECT
    float aspect = DESIRED_ASPECT;
    if (((float)gScreenWidth / gScreenHeight) > aspect)
    {
        float realx = gScreenHeight * aspect;
        float extrax = gScreenWidth - realx;

        glViewport( extrax / 2, 0, realx, gScreenHeight );
    }
    else
    {
        float realy = gScreenWidth / aspect;
        float extray = gScreenHeight - realy;

        glViewport( 0, extray / 2, gScreenWidth, realy );
    }
#else
    glViewport( 0, 0, gScreenWidth, gScreenHeight );
#endif

    reload_textures();    
}


static void do_loadtexture(const char * aFilename, int clamp = 1)
{
    int i, j;

    // Load texture using stb
	int x, y, n;
	unsigned char *data = stbi_load(aFilename, &x, &y, &n, 4);
    
    if (data == NULL)
        return;

    int l, w, h;
    w = x;
    h = y;
    l = 0;
    unsigned int * mip = new unsigned int[w * h * 5];
    unsigned int * src = (unsigned int*)data;

    memset(mip, 0, w * h * 4);

    // mark all pixels with alpha = 0 to black
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            if ((src[i * w + j] & 0xff000000) == 0)
                src[i * w + j] = 0;
        }
    }


    // Tell OpenGL to read the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)src);

    if (mip)
    {
        // precalculate summed area tables
        // it's a box filter, which isn't very good, but at least it's fast =)
        int ra = 0, ga = 0, ba = 0, aa = 0;
        int i, j, c;
        unsigned int * rbuf = mip + (w * h * 1);
        unsigned int * gbuf = mip + (w * h * 2);
        unsigned int * bbuf = mip + (w * h * 3);
        unsigned int * abuf = mip + (w * h * 4);
        
        for (j = 0, c = 0; j < h; j++)
        {
            ra = ga = ba = aa = 0;
            for (i = 0; i < w; i++, c++)
            {
                ra += (src[c] >>  0) & 0xff;
                ga += (src[c] >>  8) & 0xff;
                ba += (src[c] >> 16) & 0xff;
                aa += (src[c] >> 24) & 0xff;
                if (j == 0)
                {
                    rbuf[c] = ra;
                    gbuf[c] = ga;
                    bbuf[c] = ba;
                    abuf[c] = aa;
                }
                else
                {
                    rbuf[c] = ra + rbuf[c - w];
                    gbuf[c] = ga + gbuf[c - w];
                    bbuf[c] = ba + bbuf[c - w];
                    abuf[c] = aa + abuf[c - w];
                }
            }
        }

        while (w > 1 || h > 1)
        {
            l++;
            w /= 2;
            h /= 2;
            if (w == 0) w = 1;
            if (h == 0) h = 1;

            int dw = x / w;
            int dh = y / h;

            for (j = 0, c = 0; j < h; j++)
            {
                for (i = 0; i < w; i++, c++)
                {
                    int x1 = i * dw;
                    int y1 = j * dh;
                    int x2 = x1 + dw - 1;
                    int y2 = y1 + dh - 1;
                    int div = (x2 - x1) * (y2 - y1);
                    y1 *= x;
                    y2 *= x;
                    int r = rbuf[y2 + x2] - rbuf[y1 + x2] - rbuf[y2 + x1] + rbuf[y1 + x1];
                    int g = gbuf[y2 + x2] - gbuf[y1 + x2] - gbuf[y2 + x1] + gbuf[y1 + x1];
                    int b = bbuf[y2 + x2] - bbuf[y1 + x2] - bbuf[y2 + x1] + bbuf[y1 + x1];
					int a = abuf[y2 + x2] - abuf[y1 + x2] - abuf[y2 + x1] + abuf[y1 + x1];

                    r /= div;
                    g /= div;
                    b /= div;
                    a /= div;

                    if (a == 0)
                        mip[c] = 0;
                    else
                        mip[c] = ((r & 0xff) <<  0) | 
                                 ((g & 0xff) <<  8) | 
                                 ((b & 0xff) << 16) | 
                                 ((a & 0xff) << 24); 
                }
            }
            glTexImage2D(GL_TEXTURE_2D, l, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)mip);
        }
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); // Linear Filtering
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // Linear Filtering
        delete[] mip;
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // Linear Filtering
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // Linear Filtering
    }

    // and cleanup.
	stbi_image_free(data);

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

    do_loadtexture(aFilename, clamp);

    gTextureStoreSize++;

	texpair * t = (texpair *)realloc(gTextureStore, sizeof(texpair) * gTextureStoreSize);
	if (t != NULL)
	{
	    gTextureStore = t;
		gTextureStore[gTextureStoreSize-1].mFilename = mystrdup(aFilename);
		gTextureStore[gTextureStoreSize-1].mHandle = texname;
		gTextureStore[gTextureStoreSize-1].mClamp = clamp;
	}

    return texname;
}

void reload_textures()
{
    // bind the textures to the same texture names as the last time.
    int i;
    for (i = 0; i < gTextureStoreSize; i++)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureStore[i].mHandle);
        do_loadtexture(gTextureStore[i].mFilename, gTextureStore[i].mClamp);
    }
}

