/*
Terra3D - A 3D OpenGL terrain generator
Copyright (C) 2001  Steve Wortham

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*  
 *  TERRA3D
 *   by: Steve Wortham
 *   E-Mail: steve@gldomain.com
 *   website: www.gldomain.com
 */


#include <math.h>		// Header File For Windows
#include <windows.h>		// Header File For Windows
#include <stdio.h>			// Header File For Standard Input/Output
#include <stdarg.h>		// Header File For Variable Argument Routines	( ADD )
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
//#include <gl\glaux.h>		// Header File For The Glaux Library
#include "bmp.h"
#include <time.h>
#include <fstream>
#include "bitmap.h"
#include "glext.h"

using namespace std; // for using ifstream in iosfwd

#pragma warning(disable: 4800)
#pragma warning(disable: 4305)
#pragma warning(disable: 4244)


GLfloat SCREENWIDTH, SCREENHEIGHT;

////// Defines
#define BITMAP_ID 0x4D42		// the universal bitmap ID
////// Bitmap Information
BITMAPINFOHEADER	bitmapInfoHeader;		// bitmap info header
unsigned char*		bitmapData;			// the bitmap data
void*               imageData;			// the screen image data

// LoadBitmapFile
// desc: Returns a pointer to the bitmap image of the bitmap specified
//       by filename. Also returns the bitmap header information.
//		 No support for 8-bit bitmaps.
unsigned char *LoadBitmapFile(char *filename, BITMAPINFOHEADER *bitmapInfoHeader)
{
	FILE *filePtr;								// the file pointer
	BITMAPFILEHEADER	bitmapFileHeader;		// bitmap file header
	unsigned char		*bitmapImage;			// bitmap image data
	int					imageIdx = 0;			// image index counter
	unsigned char		tempRGB;				// swap variable

	// open filename in "read binary" mode
	filePtr = fopen(filename, "rb");
	if (filePtr == NULL)
		return NULL;

	// read the bitmap file header
	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);
	
	// verify that this is a bitmap by checking for the universal bitmap id
	if (bitmapFileHeader.bfType != BITMAP_ID)
	{
		fclose(filePtr);
		return NULL;
	}

	// read the bitmap information header
	fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);

	// move file pointer to beginning of bitmap data
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

	// allocate enough memory for the bitmap image data
	bitmapImage = (unsigned char*)malloc(bitmapInfoHeader->biSizeImage);

	// verify memory allocation
	if (!bitmapImage)
	{
		free(bitmapImage);
		fclose(filePtr);
		return NULL;
	}

	// read in the bitmap image data
	fread(bitmapImage, 1, bitmapInfoHeader->biSizeImage, filePtr);

	// make sure bitmap image data was read
	if (bitmapImage == NULL)
	{
		fclose(filePtr);
		return NULL;
	}

	// swap the R and B values to get RGB since the bitmap color format is in BGR
	for (imageIdx = 0; imageIdx < bitmapInfoHeader->biSizeImage; imageIdx+=3)
	{
		tempRGB = bitmapImage[imageIdx];
		bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
		bitmapImage[imageIdx + 2] = tempRGB;
	}

	// close the file and return the bitmap image data
	fclose(filePtr);
	return bitmapImage;
}

// WriteBitmapFile()
// desc: takes image data and saves it into a 24-bit RGB .BMP file
//       with width X height dimensions
int WriteBitmapFile(char *filename, int width, int height, unsigned char *imageData)
{
	FILE			 *filePtr;			// file pointer
	BITMAPFILEHEADER bitmapFileHeader;	// bitmap file header
	BITMAPINFOHEADER bitmapInfoHeader;	// bitmap info header
	int				 imageIdx;			// used for swapping RGB->BGR
	unsigned char	 tempRGB;			// used for swapping

	// open file for writing binary mode
	filePtr = fopen(filename, "wb");
	if (!filePtr)
		return 0;

	// define the bitmap file header
	bitmapFileHeader.bfSize = sizeof(BITMAPFILEHEADER);
	bitmapFileHeader.bfType = 0x4D42;
	bitmapFileHeader.bfReserved1 = 0;
	bitmapFileHeader.bfReserved2 = 0;
	bitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	
	// define the bitmap information header
	bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfoHeader.biPlanes = 1;
	bitmapInfoHeader.biBitCount = 24;						// 24-bit
	bitmapInfoHeader.biCompression = BI_RGB;				// no compression
	bitmapInfoHeader.biSizeImage = width * abs(height) * 3;	// width * height * (RGB bytes)
	bitmapInfoHeader.biXPelsPerMeter = 0;
	bitmapInfoHeader.biYPelsPerMeter = 0;
	bitmapInfoHeader.biClrUsed = 0;
	bitmapInfoHeader.biClrImportant = 0;
	bitmapInfoHeader.biWidth = width;						// bitmap width
	bitmapInfoHeader.biHeight = height;						// bitmap height

	// switch the image data from RGB to BGR
	for (imageIdx = 0; imageIdx < bitmapInfoHeader.biSizeImage; imageIdx+=3)
	{
		tempRGB = imageData[imageIdx];
		imageData[imageIdx] = imageData[imageIdx + 2];
		imageData[imageIdx + 2] = tempRGB;
	}

	// write the bitmap file header
	fwrite(&bitmapFileHeader, 1, sizeof(BITMAPFILEHEADER), filePtr);

	// write the bitmap info header
	fwrite(&bitmapInfoHeader, 1, sizeof(BITMAPINFOHEADER), filePtr);

	// write the image data
	fwrite(imageData, 1, bitmapInfoHeader.biSizeImage, filePtr);

	// close our file
	fclose(filePtr);

	return 1;
}

int frames = 0;

// SaveScreenshot()
// desc: takes a snapshot of the current window contents by retrieving
//       the screen data with glReadPixels() and writing the data to a file
void SaveScreenshot()
{
  frames++;
  if (frames <= 50)
  {	  
    char *filename;
		if (frames == 1) filename = "frame1.bmp";
		else if (frames == 2) filename = "frame2.bmp";
		else if (frames == 3) filename = "frame3.bmp";
		else if (frames == 4) filename = "frame4.bmp";
		else if (frames == 5) filename = "frame5.bmp";
		else if (frames == 6) filename = "frame6.bmp";
		else if (frames == 7) filename = "frame7.bmp";
		else if (frames == 8) filename = "frame8.bmp";
		else if (frames == 9) filename = "frame9.bmp";
		else if (frames == 10) filename = "frame10.bmp";
		else if (frames == 11) filename = "frame11.bmp";
		else if (frames == 12) filename = "frame12.bmp";
		else if (frames == 13) filename = "frame13.bmp";
		else if (frames == 14) filename = "frame14.bmp";
		else if (frames == 15) filename = "frame15.bmp";
		else if (frames == 16) filename = "frame16.bmp";
		else if (frames == 17) filename = "frame17.bmp";
		else if (frames == 18) filename = "frame18.bmp";
		else if (frames == 19) filename = "frame19.bmp";
		else if (frames == 20) filename = "frame20.bmp";
		else if (frames == 21) filename = "frame21.bmp";
		else if (frames == 22) filename = "frame22.bmp";
		else if (frames == 23) filename = "frame23.bmp";
		else if (frames == 24) filename = "frame24.bmp";
		else if (frames == 25) filename = "frame25.bmp";
		else if (frames == 26) filename = "frame26.bmp";
		else if (frames == 27) filename = "frame27.bmp";
		else if (frames == 28) filename = "frame28.bmp";
		else if (frames == 29) filename = "frame29.bmp";
		else if (frames == 30) filename = "frame30.bmp";
		else if (frames == 31) filename = "frame31.bmp";
		else if (frames == 32) filename = "frame32.bmp";
		else if (frames == 33) filename = "frame33.bmp";
		else if (frames == 34) filename = "frame34.bmp";
		else if (frames == 35) filename = "frame35.bmp";
		else if (frames == 36) filename = "frame36.bmp";
		else if (frames == 37) filename = "frame37.bmp";
		else if (frames == 38) filename = "frame38.bmp";
		else if (frames == 39) filename = "frame39.bmp";
		else if (frames == 40) filename = "frame40.bmp";
		else if (frames == 41) filename = "frame41.bmp";
		else if (frames == 42) filename = "frame42.bmp";
		else if (frames == 43) filename = "frame43.bmp";
		else if (frames == 44) filename = "frame44.bmp";
		else if (frames == 45) filename = "frame45.bmp";
		else if (frames == 46) filename = "frame46.bmp";
		else if (frames == 47) filename = "frame47.bmp";
		else if (frames == 48) filename = "frame48.bmp";
		else if (frames == 49) filename = "frame49.bmp";
		else if (frames == 50) filename = "frame50.bmp";

		imageData = malloc(SCREENWIDTH*SCREENHEIGHT*3);		// allocate memory for the imageData
		memset(imageData, 0, SCREENWIDTH*SCREENHEIGHT*3);	// clear imageData memory contents

		// read the image data from the window
		glReadPixels(0, 0, SCREENWIDTH-1, SCREENHEIGHT-1, GL_RGB, GL_UNSIGNED_BYTE, imageData);
	
		// write the image data to a file
		WriteBitmapFile(filename, SCREENWIDTH, SCREENHEIGHT, (unsigned char*)imageData);

		// free the image data memory
		free(imageData);
  }
}


HDC		hDC=NULL;			// Private GDI Device Context
HGLRC	hRC=NULL;			// Permanent Rendering Context
HWND	hWnd=NULL;			// Holds Our Window Handle

#define __ARB_ENABLE true											// Used To Disable ARB Extensions Entirely
// #define EXT_INFO													// Do You Want To See Your Extensions At Start-Up?
#define MAX_EXTENSION_SPACE 10240									// Characters for Extension-Strings
#define MAX_EXTENSION_LENGTH 256									// Maximum Of Characters In One Extension-String
bool multitextureSupported=false;									// Flag Indicating Whether Multitexturing Is Supported
bool useMultitexture=false;											// Use It If It Is Supported?
GLint maxTexelUnits=1;												// Number Of Texel-Pipelines. This Is At Least 1.

