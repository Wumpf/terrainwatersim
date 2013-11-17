#include "PCH.h"
#include "NoiseGenerator.h"
#include "Random.h"


NoiseGenerator::NoiseGenerator()
{
  for(int i=0; i<4096; ++i)
    m_afWhiteNoise[i] = Random::NextFloat(); // real men hate rand
}

float NoiseGenerator::GetValueNoise(const ezVec3& _vCoordinate, int _iLowOctave, int _iHighOctave, float _fPersistence, bool _bPeriodically, ezVec3* _pvGradient)
{
  if( _pvGradient )
    *_pvGradient = ezVec3(0.0f);

  float fRes = 0.0f;
  // Always start with 1 to make transformation to [-1,1] much more easier
  float fAmplitude = 1.0f;
  float fFrequence = float( 1 << _iLowOctave );
  for( int i=_iLowOctave; i<=_iHighOctave; ++i )
  {
    ezVec3 vGrad;
    fRes += fAmplitude * (GetNoise3D(_vCoordinate*fFrequence, _bPeriodically ? int(fFrequence) : 16, _pvGradient ? &vGrad : nullptr )*0.5f+0.5f);
    if( _pvGradient )
      *_pvGradient = *_pvGradient + (fFrequence * fAmplitude) * vGrad;

    fAmplitude *= _fPersistence;
    fFrequence *= 2.0f;
  }

  // Transform to [-1,1]
  //#ifdef _DEBUG
  //	float test = fRes*2.0f*(1.0f-_fPersistence)/(1.0f-fAmplitude)-1.0f;
  //	Assert( test >= -1.0f && test <= 1.0f );
  //#endif
  return fRes*2.0f*(1.0f-_fPersistence)/(1.0f-fAmplitude)-1.0f;
}

float NoiseGenerator::GetNoise3D(const ezVec3& vCoordinate, ezInt32 iPeriod, ezVec3* pvGradient)
{
  EZ_ASSERT(IsPotOf2(iPeriod), "Period must be a power of 2");

  iPeriod = std::min<ezUInt32>( iPeriod, 16 );
  int iPeriodMod = iPeriod-1;

  int iX0 = floor(vCoordinate.x),	iY0 = floor(vCoordinate.y),	iZ0 = floor(vCoordinate.z);
  float fFracX = vCoordinate.x-iX0,		fFracY = vCoordinate.y-iY0,		fFracZ = vCoordinate.z-iZ0;
  iX0 = (iX0%iPeriod + iPeriod)&iPeriodMod;
  iY0 = (iY0%iPeriod + iPeriod)&iPeriodMod;
  iZ0 = (iZ0%iPeriod + iPeriod)&iPeriodMod;
  int iX1 = (iX0+1)&iPeriodMod;
  int iY1 = (iY0+1)&iPeriodMod;
  int iZ1 = (iZ0+1)&iPeriodMod;

  // We need 2 samples per dimension -> 8 samples total
  float s000 = m_afWhiteNoise[ iX0 + 16*( iY0 + 16*iZ0 ) ];
  float s100 = m_afWhiteNoise[ iX1 + 16*( iY0 + 16*iZ0 ) ];
  float s010 = m_afWhiteNoise[ iX0 + 16*( iY1 + 16*iZ0 ) ];
  float s110 = m_afWhiteNoise[ iX1 + 16*( iY1 + 16*iZ0 ) ];
  float s001 = m_afWhiteNoise[ iX0 + 16*( iY0 + 16*iZ1 ) ];
  float s101 = m_afWhiteNoise[ iX1 + 16*( iY0 + 16*iZ1 ) ];
  float s011 = m_afWhiteNoise[ iX0 + 16*( iY1 + 16*iZ1 ) ];
  float s111 = m_afWhiteNoise[ iX1 + 16*( iY1 + 16*iZ1 ) ];

  float u = Smooth(fFracX);
  float v = Smooth(fFracY);
  float w = Smooth(fFracZ);

  //	return Saga::lrp( Saga::lrp( Saga::lrp(s000, s100, u), Saga::lrp(s010, s110, u), v),
  //					  Saga::lrp( Saga::lrp(s001, s101, u), Saga::lrp(s011, s111, u), v), w);

  float uv = u*v;
  float uw = u*w;
  float vw = v*w;

  float k0 = s000;
  float k1 = s100 - s000;
  float k2 = s010 - s000;
  float k3 = s001 - s000;
  float k4 = s110 - s010 - k1;
  float k5 = s000 - s010 - s001 + s011;
  float k6 = - k1 - s001 + s101;
  float k7 = - k4 + s001 - s101 - s011 + s111;

  if( pvGradient )
  {
    float du = SmoothDerivative(fFracX);
    float dv = SmoothDerivative(fFracY);
    float dw = SmoothDerivative(fFracZ);
    pvGradient->x = du * (k1 + k4*v + k6*w + k7*vw);
    pvGradient->y = dv * (k2 + k4*u + k5*w + k7*uw);
    pvGradient->z = dw * (k3 + k6*u + k5*v + k7*uv);
  }

  return k0 + k1*u + k2*v + k3*w + k4*uv + k5*vw + k6*uw + k7*uv*w;
}
