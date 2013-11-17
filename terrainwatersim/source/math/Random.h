#pragma once

class Random
{
public:
  /// Initialize Mersenne Twister
  static void Init(ezUInt32 uiSeed);

  /// Returns a random number between -1 and 1 (inclusive)
  static float NextFloat();
};