PFNGLMULTITEXCOORD1FARBPROC		glMultiTexCoord1fARB	= NULL;
PFNGLMULTITEXCOORD2FARBPROC		glMultiTexCoord2fARB	= NULL;
PFNGLMULTITEXCOORD3FARBPROC		glMultiTexCoord3fARB	= NULL;
PFNGLMULTITEXCOORD4FARBPROC		glMultiTexCoord4fARB	= NULL;
PFNGLACTIVETEXTUREARBPROC		glActiveTextureARB		= NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC	glClientActiveTextureARB= NULL;	

// Always Check For Extension-Availability During Run-Time!
// Here We Go!
bool isInString(char *string, const char *search) 
{
	int pos=0;
	int maxpos=strlen(search)-1;
	int len=strlen(string);	
	char *other;
	for (int i=0; i<len; i++) 
  {
		if ((i==0) || ((i>1) && string[i-1]=='\n')) 
    {				// New Extension Begins Here!
			other=&string[i];			
			pos=0;													// Begin New Search
			while (string[i]!='\n') 
      {								// Search Whole Extension-String
				if (string[i]==search[pos]) pos++;					// Next Position
				if ((pos>maxpos) && string[i+1]=='\n') return true; // We Have A Winner!
				i++;
			}			
		}
	}	
	return false;													// Sorry, Not Found!
}

// isMultitextureSupported() Checks At Run-Time If Multitexturing Is Supported
bool initMultitexture(void) 
{
	char *extensions;	
	extensions=strdup((char *) glGetString(GL_EXTENSIONS));			// Fetch Extension String
	int len=strlen(extensions);
	for (int i=0; i<len; i++)										// Separate It By Newline Instead Of Blank
		if (extensions[i]==' ') extensions[i]='\n';

#ifdef EXT_INFO
	MessageBox(hWnd,extensions,"supported GL extensions",MB_OK | MB_ICONINFORMATION);
#endif

	if (isInString(extensions,"GL_ARB_multitexture")				// Is Multitexturing Supported?
		&& __ARB_ENABLE)												// Override-Flag
	{	
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&maxTexelUnits);
		glMultiTexCoord1fARB	= (PFNGLMULTITEXCOORD1FARBPROC)		wglGetProcAddress("glMultiTexCoord1fARB");
		glMultiTexCoord2fARB	= (PFNGLMULTITEXCOORD2FARBPROC)		wglGetProcAddress("glMultiTexCoord2fARB");
		glMultiTexCoord3fARB	= (PFNGLMULTITEXCOORD3FARBPROC)		wglGetProcAddress("glMultiTexCoord3fARB");
		glMultiTexCoord4fARB	= (PFNGLMULTITEXCOORD4FARBPROC)		wglGetProcAddress("glMultiTexCoord4fARB");
		glActiveTextureARB		= (PFNGLACTIVETEXTUREARBPROC)		wglGetProcAddress("glActiveTextureARB");
		glClientActiveTextureARB= (PFNGLCLIENTACTIVETEXTUREARBPROC)	wglGetProcAddress("glClientActiveTextureARB");		
#ifdef EXT_INFO
	MessageBox(hWnd,"The GL_ARB_multitexture extension will be used.","feature supported!",MB_OK | MB_ICONINFORMATION);
#endif
	  return true;
	}
	return false;
}

typedef struct												// Create A Structure
{
	GLubyte	*imageData;										// Image Data (Up To 32 Bits)
	GLuint	bpp;											// Image Color Depth In Bits Per Pixel.
	GLuint	width;											// Image Width
	GLuint	height;											// Image Height
	GLuint	texID;											// Texture ID Used To Select A Texture
} TextureImage;												// Structure Name

GLuint		base;											// Base Display List For The Font
TextureImage textures[2];									// Storage For One Texture

int DIFF = 1;
GLfloat length;

bool	keys[256];			// Array Used For The Keyboard Routine
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default
bool  Generate_Sunlight = true;
bool	light;				// Lighting ON/OFF
bool  wireframe = FALSE;  // Wireframe ON/OFF
bool  water = true;  // Wireframe ON/OFF
bool  multitexture = false;  // Wireframe ON/OFF
bool  Afterburner = false;
bool  framerate_limit = true;
bool	lp;					// L Pressed? 
bool	fp;					// F Pressed? 
bool  wp;                 // W pressed?
bool  sp;                 // S pressed?
bool  mp;                 // M pressed?

bool  aq;
bool  sq;


GLUquadricObj *quadratic;	// Storage For Our Quadratic Objects ( NEW )
GLuint	texture[8];

GLuint MODEL;

inline GLfloat Hypot(GLfloat A, GLfloat B)
{
  return sqrt(A*A+B*B);
}
inline GLfloat ABS(GLfloat A)
{
  if (A < 0)
  A = -A; 
  return A;
}

#define	MAX_PARTICLES	250		// Number Of Particles To Create

GLfloat	slowdown=.50f;				// Slow Down Particles
GLfloat	xspeed;						// Base X Speed (To Allow Keyboard Direction Of Tail)
GLfloat	yspeed;						// Base Y Speed (To Allow Keyboard Direction Of Tail)

GLfloat water_flow = 0;
GLfloat V;
GLfloat Angle;
GLfloat AngleZ;
int loop;

typedef struct						// Create A Structure For Particle
{
	bool	active;					// Active (Yes/No)
	GLfloat	life;					// Particle Life
	GLfloat	fade;					// Fade Speed
	GLfloat	r;						// Red Value
	GLfloat	g;						// Green Value
	GLfloat	b;						// Blue Value
	GLfloat	x;						// X Position
	GLfloat	y;						// Y Position
	GLfloat	z;						// Z Position
	GLfloat	xi;						// X Direction
	GLfloat	yi;						// Y Direction
	GLfloat	zi;						// Z Direction
	GLfloat	xg;						// X Gravity
	GLfloat	yg;						// Y Gravity
	GLfloat	zg;						// Z Gravity
}
particles;							// Particles Structure

particles particle[MAX_PARTICLES];	// Particle Array (Room For Particle Info)

static GLfloat colors[12][3]=		// Rainbow Of Colors
{
	{1.0f,0.5f,0.5f},{1.0f,0.75f,0.5f},{1.0f,1.0f,0.5f},{0.75f,1.0f,0.5f},
	{0.5f,1.0f,0.5f},{0.5f,1.0f,0.75f},{0.5f,1.0f,1.0f},{0.5f,0.75f,1.0f},
	{0.5f,0.5f,1.0f},{0.75f,0.5f,1.0f},{1.0f,0.5f,1.0f},{1.0f,0.5f,0.75f}
};

