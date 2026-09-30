#pragma once
#define _USE_MATH_DEFINES
#include<math.h>
#include <stdlib.h>
#include <time.h>

class EasingUtility {

public:

	// イージング関数
	static float EaseIn(float start, float end, float t);

	static float EaseOut(float start, float end, float t);

	static float EaseInOut(float start, float end, float t);

	static float EaseInBack(float start, float end, float t);

	static float EaseOutBack(float start, float end, float t);
};
