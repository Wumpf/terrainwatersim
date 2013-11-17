#include "PCH.h"
#include "Random.h"

static const int       OR_MT_W		=	34;
static const int       OR_MT_N		=	624;
static const int       OR_MT_M		=	397;
static const int       OR_MT_R		=	31;
static const unsigned	OR_MT_A[2]	=	{ 0, 0x9908B0DF };	
static const int       OR_MT_U		=	11;
static const int       OR_MT_S		=	7;
static const unsigned	OR_MT_B		=	0x9D2C5680;
static const int       OR_MT_T		=	15;
static const unsigned	OR_MT_C		=	0xEFC60000;
static const int       OR_MT_L		=	18;
static const unsigned	OR_MT_LLMASK=	0x7FFFFFFF;
static const unsigned	OR_MT_UMASK	=	0x80000000;

static ezUInt32 g_adwMT[OR_MT_N];
static ezUInt32 g_dwMTIndex = 0;


void Random::Init(ezUInt32 uiSeed)
{
  // fill table
  for( int i=0; i<OR_MT_N; ++i )
  {
    g_adwMT[i] = i%2?uiSeed+i*527:(2135+uiSeed*74111)*i;
  }

  ezUInt32 y;
  for( int i=0; i<OR_MT_N; ++i )
  {
    y = (g_adwMT[i] & OR_MT_UMASK) | (g_adwMT[i+1==OR_MT_N?0:i+1] & OR_MT_LLMASK);
    g_adwMT[i] = g_adwMT[(i+OR_MT_M)%OR_MT_N] ^ (y >> 1) ^ OR_MT_A[y & 1];
  }

  g_dwMTIndex = 0;
}


float Random::NextFloat()
{
  ezUInt32 y;

  ezUInt32 uiNextIndex = ((g_dwMTIndex+1) == OR_MT_N) ? 0 : (g_dwMTIndex+1);
  y = (g_adwMT[g_dwMTIndex] & OR_MT_UMASK) | (g_adwMT[uiNextIndex] & OR_MT_LLMASK);
  g_adwMT[g_dwMTIndex] = g_adwMT[(g_dwMTIndex+OR_MT_M)%OR_MT_N] ^ (y >> 1) ^ OR_MT_A[y & 1];

  y = g_adwMT[g_dwMTIndex];
  g_dwMTIndex = uiNextIndex;
  y ^= y >> OR_MT_U;
  y ^= y << OR_MT_S & OR_MT_B;
  y ^= y << OR_MT_T & OR_MT_C;
  y ^= y >> OR_MT_L;

  // stint scope
  return (float)(y*4.656612874e-10 - 1.0);
}