GLfloat LightAmbient[]=		{ 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightDiffuse[]=		{ 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat LightSpecular[]=	{ 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightPosition[]=	{ 0.0f, 0.0f, 0.0f, 1.0f };

float fog_r = 211.f/255.f;
float fog_g = 237.f/255.f;
float fog_b = 254.f/255.f;

GLuint	fogMode[]= { GL_EXP, GL_EXP2, GL_LINEAR };	// Storage For Three Types Of Fog
GLuint	fogfilter = 0;								// Which Fog Mode To Use 
GLfloat	fogColor[4] = {fog_r, fog_g, fog_b, 1};		// Fog Color

GLuint	filter;				// Which Filter To Use
GLuint  object=0;			// Which Object To Draw (NEW)

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc


GLuint texturem = 0;

inline AUX_RGBImageRec *LoadBMP(const char *Filename)						// Loads A Bitmap Image
{
	FILE *File=NULL;												// File Handle

	if (!Filename)													// Make Sure A Filename Was Given
	{
		return NULL;												// If Not Return NULL
	}

	File=fopen(Filename,"r");										// Check To See If The File Exists

	if (File)														// Does The File Exist?
	{
		fclose(File);												// Close The Handle
		return auxDIBImageLoad(Filename);							// Load The Bitmap And Return A Pointer
	}

	return NULL;													// If Load Failed Return NULL
}


inline GLuint LoadGLTexture( const char *filename )						// Load Bitmaps And Convert To Textures
{
  bool Status=true;
	AUX_RGBImageRec *pImage;										// Create Storage Space For The Texture

	pImage = LoadBMP( filename );									// Loads The Bitmap Specified By filename

	// Load The Bitmap, Check For Errors, If Bitmap's Not Found Quit
	if ( pImage != NULL && pImage->data != NULL )					// If Texture Image Exists
	{
		glGenTextures(1, &texturem);									// Create The Texture

		// Typical Texture Generation Using Data From The Bitmap
		glBindTexture(GL_TEXTURE_2D, texturem);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, pImage->sizeX, pImage->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, pImage->data);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

		free(pImage->data);											// Free The Texture Image Memory
		free(pImage);												// Free The Image Structure
	}

  return Status;
}

class Model
{
	public:
		//	Mesh
		struct Mesh
		{
			int m_materialIndex;
			int m_numTriangles;
			int *m_pTriangleIndices;
		};

		//	Material properties
		struct Material
		{
			GLfloat m_ambient[4], m_diffuse[4], m_specular[4], m_emissive[4];
			GLfloat m_shininess;
			GLuint m_texture;
			char *m_pTextureFilename;
		};

		//	Triangle structure
		struct Triangle
		{
			GLfloat m_vertexNormals[3][3];
			GLfloat m_s[3], m_t[3];
			int m_vertexIndices[3];
		};

		//	Vertex structure
		struct Vertex
		{
			char m_boneID;	// for skeletal animation
			GLfloat m_location[3];
		};

	public:
		/*	Constructor. */
		Model();

		/*	Destructor. */
		virtual ~Model();

		/*	
			Load the model data into the private variables. 
				filename			Model filename
		*/
		virtual bool loadModelData( const char *filename ) = 0;

		/*
			Draw the model.
		*/
		void draw();
		/*
		    Make a display list from the model
        */
		inline GLuint makeDisplayList();

		/*
			Called if OpenGL context was lost and we need to reload textures, display lists, etc.
		*/
		void reloadTextures();

	protected:
		//	Meshes used
		int m_numMeshes;
		Mesh *m_pMeshes;

		//	Materials used
		int m_numMaterials;
		Material *m_pMaterials;

		//	Triangles used
		int m_numTriangles;
		Triangle *m_pTriangles;

		//	Vertices Used
		int m_numVertices;
		Vertex *m_pVertices;
};


inline Model::Model()
{
	m_numMeshes = 0;
	m_pMeshes = NULL;
	m_numMaterials = 0;
	m_pMaterials = NULL;
	m_numTriangles = 0;
	m_pTriangles = NULL;
	m_numVertices = 0;
	m_pVertices = NULL;
}

inline Model::~Model()
{
	int i;
	for ( i = 0; i < m_numMeshes; i++ )
		delete[] m_pMeshes[i].m_pTriangleIndices;
	for ( i = 0; i < m_numMaterials; i++ )
		delete[] m_pMaterials[i].m_pTextureFilename;

	m_numMeshes = 0;
	if ( m_pMeshes != NULL )
	{
		delete[] m_pMeshes;
		m_pMeshes = NULL;
	}

	m_numMaterials = 0;
	if ( m_pMaterials != NULL )
	{
		delete[] m_pMaterials;
		m_pMaterials = NULL;
	}

	m_numTriangles = 0;
	if ( m_pTriangles != NULL )
	{
		delete[] m_pTriangles;
		m_pTriangles = NULL;
	}

	m_numVertices = 0;
	if ( m_pVertices != NULL )
	{
		delete[] m_pVertices;
		m_pVertices = NULL;
	}
}

inline void Model::draw() 
{
	
	GLboolean texEnabled = glIsEnabled( GL_TEXTURE_2D );

	// Draw by group
	for ( int i = 0; i < m_numMeshes; i++ )
	{
		int materialIndex = m_pMeshes[i].m_materialIndex;
		if ( materialIndex >= 0 )
		{
			glMaterialfv( GL_FRONT, GL_AMBIENT, m_pMaterials[materialIndex].m_ambient );
			glMaterialfv( GL_FRONT, GL_DIFFUSE, m_pMaterials[materialIndex].m_diffuse );
			glMaterialfv( GL_FRONT, GL_SPECULAR, m_pMaterials[materialIndex].m_specular );
			glMaterialfv( GL_FRONT, GL_EMISSION, m_pMaterials[materialIndex].m_emissive );
			glMaterialf( GL_FRONT, GL_SHININESS, m_pMaterials[materialIndex].m_shininess );

			if ( m_pMaterials[materialIndex].m_texture > 0 )
			{
				glBindTexture( GL_TEXTURE_2D, m_pMaterials[materialIndex].m_texture );
				glEnable( GL_TEXTURE_2D );
			}
			else
				glDisable( GL_TEXTURE_2D );
		}
		else
		{
			// Material properties?
			glDisable( GL_TEXTURE_2D );
		}

		glBegin( GL_TRIANGLES );
		{
			for ( int j = 0; j < m_pMeshes[i].m_numTriangles; j++ )
			{
				int triangleIndex = m_pMeshes[i].m_pTriangleIndices[j];
				const Triangle* pTri = &m_pTriangles[triangleIndex];

				for ( int k = 0; k < 3; k++ )
				{
					int index = pTri->m_vertexIndices[k];

					glNormal3fv( pTri->m_vertexNormals[k] );
					glTexCoord2f( pTri->m_s[k], pTri->m_t[k] );
					glVertex3fv( m_pVertices[index].m_location );
				}
			}
		}
		glEnd();
	}

	if ( texEnabled )
		glEnable( GL_TEXTURE_2D );
	else
		glDisable( GL_TEXTURE_2D );
}

inline GLuint Model::makeDisplayList() 
{
	GLuint List = glGenLists(1); 
	glNewList(List,GL_COMPILE);							// Start With The Box List
	GLboolean texEnabled = glIsEnabled( GL_TEXTURE_2D );

	// Draw by group
	for ( int i = 0; i < m_numMeshes; i++ )
	{
		int materialIndex = m_pMeshes[i].m_materialIndex;
		if ( materialIndex >= 0 )
		{
			glMaterialfv( GL_FRONT, GL_AMBIENT, m_pMaterials[materialIndex].m_ambient );
			glMaterialfv( GL_FRONT, GL_DIFFUSE, m_pMaterials[materialIndex].m_diffuse );
			glMaterialfv( GL_FRONT, GL_SPECULAR, m_pMaterials[materialIndex].m_specular );
			glMaterialfv( GL_FRONT, GL_EMISSION, m_pMaterials[materialIndex].m_emissive );
			glMaterialf( GL_FRONT, GL_SHININESS, m_pMaterials[materialIndex].m_shininess );

			if ( m_pMaterials[materialIndex].m_texture > 0 )
			{
				glBindTexture( GL_TEXTURE_2D, m_pMaterials[materialIndex].m_texture );
				glEnable( GL_TEXTURE_2D );
			}
			else
				glDisable( GL_TEXTURE_2D );
		}
		else
		{
			// Material properties?
			glDisable( GL_TEXTURE_2D );
		}

		glBegin( GL_TRIANGLES );
		{
			for ( int j = 0; j < m_pMeshes[i].m_numTriangles; j++ )
			{
				int triangleIndex = m_pMeshes[i].m_pTriangleIndices[j];
				const Triangle* pTri = &m_pTriangles[triangleIndex];

				for ( int k = 0; k < 3; k++ )
				{
					int index = pTri->m_vertexIndices[k];

					glNormal3fv( pTri->m_vertexNormals[k] );
					glTexCoord2f( pTri->m_s[k], pTri->m_t[k] );
					glVertex3fv( m_pVertices[index].m_location );
				}
			}
		}
		glEnd();
	}

	if ( texEnabled )
		glEnable( GL_TEXTURE_2D );
	else
		glDisable( GL_TEXTURE_2D );

   	glEndList();
	return List;
}


inline void Model::reloadTextures()
{
	for ( int i = 0; i < m_numMaterials; i++ )
		if ( strlen( m_pMaterials[i].m_pTextureFilename ) > 0 )
			m_pMaterials[i].m_texture = LoadGLTexture( m_pMaterials[i].m_pTextureFilename );
		else
			m_pMaterials[i].m_texture = 0;
}


class MilkshapeModel : public Model
{
	public:
		/*	Constructor. */
		MilkshapeModel();

		/*	Destructor. */
		virtual ~MilkshapeModel();

		/*	
			Load the model data into the private variables. 
				filename			Model filename
		*/
		virtual bool loadModelData( const char *filename );
};


inline MilkshapeModel::MilkshapeModel()
{
}

inline MilkshapeModel::~MilkshapeModel()
{
}

/* 
	MS3D STRUCTURES 
*/

// byte-align structures
#ifdef _MSC_VER
#	pragma pack( push, packing )
#	pragma pack( 1 )
#	define PACK_STRUCT
#elif defined( __GNUC__ )
#	define PACK_STRUCT	__attribute__((packed))
#else
#	error you must byte-align these structures with the appropriate compiler directives
#endif

typedef unsigned char byte;
typedef unsigned short word;

// File header
struct MS3DHeader
{
	char m_ID[10];
	int m_version;
} PACK_STRUCT;

// Vertex information
struct MS3DVertex
{
	byte m_flags;
	GLfloat m_vertex[3];
	char m_boneID;
	byte m_refCount;
} PACK_STRUCT;

// Triangle information
struct MS3DTriangle
{
	word m_flags;
	word m_vertexIndices[3];
	GLfloat m_vertexNormals[3][3];
	GLfloat m_s[3], m_t[3];
	byte m_smoothingGroup;
	byte m_groupIndex;
} PACK_STRUCT;

// Material information
struct MS3DMaterial
{
    char m_name[32];
    GLfloat m_ambient[4];
    GLfloat m_diffuse[4];
    GLfloat m_specular[4];
    GLfloat m_emissive[4];
    GLfloat m_shininess;	// 0.0f - 128.0f
    GLfloat m_transparency;	// 0.0f - 1.0f
    byte m_mode;	// 0, 1, 2 is unused now
    char m_texture[128];
    char m_alphamap[128];
} PACK_STRUCT;

//	Joint information
struct MS3DJoint
{
	byte m_flags;
	char m_name[32];
	char m_parentName[32];
	GLfloat m_rotation[3];
	GLfloat m_translation[3];
	word m_numRotationKeyframes;
	word m_numTranslationKeyframes;
} PACK_STRUCT;

// Keyframe data
struct MS3DKeyframe
{
	GLfloat m_time;
	GLfloat m_parameter[3];
} PACK_STRUCT;

// Default alignment
#ifdef _MSC_VER
#	pragma pack( pop, packing )
#endif

#undef PACK_STRUCT

inline bool MilkshapeModel::loadModelData( const char *filename )
{
	ifstream inputFile( filename, ios::in | ios::binary | ios::_Nocreate );
	if ( inputFile.fail())
		return false;	// "Couldn't open the model file."

	inputFile.seekg( 0, ios::end );
	long fileSize = inputFile.tellg();
	inputFile.seekg( 0, ios::beg );

	byte *pBuffer = new byte[fileSize];
	inputFile.read( (char*)pBuffer, fileSize );
	inputFile.close();

	const byte *pPtr = pBuffer;
	MS3DHeader *pHeader = ( MS3DHeader* )pPtr;
	pPtr += sizeof( MS3DHeader );

	if ( strncmp( pHeader->m_ID, "MS3D000000", 10 ) != 0 )
		return false; // "Not a valid Milkshape3D model file."

	if ( pHeader->m_version < 3 || pHeader->m_version > 4 )
		return false; // "Unhandled file version. Only Milkshape3D Version 1.3 and 1.4 is supported." );

	int nVertices = *( word* )pPtr; 
	m_numVertices = nVertices;
	m_pVertices = new Vertex[nVertices];
	pPtr += sizeof( word );

	int i;
	for ( i = 0; i < nVertices; i++ )
	{
		MS3DVertex *pVertex = ( MS3DVertex* )pPtr;
		m_pVertices[i].m_boneID = pVertex->m_boneID;
		memcpy( m_pVertices[i].m_location, pVertex->m_vertex, sizeof( GLfloat )*3 );
		pPtr += sizeof( MS3DVertex );
	}

	int nTriangles = *( word* )pPtr;
	m_numTriangles = nTriangles;
	m_pTriangles = new Triangle[nTriangles];
	pPtr += sizeof( word );

	for ( i = 0; i < nTriangles; i++ )
	{
		MS3DTriangle *pTriangle = ( MS3DTriangle* )pPtr;
		int vertexIndices[3] = { pTriangle->m_vertexIndices[0], pTriangle->m_vertexIndices[1], pTriangle->m_vertexIndices[2] };
		GLfloat t[3] = { 1.0f-pTriangle->m_t[0], 1.0f-pTriangle->m_t[1], 1.0f-pTriangle->m_t[2] };
		memcpy( m_pTriangles[i].m_vertexNormals, pTriangle->m_vertexNormals, sizeof( GLfloat )*3*3 );
		memcpy( m_pTriangles[i].m_s, pTriangle->m_s, sizeof( GLfloat )*3 );
		memcpy( m_pTriangles[i].m_t, t, sizeof( GLfloat )*3 );
		memcpy( m_pTriangles[i].m_vertexIndices, vertexIndices, sizeof( int )*3 );
		pPtr += sizeof( MS3DTriangle );
	}

	int nGroups = *( word* )pPtr;
	m_numMeshes = nGroups;
	m_pMeshes = new Mesh[nGroups];
	pPtr += sizeof( word );
	for ( i = 0; i < nGroups; i++ )
	{
		pPtr += sizeof( byte );	// flags
		pPtr += 32;				// name

		word nTriangles = *( word* )pPtr;
		pPtr += sizeof( word );
		int *pTriangleIndices = new int[nTriangles];
		for ( int j = 0; j < nTriangles; j++ )
		{
			pTriangleIndices[j] = *( word* )pPtr;
			pPtr += sizeof( word );
		}

		char materialIndex = *( char* )pPtr;
		pPtr += sizeof( char );
	
		m_pMeshes[i].m_materialIndex = materialIndex;
		m_pMeshes[i].m_numTriangles = nTriangles;
		m_pMeshes[i].m_pTriangleIndices = pTriangleIndices;
	}

	int nMaterials = *( word* )pPtr;
	m_numMaterials = nMaterials;
	m_pMaterials = new Material[nMaterials];
	pPtr += sizeof( word );
	for ( i = 0; i < nMaterials; i++ )
	{
		MS3DMaterial *pMaterial = ( MS3DMaterial* )pPtr;
		memcpy( m_pMaterials[i].m_ambient, pMaterial->m_ambient, sizeof( GLfloat )*4 );
		memcpy( m_pMaterials[i].m_diffuse, pMaterial->m_diffuse, sizeof( GLfloat )*4 );
		memcpy( m_pMaterials[i].m_specular, pMaterial->m_specular, sizeof( GLfloat )*4 );
		memcpy( m_pMaterials[i].m_emissive, pMaterial->m_emissive, sizeof( GLfloat )*4 );
		m_pMaterials[i].m_shininess = pMaterial->m_shininess;
		m_pMaterials[i].m_pTextureFilename = new char[strlen( pMaterial->m_texture )+1];
		strcpy( m_pMaterials[i].m_pTextureFilename, pMaterial->m_texture );
		pPtr += sizeof( MS3DMaterial );
	}

	reloadTextures();

	delete[] pBuffer;

	return true;
}

Model *pModel = NULL;

// Create A Structure For The Timer Information
struct
{
  __int64       frequency;          // Timer Frequency
  GLfloat            resolution;          // Timer Resolution
  unsigned long mm_timer_start;     
  
  // Multimedia Timer Start Value
  unsigned long mm_timer_elapsed;      // Multimedia Timer Elapsed Time
  bool   performance_timer;    
  
  // Using The Performance Timer?
  __int64       performance_timer_start;      // Performance Timer Start Value
  __int64       performance_timer_elapsed; // Performance Timer Elapsed Time
} timer;

// Initialize Our Timer
inline void TimerInit(void)
{
     memset(&timer, 0, sizeof(timer));   
 // Clear Our Timer Structure
     // Check To See If A Performance Counter Is Available
     // If One Is Available The Timer Frequency Will Be Updated
     if (!QueryPerformanceFrequency((LARGE_INTEGER *) &timer.frequency))
     {
          // No Performace Counter Available
          timer.performance_timer = FALSE;                      // Set Performance Timer To FALSE
          timer.mm_timer_start = timeGetTime();                 // Use timeGetTime()
          timer.resolution  = 1.0f/1000.0f;                           // Set Our Timer Resolution To .001f
          timer.frequency   = 1000;                                     // Set Our Timer Frequency To 1000
          timer.mm_timer_elapsed = timer.mm_timer_start; // Set The Elapsed Time
     }
     else
     {
          // Performance Counter Is Available, Use It Instead Of The Multimedia Timer
          // Get The Current Time And Store It In performance_timer_start
          QueryPerformanceCounter((LARGE_INTEGER *) &timer.performance_timer_start);
          timer.performance_timer   = TRUE;    // Set Performance Timer To TRUE
          // Calculate The Timer Resolution Using The Timer Frequency
          timer.resolution    = (GLfloat) (((double)1.0f)/((double)timer.frequency));
          // Set The Elapsed Time To The Current Time
          timer.performance_timer_elapsed = timer.performance_timer_start;
     }
}

// Get Time In Milliseconds
inline GLfloat TimerGetTime()
{
     __int64 time;                                  // 'time' Will Hold A 64 Bit Integer
     if (timer.performance_timer)           // Are We Using The Performance Timer?
     {
          QueryPerformanceCounter((LARGE_INTEGER *) &time); // Current Performance Time
          // Return The Time Elapsed since TimerInit was called
          return ( (GLfloat) ( time - timer.performance_timer_start) * timer.resolution)*1000.0f;
     }
     else
     {
          // Return The Time Elapsed since TimerInit was called
          return ( (GLfloat) ( timeGetTime() - timer.mm_timer_start) * timer.resolution)*1000.0f;
     }
}


inline GLvoid BuildFont(GLvoid)								// Build Our Bitmap Font
{
	HFONT	font;										// Windows Font ID
    base = glGenLists(96);
	
	font = CreateFont(	-14,							// Height Of Font
						0,								// Width Of Font
						0,								// Angle Of Escapement
						0,								// Orientation Angle
						FW_BOLD,						// Font Weight
						FALSE,							// Italic
						FALSE,							// Underline
						FALSE,							// Strikeout
						ANSI_CHARSET,					// Character Set Identifier
						OUT_TT_PRECIS,					// Output Precision
						CLIP_DEFAULT_PRECIS,			// Clipping Precision
						ANTIALIASED_QUALITY,			// Output Quality
						FF_DONTCARE|DEFAULT_PITCH,		// Family And Pitch
						"Verdana");					// Font Name

	SelectObject(hDC, font);							// Selects The Font We Want

	wglUseFontBitmaps(hDC, 32, 96, base);				// Builds 96 Characters Starting At Character 32
}

inline GLvoid KillFont(GLvoid)									// Delete The Font
{
	glDeleteLists(base, 96);							// Delete All 96 Characters
}

inline GLvoid glPrint(const char *fmt, ...)					// Custom GL "Print" Routine
{
	char		text[256];								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	    vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(base - 32);								// Sets The Base Character to 32
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
}


inline int LoadGLTextures()									// Load Bitmap And Convert To A Texture
{
	int Status=FALSE;								// Status Indicator
  AUX_RGBImageRec *TextureImage[1];				// Create Storage Space For The Textures
  memset(TextureImage,0,sizeof(void *)*1);		// Set The Pointer To NULL

  if (TextureImage[0]=LoadBMP("texture/asphalt.bmp"))	// Load Particle Texture
  {
		Status=TRUE;								// Set The Status To TRUE
		glGenTextures(1, &texture[0]);				// Create One Texture
           
    // Create MipMapped Texture
    glBindTexture(GL_TEXTURE_2D, texture[0]);
	  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	  gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);           
  }
  
  if (TextureImage[0]=LoadBMP("texture/sky.bmp"))	// Load Particle Texture
  {
		Status=TRUE;								// Set The Status To TRUE
	  glGenTextures(1, &texture[1]);				// Create One Texture

		// Create MipMapped Texture
 		glBindTexture(GL_TEXTURE_2D, texture[1]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
   	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);           
  }

  if (TextureImage[0]=LoadBMP("texture/Lightmap_256x256.bmp"))	// Load Particle Texture
  {
	  Status=TRUE;								// Set The Status To TRUE
		glGenTextures(1, &texture[2]);				// Create One Texture
		
		// Create MipMapped Texture
 		glBindTexture(GL_TEXTURE_2D, texture[2]);
   	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
   	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);           
  }

  if (TextureImage[0]=LoadBMP("texture/Loading.bmp"))	// Load Particle Texture
  {
		Status=TRUE;								// Set The Status To TRUE
		glGenTextures(1, &texture[3]);				// Create One Texture

    // Create MipMapped Texture
    glBindTexture(GL_TEXTURE_2D, texture[3]);
	  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	  gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);           
  }

  if (TextureImage[0]=LoadBMP("texture/Water.bmp"))	// Load Particle Texture
  {
		Status=TRUE;								// Set The Status To TRUE
		glGenTextures(1, &texture[4]);				// Create One Texture

 		// Create MipMapped Texture
		glBindTexture(GL_TEXTURE_2D, texture[4]);
 		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
 		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);           
  }

  if (TextureImage[0]=LoadBMP("texture/nebula.bmp"))	// Load Particle Texture
  {
		Status=TRUE;								// Set The Status To TRUE
		glGenTextures(1, &texture[5]);				// Create One Texture

 		// Create MipMapped Texture
  	glBindTexture(GL_TEXTURE_2D, texture[5]);
 		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
 		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);           
  }

  if (TextureImage[0]=LoadBMP("texture/Water2.bmp"))	// Load Particle Texture
  {
		Status=TRUE;								// Set The Status To TRUE
		glGenTextures(1, &texture[6]);				// Create One Texture

  	// Create MipMapped Texture
  	glBindTexture(GL_TEXTURE_2D, texture[6]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);           
  }

  if (TextureImage[0])							// If Texture Exists
	{
		if (TextureImage[0]->data)					// If Texture Image Exists
		{
			free(TextureImage[0]->data);			// Free The Texture Image Memory
		}
		free(TextureImage[0]);						// Free The Image Structure
	}

  return Status;									// Return The Status
}


