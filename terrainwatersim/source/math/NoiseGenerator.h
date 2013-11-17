#pragma once

// Big thx to JoJendersie.de for sharing his code :)

class NoiseGenerator
{
public:
  NoiseGenerator();

  /// \brief Sample a value noise at a 3D position.
  /// \details The domain goes from 0.0 to 1.0 outside the coordinates are
  ///		wrapped.
  ///
  ///		A periodicity only occures if the octaves are at least 4.
  ///	\param [out,optional] _pvGradient nullptr or a vector to capture the gradient.
  /// \return A value in the range from -1 to 1.
  float GetValueNoise(const ezVec3& vCoordinate, int iLowOctave, int iHighOctave, float fPersistence, bool bPeriodically, ezVec3* pvGradient);

private:
  float GetNoise3D(const ezVec3& vCoordinate, ezInt32 iPeriod, ezVec3* pvGradient);

  float Smooth(float f)					    { return f*f*f*(f*(f*6.0f-15.0f)+10.0f); }
  float SmoothDerivative(float f)		{ return f*f*(f*(f-2.0f)+1.0f)*30.0f; }
  inline int floor(const float a)		{ int r=(int)a; return r - (int)((a<0)&&(a-r!=0.0f)); }

  // Fast method to check wether a number is a potence of two (2^x=n) or not.
  inline bool IsPotOf2(ezUInt32 x)
  {
    // The second is said to be faster than the first variant
    //return (x != 0) && ((x & (x - 1)) == 0);
    return (x>=1) & ((x&(x-1)) < 1);
  }


  float m_afWhiteNoise[4096];
};

