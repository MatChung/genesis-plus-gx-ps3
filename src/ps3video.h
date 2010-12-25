/*
 * PS3Graphics.h
 *
 *  Created on: Oct 22, 2010
 *      Author: Squarepusher2
 */
#ifndef EMULATOR_GRAPHICS_H_
#define EMULATOR_GRAPHICS_H_

#include <string>
#include <vector>

#include <sysutil/sysutil_sysparam.h>
 
#define EMU_DBG_DELAY 2000000

#define DEFAULT_PROGRAM_SOURCE	"/dev_hdd0/game/GENP00001/USRDIR/shaders/stock.cg"

#ifdef EMUDEBUG
	#ifdef PSGL
	#define EMU_DBG(fmt, args...) do {\
	   gl_dprintf(0.1, 0.1, 1.0, fmt, ##args);\
	   sys_timer_usleep(EMU_DBG_DELAY);\
	   } while(0)
	#else
	#define EMU_DBG(fmt, args...) do {\
	   cellDbgFontPrintf(0.1f, 0.1f, DEBUG_FONT_SIZE, RED, fmt, ##args);\
	   sys_timer_usleep(EMU_DBG_DELAY);\
	   } while(0)
	#endif
#else
#define EMU_DBG(fmt, args...) ((void)0)
#endif

// default shader sources
#define DEFAULT_SHADER_FILE "/dev_hdd0/game/GENP00001/USRDIR/shaders/stock.cg"

#define EMU_RENDER_TEXTURE_WIDTH  (320)
#define EMU_RENDER_TEXTURE_HEIGHT (224)
#define EMU_RENDER_BYTE_BY_PIXEL  (4)

#define SCREEN_16_9_ASPECT_RATIO (16.0/9)
#define SCREEN_4_3_ASPECT_RATIO (4.0/3)
#define EMU_ASPECT_RATIO SCREEN_4_3_ASPECT_RATIO

#define EMU_SCREEN_PITCH (EMU_RENDER_TEXTURE_WIDTH * EMU_RENDER_BYTE_BY_PIXEL)

#define SCREEN_RENDER_TEXTURE_WIDTH EMU_RENDER_TEXTURE_WIDTH
#define SCREEN_RENDER_TEXTURE_HEIGHT EMU_RENDER_TEXTURE_HEIGHT
#define SCREEN_REAL_ASPECT_RATIO EMU_ASPECT_RATIO
#define SCREEN_RENDER_TEXTURE_PITCH EMU_SCREEN_PITCH

#include "cellframework/graphics/PSGLGraphics.h"

typedef struct _Vertex
{
	float x;
	float y;
	float z;
} Vertex;

typedef struct _Rect
{
	float x;
	float y;
	float w;
	float h;
} Rect;

typedef struct _TextureCoord
{
	float u;
	float v;
} TextureCoord;

typedef struct _Quad
{
	Vertex v1;
	Vertex v2;
	Vertex v3;
	Vertex v4;
	TextureCoord t1;
	TextureCoord t2;
	TextureCoord t3;
	TextureCoord t4;
} Quad;

class PS3Graphics : public PSGLGraphics
{
	public:
		PS3Graphics();
		~PS3Graphics();

		void Clear();
		void Draw(int width, int height,uint8_t* screen);
		void FlushDbgFont();
		void Swap();
		void Init();
		void Deinit();
		void SetRect(const Rect &view);

		int32_t ChangeResolution(uint32_t resId, uint16_t pal60Hz);
		void SetOverscan(bool overscan, float amount = 0.0);
		int32_t InitCg();
		int32_t LoadFragmentShader(std::string shaderPath);
		void UpdateCgParams(unsigned width, unsigned height, unsigned tex_width, unsigned tex_height);

		void SetAspectRatio(bool keep_aspect);
		void InitScreenQuad(int width, int height);
		void SetSmooth(bool smooth);
		void SetPAL60Hz(bool pal60Hz);
		bool GetPAL60Hz();

		void DeInitDbgFont();
		void InitDbgFont();
		std::string GetFragmentShaderPath() { return _curFragmentShaderPath; }
	private:
		int32_t PSGLInit();
		Quad screenQuad;
		GLuint vbo[2];
		bool overscan;

		bool m_overscan;
		float m_overscan_amount;

		float m_ratio;
		bool m_smooth;
		bool m_pal60Hz;

		uint8_t *gl_buffer;
		uint8_t *vertex_buf;

		std::string _curFragmentShaderPath;

		CGcontext _cgContext;

		CGprogram _vertexProgram;
		CGprogram _fragmentProgram;

		CGparameter _cgpModelViewProj;

		CGparameter _cgpVideoSize;
		CGparameter _cgpTextureSize;
		CGparameter _cgpOutputSize;

		GLuint _cgViewWidth;
		GLuint _cgViewHeight;

		void SetViewports();
};

#endif /* EMULATOR_GRAPHICS_H_ */