inline GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport
  SCREENWIDTH=width; 
  SCREENHEIGHT=height; 

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.9f,50000.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}


const int MAX = 500; // LENGTH AND WIDTH OF MATRIX(2D array)
const int skyMAX = 50; // LENGTH AND WIDTH OF MATRIX(2D array)

struct vertex
{
	GLfloat x, y, z, light;
};
vertex field[MAX+9][MAX+9];

GLfloat xtrans = MAX/2;
GLfloat xptrans = 0;
GLfloat ytrans = 0;
GLfloat yptrans = 0;
GLfloat ztrans = MAX/2;
GLfloat zptrans = 0;
GLfloat visual_distance = 100;
GLfloat sun_height = 1000;
GLfloat sun_zdistance = -5000;

GLfloat Progress = 0;
BYTE g_HeightMap[MAX*MAX];					// Holds The Height Map Data (NEW)

inline void DrawProgress()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
  glLoadIdentity();
  glTranslatef(0,0,0);

  Progress += 0.01f;   
  
  glColor4f(1,1,1,1);
  
  glBindTexture(GL_TEXTURE_2D, texture[3]);
  glBegin(GL_TRIANGLE_STRIP);
  glLoadIdentity();
  glTexCoord2f(1,1); glVertex3f(1,0-4,-15);
  glTexCoord2f(1,0); glVertex3f(1,-.5-4,-15);
  glTexCoord2f(0,1); glVertex3f(-1,0-4,-15);
  glTexCoord2f(0,0); glVertex3f(-1,-.5-4,-15);
  glEnd();

  glDisable(GL_TEXTURE_2D);
  glColor4f(0,0,1,1);
  glBegin(GL_QUADS);
  glVertex3f(-1.8f,-1.55f,-5.0f);
  glVertex3f(-1.8f,-1.7f,-5.0f);
  glVertex3f(-1.8f+Progress,-1.7f,-5.0f);
  glVertex3f(-1.8f+Progress,-1.55f,-5.0f);
  glEnd(); 
  glEnable(GL_TEXTURE_2D);
}


