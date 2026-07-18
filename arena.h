#ifndef ARENA_
#define ARENA_


#include <memoryapi.h>
#if defined(__clang__)
#define COMPILER_CLANG 1
#define COMPILER_MSVC 0
#define COMPILER_GCC 0

#elif defined(_MSC_VER)
#define COMPILER_CLANG 0
#define COMPILER_MSVC 1
#define COMPILER_GCC 0

#elif defined(__GNUC__)
#define COMPILER_CLANG 0
#define COMPILER_MSVC 0
#define COMPILER_GCC 1

#else
#define COMPILER_CLANG 0
#define COMPILER_MSVC 0
#define COMPILER_GCC 0
#error "Unknown compiler!"

#endif

#if defined(__cplusplus)
#define LANG_CPP 1
#define LANG_C 0
#else
#define LANG_CPP 0
#define LANG_C 1
#endif


#if defined(_WIN32)
#define OS_WINDOWS 1
#else
#define OS_WINDOWS 0
#error "Currently only for Windows!"
#endif

#if OS_WINDOWS
#define NOGDI
#define NOUSER
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#define global static


// to suppress unused function warning

#if COMPILER_CLANG || COMPILER_GCC
#define internal static __attribute__((unused))
#elif COMPILER_MSVC
#define internal __pragma(warning(suppress : 4505)) static
#else
#define internal static
#endif

#if COMPILER_MSVC
#define force_inline __forceinline
#elif COMPILER_CLANG || COMPILER_GCC
#define force_inline __attribute__((always_inline))
#else
#define force_inline
#endif

#if COMPILER_MSVC
#define no_inline __declspec(noinline)
#elif COMPILER_CLANG || COMPILER_GCC
#define no_inline __attribute__((noinline))
#else
#define no_inline
#endif


/* Units */
#define KB(n) (((U64)(n)) << 10)
#define MB(n) (((U64)(n)) << 20)
#define GB(n) (((U64)(n)) << 30)
#define TB(n) (((U64)(n)) << 40)

/* pow2 math */
#define is_pow2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
#define is_pow2_or_zero(x) (((x) & ((x) - 1)) == 0)

#define align_up_pow2(x, p) (((x) + ((p) - 1)) & ~((p) - 1))
#define align_down_pow2(x, p) ((x) & ~((p) - 1))

/* Clamp */
#define clamp_top(val, high) (((val) < (high)) ? (val) : (high))
#define clamp_bottom(val, low) (((val) > (low)) ? (val) : (low))
#define clamp(val, low, high) (clamp_bottom(low, clamp_top(val, high)))

/* Alignment */
#if COMPILER_MSVC
#define align_of(T) __alignof(T)
#elif COMPILER_CLANG
#define align_of(T) __alignof(T)
#elif COMPILER_GCC
#define align_of(T) __alignof__(T)
#else
#error align_of not defined for this compiler.
#endif

#if COMPILER_MSVC
#define align_to(x) __declspec(align(x))
#elif COMPILER_CLANG || COMPILER_GCC
#define align_to(x) __attribute__((aligned(x)))
#else
#error align_to not defined for this compiler.
#endif

/* Scope Wrappers */
#define scope(it, begin, end) for ((it) = ((begin), 1); (it); (it) = 0, (end)) /* `it` is used internaly. */

#define OPTIONS(T, ...)                                                                                                                                   \
	typedef struct T {                                                                                                                                    \
		__VA_ARGS__                                                                                                                                       \
	} T

/*  Base Type  */
typedef signed char	i8;
typedef short       i16;
typedef int         i32;
typedef long long   i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float  f32;
typedef double f64;


#include <stdio.h>


/* ======================================================================= */
/*                              decelaration                               */
/* ======================================================================= */
#ifdef LANG_CPP 
extern "C" {
#endif
/* .... structs .... */

typedef struct ArenaTempNode{
    struct ArenaTempNode* next;
    u64 start_cursor;
}ArenaTempNode;

typedef struct Arena{
    u64 cursor;
    u64 committed;
    u64 reserved;
    ArenaTempNode* temp_stack_tail;
    ArenaTempNode* temp_stack_head;
}Arena;

#define ARENA_HEADER_SIZE align_up_pow2(sizeof(Arena), 64)

/* .... global var .... */
global u64 global_page_size;


/* .... function .... */

OPTIONS(ArenaOpt, u64 commit_size;);
internal Arena* arena_alloc(u64 reserve_size, ArenaOpt opt);
internal void* arena_push(Arena* arena, u64 size, u64 alignment);
internal void arena_release(Arena* arena);
internal void arena_reset(Arena* arena);

#define arena_push_type(arena, T) arena_push(arena, sizeof(T), align_of(T))
#define arena_push_array(arena, T, count) arena_push(arena, (count * sizeof(T)), align_of(T))

#ifdef LANG_CPP
}
#endif

/* ======================================================================= */
/*                             implementation                              */
/* ======================================================================= */

/* for now */ #define ARENA_IMPLEMENTATION

#ifdef ARENA_IMPLEMENTATION

internal Arena* arena_alloc(u64 reserve_size, ArenaOpt opt){
#if LANG_C
	SYSTEM_INFO sys_info = {};
#else
	SYSTEM_INFO sys_info = {0};
#endif
	Arena* arena;
	u64	   page_size;
	void*  memory;
	if(page_size == 0){
        GetSystemInfo(&sys_info);
		page_size = sys_info.dwPageSize;
	}
    
    reserve_size    = align_up_pow2(reserve_size, page_size);
    // by default commit just one page
    opt.commit_size = (opt.commit_size) ? align_up_pow2(opt.commit_size, page_size) : page_size;
    
    memory = VirtualAlloc(0, reserve_size, MEM_RESERVE, PAGE_READWRITE);
    VirtualAlloc(memory, opt.commit_size, MEM_COMMIT, PAGE_READWRITE);

	arena = (Arena*)memory;

	arena->cursor		   = ARENA_HEADER_SIZE;
	arena->committed	   = opt.commit_size;
	arena->reserved		   = reserve_size;
	arena->temp_stack_tail = 0;
	arena->temp_stack_head = 0;

	return arena;
}

internal void* arena_push(Arena *arena, u64 size, u64 alignment){
    u64 begin;
    u64 end;
    void* user_ptr;

    if(!arena)
        fprintf(stderr, "ERROR: Arena is null\n");
    
    begin = align_up_pow2(arena->cursor, alignment);
    end = begin + size;

    if(end > arena->committed && end <= arena->reserved){
        u64 required_size = end - arena->committed;
        required_size = align_up_pow2(required_size, global_page_size);
        arena->committed += required_size;
        VirtualAlloc((u8*)arena + arena->committed, required_size, MEM_COMMIT, PAGE_READWRITE);
    }

    arena->cursor += end;
    user_ptr = (u8*)arena + begin;
    return user_ptr;
}

internal void arena_reset(Arena* arena) {
	arena->cursor		   = ARENA_HEADER_SIZE;
	arena->temp_stack_head = 0;
	arena->temp_stack_tail = 0;
}

internal void arena_release(Arena *arena){
    VirtualFree(arena, 0, MEM_RELEASE);
}

#endif
#endif
