#include "EasingUtility.h"

// イージング関数
float EasingUtility::EaseIn(float start, float end, float t) {
	float easedT = t * t;
	return (1.0f - easedT) * start + easedT * end;
}

float EasingUtility::EaseOut(float start, float end, float t) {
	float easedT = 1.0f - powf(1.0f - t, 3.0f);
	return (1.0f - easedT) * start + easedT * end;
}

float EasingUtility::EaseInOut(float start, float end, float t) {
	float easedT = -(cosf(static_cast<float>(M_PI) * t) - 1.0f) / 2.0f;
	return (1.0f - easedT) * start + easedT * end;
}

float EasingUtility::EaseInBack(float start, float end, float t) {
	float s = 1.70158f;
	float c = end - start;
	float easedT = c * (t * t * ((s + 1.0f) * t - s));
	return start + easedT;
}

float EasingUtility::EaseOutBack(float start, float end, float t) {
	float s = 1.70158f;
	float c = end - start;
	t = t - 1.0f;
	float easedT = (t * t * ((s + 1.0f) * t + s) + 1.0f);

	return start + c * easedT;
}