// Loads The .RAW File And Stores It In pHeightMap
void LoadRawFile(LPSTR strName, int nSize, BYTE *pHeightMap)
{
	FILE *pFile = NULL;

	// Open The File In Read / Binary Mode.
	pFile = fopen( strName, "rb" );

	// Check To See If We Found The File And Could Open It
	if ( pFile == NULL )	
	{
		// Display Error Message And Stop The Function
		MessageBox(NULL, "Can't Find The Height Map!", "Error", MB_OK);
		return;
	}

	fread( pHeightMap, 1, nSize, pFile );

	// After We Read The Data, It's A Good Idea To Check If Everything Read Fine
	int result = ferror( pFile );

	// Check If We Received An Error
	if (result)
	{
		MessageBox(NULL, "Failed To Get Data!", "Error", MB_OK);
	}

	// Close The File.
	fclose(pFile);
}

void SaveLightMap(char *FileName)
{ 
  unsigned int LightMapWidth = MAX;  
  unsigned int LightMapHeight = MAX;
  unsigned char *LightMapImage;
  LightMapImage = new unsigned char[LightMapWidth*LightMapHeight*3];

  int i, i2, i3;

  for (i = 0; i < LightMapWidth; i++)
  {
    for(i2 = 0; i2 < LightMapHeight; i2++)
    {
      for (i3 = 0; i3 < 3; i3++)
      {
  	    if (field[i2][LightMapHeight-i].light > 0)
					LightMapImage[(i*LightMapWidth+i2)*3+i3] = field[i2][LightMapHeight-i].light;
				else
					LightMapImage[(i*LightMapWidth+i2)*3+i3] = 0;
				if (LightMapImage[(i*LightMapWidth+i2)*3+i3] > 200) LightMapImage[(i*LightMapWidth+i2)*3+i3] = 200;
			}
    }
  }

  write24BitBmpFile(FileName, LightMapWidth, LightMapHeight, LightMapImage);

  delete [] LightMapImage;
}

inline void RestoreMyDefaultSettings()
{
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_FOG);
}

inline int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
  useMultitexture=initMultitexture();

	for (loop=0;loop<MAX_PARTICLES;loop++)				
	{
		particle[loop].active=true;						// Make All The Particles Active
		particle[loop].life=1.0f;					// Give It New Life
		particle[loop].fade=GLfloat(rand()%100)/7500 + 0.0075f;	// Random Fade Value
		if (loop < MAX_PARTICLES/2) 
      particle[loop].x= .75f;
		else  
      particle[loop].x= -.75f;
		particle[loop].y= -.15;						// Center On Y Axis
		particle[loop].z= 3;						// Center On Z Axis
    V = (GLfloat((rand()%5))+2)/5;
		Angle = GLfloat(rand()%360);              
    particle[loop].zg = .15;
    particle[loop].xi = sin(Angle) * V;
    particle[loop].yi = cos(Angle) * V;
    particle[loop].zi = ((rand()%10)-5)/5;//V*5;// +(oldzp/8);
	}

	pModel->reloadTextures();

	if (!LoadGLTextures())								// Jump To Texture Loading Routine
	{
		return FALSE;									// If Texture Didn't Load Return FALSE
	}

  TimerInit(); //initialize timer

	glClearColor(fog_r, fog_g, fog_b, 1);			// Black Background
	glClearDepth(1.0f);		   							// Depth Buffer Setup
  
	RestoreMyDefaultSettings();
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
          
  glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);		// Setup The Ambient Light
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);		// Setup The Diffuse Light
	glLightfv(GL_LIGHT1, GL_SPECULAR,LightSpecular);	// Setup The Specular Light
	glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);	// Position The Light   
	glEnable(GL_LIGHT1);								// Enable Light One
	
	glFogi(GL_FOG_MODE, fogMode[2]);			        // Fog Mode
	glFogfv(GL_FOG_COLOR, fogColor);					// Set Fog Color
	glFogf(GL_FOG_DENSITY, 0.294f);						// How Dense Will The Fog Be
	glHint(GL_FOG_HINT, GL_NICEST);					    // Fog Hint Value
	glFogf(GL_FOG_START, 10.0f);						// Fog Start Depth
	glFogf(GL_FOG_END, visual_distance);							// Fog End Depth
	glEnable(GL_FOG);									// Enables GL_FOG

	quadratic=gluNewQuadric();							// Create A Pointer To The Quadric Object (Return 0 If No Memory) (NEW)
	gluQuadricNormals(quadratic, GLU_SMOOTH);			// Create Smooth Normals (NEW)
	gluQuadricTexture(quadratic, GL_TRUE);				// Create Texture Coords (NEW)

  BuildFont();   

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  SwapBuffers(hDC);

  int i,i2;     
    
  DrawProgress();

  field[0][0].y=(GLfloat(rand()%100)-50)/3;

//  GENERATE TERRAIN (IF YOU MODIFY THE SHAPE OF THE TERRAIN, YOU SHOULD RECALCULATE THE LIGHTMAP)
  for (i = 0; i < MAX; i++)
  {     
    for (i2 = 0; i2 < MAX; i2++)
    {
  		if (i<10 || i2<10 || i>MAX-10 || i2>MAX-10)
        field[i][i2].y=0;   
      else
        field[i][i2].y=(GLfloat(rand()%151)-75)/50+(field[i-1][i2-1].y+field[i-1][i2].y+field[i-1][i2+1].y+field[i-1][i2-2].y+field[i-1][i2+2].y)/5.05; //Calculate the y coordinate on the same principle. 				
    }
  }        
// SMOOTH TERRAIN
  for (int cnt = 0; cnt < 3; cnt++)
  {  
    SwapBuffers(hDC);
    DrawProgress();         

    for (int t = 1; t < MAX-1; t++)
    {
      for (int t2 = 1; t2 < MAX-1; t2++)
      {
        field[t][t2].y = (field[t+1][t2].y+field[t][t2-1].y+field[t-1][t2].y+field[t][t2+1].y)/4;           
        
        if (cnt == 0)
        {          
          if (field[t][t2].y < -1 && field[t][t2].y > -1-.5) field[t][t2].y -= .45, field[t][t2].y *= 2;
          else if (field[t][t2].y > -1 && field[t][t2].y < -1+.5) field[t][t2].y += .5, field[t][t2].y /= 5;
        } 
      }
    }
  }

	float sun_x;
	float sun_y;
	float sun_z;
	float sun_xi;
	float sun_yi;
	float sun_zi;
    
	int RAYS_3 = 200;
	int MAX_Rays = RAYS_3*RAYS_3*RAYS_3;
	float xd = -(float)RAYS_3/2;
  float yd = -(float)RAYS_3/2;
  float zd = -(float)RAYS_3/2;
	float sun_dist;
	bool sun_ray_finished;
	int sun_counter;
	
