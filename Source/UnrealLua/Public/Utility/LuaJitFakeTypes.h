#pragma once
#ifdef SOL_LUAJIT
#include <cstdint>
namespace UnrealLua::LuaFakeTypes
{
	/* GCobj reference */
	typedef struct GCRef {
		uint64_t gcptr64;	/* True 64 bit pointer. */
	} GCRef;

	/* Common GC header for all collectable objects. */
#define GCHeader	GCRef nextgc; uint8_t marked; uint8_t gct
	/* This occupies 6 bytes, so use the next 2 bytes for non-32 bit fields. */

	/* Memory reference */
	typedef struct MRef {
		uint64_t ptr64;	/* True 64 bit pointer. */
	} MRef;
	typedef uint32_t MSize;

#define LJ_ALIGN(n)	__declspec(align(n))

#if PLATFORM_LITTLE_ENDIAN
#define LJ_LE			1
#define LJ_BE			0
#define LJ_ENDIAN_SELECT(le, be)	le
#define LJ_ENDIAN_LOHI(lo, hi)		lo hi
#else
#define LJ_LE			0
#define LJ_BE			1
#define LJ_ENDIAN_SELECT(le, be)	be
#define LJ_ENDIAN_LOHI(lo, hi)		hi lo
#endif

	/* 2-slot frame info. */
#define LJ_FR2			1

	/* Frame link. */
	typedef union {
		int32_t ftsz;		/* Frame type and size of previous frame. */
		MRef pcr;		/* Or PC for Lua frames. */
	} FrameLink;

	/* Tagged value. */
	typedef LJ_ALIGN(8) union TValue {
		uint64_t u64;		/* 64 bit pattern overlaps number. */
		lua_Number n;		/* Number object overlaps split tag/value object. */

		GCRef gcr;		/* GCobj reference with tag. */
		int64_t it64;
		struct {
			LJ_ENDIAN_LOHI(
			  int32_t i;	/* Integer value. */
			, uint32_t it;	/* Internal object tag. Must overlap MSW of number. */
			)
		  };
#if LJ_FR2
		int64_t ftsz;		/* Frame type and size of previous frame, or PC. */
#else
		struct {
			LJ_ENDIAN_LOHI(
			  GCRef func;	/* Function for next frame (or dummy L). */
			, FrameLink tp;	/* Link to previous frame. */
			)
		  } fr;
#endif
		struct {
			LJ_ENDIAN_LOHI(
			  uint32_t lo;	/* Lower 32 bits of number. */
			, uint32_t hi;	/* Upper 32 bits of number. */
			)
		  } u32;
	} TValue;

	enum {
		FRAME_LUA, FRAME_C, FRAME_CONT, FRAME_VARG,
		FRAME_LUAP, FRAME_CP, FRAME_PCALL, FRAME_PCALLH
	  };

	/* Per-thread state object. */
	struct lua_State {
		GCHeader;
		uint8_t dummy_ffid;	/* Fake FF_C for curr_funcisL() on dummy frames. */
		uint8_t status;	/* Thread status. */
		MRef glref;		/* Link to global state. */
		GCRef gclist;		/* GC chain. */
		TValue *base;		/* Base of currently executing function. */
		TValue *top;		/* First free slot in the stack. */
		MRef maxstack;	/* Last free slot in the stack. */
		MRef stack;		/* Stack base. */
		GCRef openupval;	/* List of open upvalues in the stack. */
		GCRef env;		/* Thread environment (table of globals). */
		void *cframe;		/* End of C stack frame chain. */
		MSize stacksize;	/* True stack size (incl. LJ_STACK_EXTRA). */
	};

	/* Types for handling bytecodes. Need this here, details in lj_bc.h. */
	typedef uint32_t BCIns;  /* Bytecode instruction. */
	typedef uint32_t BCPos;  /* Bytecode position. */
	typedef uint32_t BCReg;  /* Bytecode register. */
	typedef int32_t BCLine;  /* Bytecode line number. */
#define FRAME_TYPE		3


#define frame_ftsz(f)		((ptrdiff_t)(f)->ftsz)
#define frame_type(f)		(frame_ftsz(f) & FRAME_TYPE)
#define frame_islua(f)		(frame_type(f) == FRAME_LUA)
#define bc_b(i)		((BCReg)((i)>>24))
#define frame_pc(f)		((const BCIns *)frame_ftsz(f))

	/* Return number of results wanted by caller. */
	static ptrdiff_t results_wanted(lua_State *L)
	{
		//L->base is the currently executing function
		//L->base-1 is the calling function
		TValue *frame = L->base-1;
		if (frame_islua(frame))
		{
			return (ptrdiff_t)bc_b(frame_pc(frame)[-1]) - 1;
		}
		else
		{
			return -1;
		}
	}
}
#endif