/***********************************************/
/*   RAYCASTING ALGORITHM   ********************/
/***********************************************/
	if (Generate_Sunlight) 
  {
	  for (i = 0; i < MAX_Rays; i++)
	  {
	    sun_x = rand()%MAX;
	    sun_y = sun_height;
	    sun_z = sun_zdistance;
      xd++;

	    if (i % (RAYS_3*RAYS_3) == 0)
	    {
        SwapBuffers(hDC);
        DrawProgress();   
	    }
	    sun_xi = 0;
      sun_yi = -(float(rand()%10000))/10000;
      sun_zi = 1;

	    sun_dist = Hypot(sun_y, sun_z);
	    sun_ray_finished = false;
	    sun_counter = -sun_zdistance*10;
	    while (!sun_ray_finished && sun_counter > 0)
	    {
	      sun_counter--;
			  sun_dist /= 3;
			  if (sun_dist < 1) 
          sun_dist = 1;
			  sun_x += sun_xi*sun_dist;
	      sun_y += sun_yi*sun_dist;
	      sun_z += sun_zi*sun_dist;
 	      if (!(sun_x > 0 && sun_z > sun_zdistance && sun_x < MAX && sun_z < MAX && sun_y < sun_height) || sun_y < -25)
			  {
				  sun_ray_finished = true;
			  }		
			  else if (sun_y < 25 && sun_z > 0)
			  {			 
				  if (sun_y < field[int(sun_x)][int(sun_z)].y) 
				  {
					  field[int(sun_x)][int(sun_z)].light += 80;
					  sun_ray_finished = true;
				  }		  
			  }
	    }
	  }
	  
    SaveLightMap("Lightmap.bmp");
  }

	MODEL = pModel->makeDisplayList();
		
	return TRUE;										// Initialization Went OK
}


GLfloat	xrot=0;				// X Rotation
GLfloat	yrot=0;				// Y Rotation
GLfloat	zrot=0;				// Y Rotation
GLfloat Throttlei;
GLfloat Throttle = 5;
GLfloat _Throttle = Throttle;
GLfloat Speed = Throttle;
GLfloat Speedi;

const GLfloat piover180 = 0.0174532925f;
GLfloat XPOS = -MAX/2;
GLfloat YPOS = 0;
GLfloat ZPOS = -MAX/2;
GLfloat XP=0;
GLfloat YP=0;
GLfloat ZP=0;

GLfloat sceneroty;
GLfloat heading;
GLfloat pitch = 0;
GLfloat yaw = 0;
GLfloat walkbias = 0;
GLfloat walkbiasangle = 0;
GLfloat zprot;
GLfloat yptrans2 = 0;

int quality = 3;

GLfloat H = 0;
GLfloat angle;

GLfloat xdist;
GLfloat zdist;

GLfloat Time1;
GLfloat Time2;
GLfloat DiffTime;
GLfloat FPS = 0;
GLfloat multiplier = 360/(3.14159*2); // multiplier is necessary for conversion from 360 degrees.

GLfloat glow = .4;
GLfloat glowp = 0;

bool has_wireframe_executed = false;

inline int DrawGLScene(GLvoid)									// Here's Where We Do All The Drawing and Animation
{ 
  float ht = SCREENHEIGHT/8;
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);	// Clear The Screen And The Depth Buffer   

  RestoreMyDefaultSettings();

	int i;    
  int i2;    
   
  if (-XPOS < 0) XPOS -= MAX; 
  if (-XPOS > MAX) XPOS += MAX;
  if (-ZPOS < 0) ZPOS -= MAX; 
  if (-ZPOS > MAX) ZPOS += MAX;

  xtrans = -XPOS;
	ztrans = -ZPOS;
    
  yrot = heading;
    
	sceneroty = 360.0f - yrot;
	H = sceneroty;
	if (H > 360) H = 0;
	else if (H < 0) H = 360;

  int xpos = MAX-int(xtrans);
  int zpos = MAX-int(ztrans);
  ytrans = YPOS;

  glLoadIdentity();
  glTranslatef(0,0,-10);
  glRotatef(sceneroty,0,1,0);
  glTranslatef(xtrans,ytrans-3.5-ABS(Speed)/5,ztrans);    
          
  GLfloat xtexa; 
  GLfloat ytexa; 
  GLfloat xtexa2; 
  GLfloat ytexa2; 
  GLfloat xtexb; 
  GLfloat ytexb; 
  GLfloat xtexb2; 
  GLfloat ytexb2; 
    
  int xrange1 = int(MAX-xtrans - visual_distance); 
  int xrange2 = int(MAX-xtrans + visual_distance);
  int zrange1 = int(MAX-ztrans - visual_distance);
  int zrange2 = int(MAX-ztrans + visual_distance);   
  
  if (quality != 1)
  {
    xrange1 /= quality;
    xrange1 *= quality;
    xrange2 /= quality;
    xrange2 *= quality;

    zrange1 /= quality;
    zrange1 *= quality;
    zrange2 /= quality;
    zrange2 *= quality;
  }    

  int t, t2;

  if (wireframe)
  {    
	  glDisable(GL_TEXTURE_2D);
    glColor4f(0.0f,0.0f,0.0f,1.0f);
    for (t = xrange1; t < xrange2; t+=quality)
	  {
      for (t2 = zrange1; t2 < zrange2; t2+=quality)
	  	{
        i = t;
        i2 = t2;
            
        while (i < 0) i += MAX;             
        while (i > MAX) i -= MAX;            
        while (i2 < 0) i2 += MAX;             
        while (i2 > MAX) i2 -= MAX;
        int coord=t-MAX;
        int coord2=t2-MAX;

        glBegin(GL_LINE_LOOP);
        glVertex3f(coord,field[i][i2].y,coord2);        
        glVertex3f(coord+quality,field[i+quality][i2].y,coord2);
        glVertex3f(coord+quality,field[i+quality][i2+quality].y,coord2+quality);
        glVertex3f(coord,field[i][i2+quality].y,coord2+quality);
        glVertex3f(coord+quality,field[i+quality][i2].y,coord2);
        glEnd();
		  }
	  }
    glEnable(GL_TEXTURE_2D);
	  
	  glLoadIdentity();
    glTranslatef(0,-.5f,-10);    
    glRotatef(yaw,0,1,0);
    glRotatef(zprot*15,0,0,1);
    glRotatef(pitch,1,0,0);

	  glEnable(GL_LIGHTING);
    glCallList(MODEL);
    glDisable(GL_LIGHTING);
    has_wireframe_executed = true;
  }
  else
  {   
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
    // IF MULTITEXTURING is supported by your video card, then use it...
    if (useMultitexture)
    { 
			// TEXTURE-UNIT #0		
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, texture[0]);
			// TEXTURE-UNIT #1:
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texture[2]);

			glColor4f(1,1,1,1);
			for (t = xrange1; t < xrange2; t+=quality)
			{             
				for (t2 = zrange1; t2 < zrange2; t2+=quality)
				{                                                      
					i = t;
					i2 = t2;
					while (i < 0) i += MAX;             
					while (i > MAX) i -= MAX;            
					while (i2 < 0) i2 += MAX;             
					while (i2 > MAX) i2 -= MAX;
                    
       		xtexa = (GLfloat(i)/MAX)*57;
					xtexa2 = (GLfloat(i+quality)/MAX)*57;
					ytexa = (GLfloat(i2)/MAX)*57;
					ytexa2 = (GLfloat(i2+quality)/MAX)*57;       
      		xtexb = (GLfloat(i)/MAX);
					xtexb2 = (GLfloat(i+quality)/MAX);
					ytexb = (GLfloat(i2)/MAX);
					ytexb2 = (GLfloat(i2+quality)/MAX);     
					int coord=t-MAX;
					int coord2=t2-MAX;        
        
					glBegin(GL_TRIANGLE_STRIP);
					glMultiTexCoord2fARB(GL_TEXTURE0_ARB,xtexa2, ytexa2); 
					glMultiTexCoord2fARB(GL_TEXTURE1_ARB,xtexb2, ytexb2); 
					glVertex3f(coord+quality,field[i+quality][i2+quality].y,  coord2+quality);
				
					glMultiTexCoord2fARB(GL_TEXTURE0_ARB,xtexa2, ytexa); 
					glMultiTexCoord2fARB(GL_TEXTURE1_ARB,xtexb2, ytexb);
					glVertex3f(coord+quality,field[i+quality][i2].y,coord2); 
				
					glMultiTexCoord2fARB(GL_TEXTURE0_ARB,xtexa, ytexa2); 
					glMultiTexCoord2fARB(GL_TEXTURE1_ARB,xtexb, ytexb2);
					glVertex3f(coord,field[i][i2+quality].y,coord2+quality); 
				
					glMultiTexCoord2fARB(GL_TEXTURE0_ARB,xtexa, ytexa);
					glMultiTexCoord2fARB(GL_TEXTURE1_ARB,xtexb, ytexb); 
					glVertex3f(coord,field[i][i2].y,coord2); 
					glEnd();
				}   
			}
			glActiveTextureARB(GL_TEXTURE1_ARB);		
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);			   
			glDisable(GL_TEXTURE_2D);
			glEnable(GL_TEXTURE_2D);
		}
    else 
    {    
			glColor4f(1,1,1,1);
			glBindTexture(GL_TEXTURE_2D, texture[0]);
			for (t = xrange1; t < xrange2; t+=quality)
			{        
				for (t2 = zrange1; t2 < zrange2; t2+=quality)
				{                                     
          i = t;
          i2 = t2;
            
          while (i < 0) i += MAX;             
          while (i > MAX) i -= MAX;            
          while (i2 < 0) i2 += MAX;             
          while (i2 > MAX) i2 -= MAX;

    	    xtexa = (GLfloat(i)/MAX)*57;
          xtexa2 = (GLfloat(i+quality)/MAX)*57;    
          ytexa = (GLfloat(i2)/MAX)*57;
          ytexa2 = (GLfloat(i2+quality)/MAX)*57;       
          int coord=t-MAX;
          int coord2=t2-MAX;
        
          glBegin(GL_TRIANGLE_STRIP);
          glTexCoord2f(xtexa2,ytexa2);  glVertex3f(coord+quality,field[i+quality][i2+quality].y,  coord2+quality);
          glTexCoord2f(xtexa2,ytexa);   glVertex3f(coord+quality,field[i+quality][i2].y,coord2); 
          glTexCoord2f(xtexa,ytexa2);   glVertex3f(coord,field[i][i2+quality].y,coord2+quality); 
          glTexCoord2f(xtexa,ytexa);   glVertex3f(coord,field[i][i2].y,coord2); 
          glEnd();       
				}   
			}

      glEnable(GL_BLEND);
			glBlendFunc(GL_DST_COLOR, GL_ZERO);
// SECOND PASS OF DRAWING THE LANDSCAPE(fake multitexturing)
      glBindTexture(GL_TEXTURE_2D, texture[2]);
      glColor4f(1,1,1,.5f);
      for (t = xrange1; t < xrange2; t+=quality)
      {   
        for (t2 = zrange1; t2 < zrange2; t2+=quality)
        {               
          i = t;
          i2 = t2;
            
          while (i < 0) i += MAX;             
          while (i > MAX) i -= MAX;            
          while (i2 < 0) i2 += MAX;             
          while (i2 > MAX) i2 -= MAX;

          xtexa = (GLfloat(i)/MAX)*1;
          xtexa2 = (GLfloat(i+quality)/MAX)*1;
          ytexa = (GLfloat(i2)/MAX)*1;
          ytexa2 = (GLfloat(i2+quality)/MAX)*1;       
          int coord=t-MAX;
          int coord2=t2-MAX;
            
          glBegin(GL_TRIANGLE_STRIP);
          glTexCoord2f(xtexa2,ytexa2);  glVertex3f(coord+quality,field[i+quality][i2+quality].y,  coord2+quality);
          glTexCoord2f(xtexa2,ytexa);   glVertex3f(coord+quality,field[i+quality][i2].y,coord2); 
          glTexCoord2f(xtexa,ytexa2);   glVertex3f(coord,field[i][i2+quality].y,coord2+quality); 
          glTexCoord2f(xtexa,ytexa);   glVertex3f(coord,field[i][i2].y,coord2); 
          glEnd();            
        }
      }   
   	  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDisable(GL_BLEND);
		}  
       
		glFrontFace(GL_CW);
		glDisable(GL_CULL_FACE);

		// SKYDOME GENERATED WITH A PRECISELY POSITIONED SPHERE(a shortcut to the real thing)
		glFogf(GL_FOG_START, MAX*2);							// Fog Start Depth
		glFogf(GL_FOG_END, MAX*15);							// Fog End Depth
    glColor4f(1,1,1,1);
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glTranslatef(-xtrans,-ytrans-MAX*48,-ztrans);
    glRotatef(90,1,0,1);
		gluSphere(quadratic,MAX*50,20,20);
		glFogf(GL_FOG_START, 10.0f);						// Fog Start Depth
		glFogf(GL_FOG_END, visual_distance);							// Fog End Depth
	
    if(water)
    { 
      glEnable(GL_BLEND);
  	  glLoadIdentity();
      glTranslatef(0,0,-10);
      glRotatef(sceneroty,0,1,0);
      glTranslatef(xtrans,ytrans-3.5-ABS(Speed)/5,ztrans);
			
	    if (useMultitexture)
			{
				glActiveTextureARB(GL_TEXTURE0_ARB);
				glBindTexture(GL_TEXTURE_2D, texture[2]);
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, texture[4]);

		    xtexa = (GLfloat(xrange1)/MAX)*1;
		    xtexa2 = (GLfloat(xrange2)/MAX)*1;
		    ytexa = (GLfloat(zrange1)/MAX)*1;
		    ytexa2 = (GLfloat(zrange2)/MAX)*1;

		    water_flow += .05f;
				if (water_flow > MAX) water_flow = 0;
				xtexb = (GLfloat(xrange1+water_flow)/MAX)*50;
		    xtexb2 = (GLfloat(xrange2+water_flow)/MAX)*50;
		    ytexb = (GLfloat(zrange1)/MAX)*50;
		    ytexb2 = (GLfloat(zrange2)/MAX)*50;
				glColor4f(1,1,1,.5f);
		
				glBegin(GL_TRIANGLE_STRIP);
				glMultiTexCoord2fARB(GL_TEXTURE0_ARB,xtexa2, ytexa2); 
				glMultiTexCoord2fARB(GL_TEXTURE1_ARB,xtexb2, ytexb2); 
				glVertex3f(xrange2-MAX,-1,zrange2-MAX);
			  glMultiTexCoord2fARB(GL_TEXTURE0_ARB,xtexa2, ytexa); 
				glMultiTexCoord2fARB(GL_TEXTURE1_ARB,xtexb2, ytexb); 
				glVertex3f(xrange2-MAX,-1,zrange1-MAX); 
			  glMultiTexCoord2fARB(GL_TEXTURE0_ARB,xtexa, ytexa2); 
				glMultiTexCoord2fARB(GL_TEXTURE1_ARB,xtexb, ytexb2); 
				glVertex3f(xrange1-MAX,-1,zrange2-MAX); 
		    glMultiTexCoord2fARB(GL_TEXTURE0_ARB,xtexa, ytexa); 
				glMultiTexCoord2fARB(GL_TEXTURE1_ARB,xtexb, ytexb); 
				glVertex3f(xrange1-MAX,-1,zrange1-MAX); 
			  glEnd();

				glActiveTextureARB(GL_TEXTURE1_ARB);		
				glDisable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE0_ARB);			   
				glDisable(GL_TEXTURE_2D);
				glEnable(GL_TEXTURE_2D);
			}
			else
			{   
				glBindTexture(GL_TEXTURE_2D, texture[6]);
				glColor4f(1,1,1,.35f);
				glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2f(xtexa2,ytexa2); glVertex3f(xrange2-MAX,-1,zrange2-MAX);
				glTexCoord2f(xtexa2,ytexa);  glVertex3f(xrange2-MAX,-1,zrange1-MAX); 
				glTexCoord2f(xtexa,ytexa2);  glVertex3f(xrange1-MAX,-1,zrange2-MAX); 
				glTexCoord2f(xtexa,ytexa);   glVertex3f(xrange1-MAX,-1,zrange1-MAX); 
				glEnd();
      }
			glDisable(GL_BLEND);
   
    }         
   
    glColor4f(1,1,1,1);
    glLoadIdentity();
    glTranslatef(0,-.5f,-10);
     
    glRotatef(yaw,0,1,0);
    glRotatef(zprot*15,0,0,1);
    glRotatef(pitch,1,0,0);

		glEnable(GL_LIGHTING);
    glCallList(MODEL);
    glDisable(GL_LIGHTING);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBindTexture(GL_TEXTURE_2D,texture[5]);

    GLfloat exhaust_r, exhaust_g, exhaust_b;
		if (Afterburner)
		{
			exhaust_r = 1;
			exhaust_g = .5f;
			exhaust_b = 0;
		}
    else
		{
			exhaust_r = .5f;
			exhaust_g = .5f;
			exhaust_b = 1;
		}
	
		glowp += .5f-glow;    
		glow += glowp*(ABS(Throttle)/500);
    if (glow > 1) glow = 1;
		else if (glow < .25f) glow = .25f;
		glColor4f(exhaust_r,exhaust_g,exhaust_b,glow);
    float glowsize = 1;
    for (float glowpos = 3; glowpos <= 3.25f; glowpos+=.25f)
    {
		  glowsize -= .175f;
      glBegin(GL_TRIANGLE_STRIP);						// Build Quad From A Triangle Strip
      glTexCoord2f(1,1); glVertex3f(.52+glowsize,-.8f+glowsize,glowpos); // Top Right
		  glTexCoord2f(0,1); glVertex3f(.52-glowsize,-.8f+glowsize,glowpos); // Top Left
			glTexCoord2f(1,0); glVertex3f(.52+glowsize,-.8f-glowsize,glowpos); // Bottom Right
			glTexCoord2f(0,0); glVertex3f(.52-glowsize,-.8f-glowsize,glowpos); // Bottom Left
			glEnd();										// Done Building Triangle Strip
      glBegin(GL_TRIANGLE_STRIP);						// Build Quad From A Triangle Strip
      glTexCoord2f(1,1); glVertex3f(-.52+glowsize,-.8f+glowsize,glowpos); // Top Right
			glTexCoord2f(0,1); glVertex3f(-.52-glowsize,-.8f+glowsize,glowpos); // Top Left
			glTexCoord2f(1,0); glVertex3f(-.52+glowsize,-.8f-glowsize,glowpos); // Bottom Right
			glTexCoord2f(0,0); glVertex3f(-.52-glowsize,-.8f-glowsize,glowpos); // Bottom Left
			glEnd();										// Done Building Triangle Strip
    } 
	
		for (loop=0;loop<MAX_PARTICLES;loop++)					// Loop Through All The Particles
		{       	
			GLfloat x=particle[loop].x;						// Grab Our Particle X Position
			GLfloat y=particle[loop].y;						// Grab Our Particle Y Position
			GLfloat z=particle[loop].z;					// Particle Z Pos + Zoom           

 		  glColor4f(particle[loop].r,particle[loop].g,particle[loop].b,particle[loop].life/2);		  	                  
    
      glBegin(GL_TRIANGLE_STRIP);						// Build Quad From A Triangle Strip
			glTexCoord2f(1,1); glVertex3f(x+0.1f,y+0.1f,z); // Top Right
			glTexCoord2f(0,1); glVertex3f(x-0.1f,y+0.1f,z); // Top Left
			glTexCoord2f(1,0); glVertex3f(x+0.1f,y-0.1f,z); // Bottom Right
			glTexCoord2f(0,0); glVertex3f(x-0.1f,y-0.1f,z); // Bottom Left
			glEnd();										// Done Building Triangle Strip
            
      particle[loop].x+=particle[loop].xi/250;// Move On The X Axis By X Speed
			particle[loop].y+=particle[loop].yi/250;// Move On The Y Axis By Y Speed
			particle[loop].z+=particle[loop].zi/250;// Move On The Z Axis By Z Speed
      particle[loop].xi*=.975f;
      particle[loop].yi*=.975f;
      particle[loop].zi*=.975f;
			particle[loop].zi+=particle[loop].zg;			// Take Pull On Z Axis Into Account
			particle[loop].life-=particle[loop].fade*3;		// Reduce Particles Life By 'Fade'
      if (particle[loop].life < .5f) 
				particle[loop].life*=.975;

			if (particle[loop].life<0.05f)					// If Particle Is Burned Out
			{ 			    
  		  particle[loop].r=exhaust_r;
				particle[loop].g=exhaust_g;
				particle[loop].b=exhaust_b;
				
				particle[loop].life=1.0f;					// Give It New Life
				particle[loop].fade=GLfloat(rand()%100)/2500 + 0.02f;	// Random Fade Value
				if (loop < MAX_PARTICLES/2) 
          particle[loop].x= .52f;						
				else  
          particle[loop].x= -.52f;						

				particle[loop].y= -.8f;						
				particle[loop].z= 3.f;						
        V = (GLfloat((rand()%5))+2)/5;
		    Angle = GLfloat(rand()%360);
              
        particle[loop].xi = sin(Angle) * V;
        particle[loop].yi = cos(Angle) * V;
        particle[loop].zi = ((rand()%10)-5)/5 + Throttle*4;
			}
    } 

    glDisable(GL_FOG);
		glLoadIdentity();
    glRotatef(sceneroty,0,1,0);
	
		float sun_flare_size;
		sun_flare_size = 5000;
		glColor4f(1,.5f,0,.5f);
		glBegin(GL_TRIANGLE_STRIP);						// Build Quad From A Triangle Strip
		glTexCoord2f(1,1); glVertex3f(MAX/2+sun_flare_size,sun_height+sun_flare_size,sun_zdistance); // Top Right
		glTexCoord2f(0,1); glVertex3f(MAX/2-sun_flare_size,sun_height+sun_flare_size,sun_zdistance); // Top Left
		glTexCoord2f(1,0); glVertex3f(MAX/2+sun_flare_size,sun_height-sun_flare_size,sun_zdistance); // Bottom Right
		glTexCoord2f(0,0); glVertex3f(MAX/2-sun_flare_size,sun_height-sun_flare_size,sun_zdistance); // Bottom Left
		glEnd();										// Done Building Triangle Strip
	
		sun_flare_size = 500;
		glColor4f(1,.5f,0,1);
		glBegin(GL_TRIANGLE_STRIP);						// Build Quad From A Triangle Strip
		glTexCoord2f(1,1); glVertex3f(MAX/2+sun_flare_size,sun_height+sun_flare_size,sun_zdistance); // Top Right
		glTexCoord2f(0,1); glVertex3f(MAX/2-sun_flare_size,sun_height+sun_flare_size,sun_zdistance); // Top Left
		glTexCoord2f(1,0); glVertex3f(MAX/2+sun_flare_size,sun_height-sun_flare_size,sun_zdistance); // Bottom Right
		glTexCoord2f(0,0); glVertex3f(MAX/2-sun_flare_size,sun_height-sun_flare_size,sun_zdistance); // Bottom Left
		glEnd();										// Done Building Triangle Strip
  }

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
      
  Time2 = TimerGetTime()/1000;
  DiffTime = ABS(Time2-Time1);
	if (framerate_limit)
  {
 	  while (DiffTime < .015f) // limits framerate to about 65 FPS
    {
		  Sleep(1);
			Time2 = TimerGetTime()/1000;
			DiffTime = ABS(Time2-Time1);				
		}
  }
  Time1 = TimerGetTime()/1000;     
  FPS = 1/(DiffTime);
  glLoadIdentity();
  glTranslatef(0.0f,0.0f,-5.0f);
  glColor4f(0,0,1,1);
  glRasterPos2f(1, 1.7f);
  glPrint("FPS: %3.0f", FPS);

  return TRUE;										// Keep Going
}

inline GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}
    KillFont();
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/
 
inline BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	HINSTANCE	hInstance;				// Holds The Instance Of The Application
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

  SCREENWIDTH=width; 
  SCREENHEIGHT=height; 

	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}
	
	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL,"Program Will Now Close.","ERROR",MB_OK|MB_ICONSTOP);
				return FALSE;									// Return FALSE
			}
		}
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;	// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;						// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;	// Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"OpenGL",							// Class Name
								title,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		32,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	return TRUE;									// Success
}

inline LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
				active=TRUE;						// Program Is Active
			}
			else
			{
				active=FALSE;						// Program Is No Longer Active
			}

			return 0;								// Return To The Message Loop
		}

		case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch (wParam)							// Check System Calls
			{
				case SC_SCREENSAVE:					// Screensaver Trying To Start?
				case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
			break;									// Exit
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

		case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			keys[wParam] = TRUE;					// If So, Mark It As TRUE
			return 0;								// Jump Back
		}

		case WM_KEYUP:								// Has A Key Been Released?
		{
			keys[wParam] = FALSE;					// If So, Mark It As FALSE
			return 0;								// Jump Back
		}

		case WM_SIZE:								// Resize The OpenGL Window
		{
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}


int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
{
	pModel = new MilkshapeModel();									// Memory To Hold The Model
	if ( pModel->loadModelData( "model.ms3d" ) == false )		// Loads The Model And Checks For Errors
	{
		MessageBox( NULL, "Couldn't load the model data\\model.ms3d", "Error", MB_OK | MB_ICONERROR );
		return 0;													// If Model Didn't Load Quit
	}
	
  MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;								// Bool Variable To Exit Loop
   
  fullscreen = true;

  // Ask The User Which Screen Mode They Prefer
	if (MessageBox(NULL,"Would You Like To Run In Fullscreen Mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
	{
		fullscreen = false;							// Windowed Mode
	}

  /*********************************************/ 
	/*  Set Generate_Sunlight to TRUE to  ********/
  /*  recalculate the lightmap for the  ********/
  /*  terrain.  ********************************/
  Generate_Sunlight = false;
  /*********************************************/ 

  // Create Our OpenGL Window
	if (!CreateGLWindow("Terra3D",800,600,32,fullscreen))
	{
		return 0;									// Quit If Window Was Not Created
	}

	while(!done)									// Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=TRUE;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
		{		
      // Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if ((active && !DrawGLScene()) || keys[VK_ESCAPE])	// Active?  Was There A Quit Received?
			{
				done=TRUE;							// ESC or DrawGLScene Signalled A Quit
			}
			else									// Not Time To Quit, Update Screen
			{
				SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
				
				if (keys['1'])
				{
				  _Throttle = 1;
				}
        else if (keys['2'])
				{
				  _Throttle = 2;
				}
        else if (keys['3'])
				{
				  _Throttle = 3;
				}
        else if (keys['4'])
				{
				  _Throttle = 4;
				}
        else if (keys['5'])
				{
				  _Throttle = 5;
				}
        else if (keys['6'])
				{
				  _Throttle = 6;
				}
        else if (keys['7'])
				{
				  _Throttle = 7;
				}
        else if (keys['8'])
				{
				  _Throttle = 8;
				}
        else if (keys['9'])
				{
				  _Throttle = 9;
				}
        else if (keys['0'])
				{				  
				  _Throttle = 15;  
				}
                
				if (keys[VK_UP])
        {
					pitch -= 15 / (ABS(Speed)+1);
			  }
        else if (keys[VK_DOWN])
        {
					pitch += 15 / (ABS(Speed)+1);
			  }

        if (keys[VK_LEFT])
        {
				  zprot += 5/(ABS(Speed)+1);
          Throttle*=.99;               
        }
        else if (keys[VK_RIGHT])
        {
  				zprot -= 5/(ABS(Speed)+1);
          Throttle*=.99;
        }
				
				if (_Throttle == 15)
				{
				  Afterburner = true;
				}
				else 
				{
				  Afterburner = false;
				}

			  zprot*=0.935f;
				heading += zprot/3;
				yaw += zprot/5;
				yaw*=.95f; 

				Throttlei += (_Throttle-Throttle)/10;
				Throttlei *= .9f;
				Throttle += Throttlei/10;
				
				GLfloat MAX_Speed = (sqrt(Throttle+1)) * 10; 
				Speedi += MAX_Speed-Speed;
				Speedi *= .9f;
				Speed += Speedi/1000;
				XP = -(GLfloat)sin(heading*piover180) * Speed;	
				YP = -(GLfloat)sin(pitch*piover180) * Speed;
				ZP = -(GLfloat)cos(heading*piover180) * Speed;
        GLfloat overallspeed = Hypot(Hypot(XP,YP),ZP) / (ABS(Speed)+1);  				
				
				YP *= overallspeed;
				XP *= overallspeed;
				ZP *= overallspeed;

				XPOS += XP/30;
				YPOS += YP/30;
				ZPOS += ZP/30;
                
        if (keys['M'] && !mp)
        {
 				  mp=TRUE;
				  multitexture=!multitexture;                   
        }

				if (!keys['M'])
				{
					mp=FALSE;
				}

        if (keys['L'] && !lp)
        {
 				  lp=TRUE;
				  water=!water;                   
        }

				if (!keys['L'])
				{
					lp=FALSE;
				}

        if (keys['W'] && !wp)
        {
 				  wp=TRUE;
				  wireframe=!wireframe;                   
        }

				if (!keys['W'])
				{
					wp=FALSE;
				}

        if (keys['F'] && !fp)
        {
 				  fp=TRUE;
				  framerate_limit=!framerate_limit;
        }

				if (!keys['F'])
				{
					fp=FALSE;
				}

        if (keys['S']) // HOLD DOWN 'S' TO TAKE CONTINUOUS SCREENSHOTS
        {
  			  SaveScreenshot();
        }

        if (keys[VK_ADD] && !aq)
        {                 
          aq=TRUE;
          if (quality <= 1) quality = 1;
          else quality--;    
        }
        
				if (!keys[VK_ADD])
        {
          aq=FALSE;                    
        }

        if (keys[VK_SUBTRACT] && !sq)
        {                 
          sq=TRUE;
          if (quality >= 8) quality = 8;
          else quality++;    
        }

        if (!keys[VK_SUBTRACT])
        {
          sq=FALSE;                    
        }

        if(keys[VK_NEXT])
        {
					yptrans2+=0.05f;
          if (yptrans2 > 0) yptrans2*=0.9f;
        }

        if(keys[VK_PRIOR])
        {
					yptrans2-=0.05f;
          if (yptrans2 < -12) yptrans2*=0.99f;
        }
			}
		}
	}

	// Shutdown
	KillGLWindow();									// Kill The Window
  return (msg.wParam);							// Exit The Program
}
