#pragma once

static const float DEFAULT_BOW_MASS = 1.0f;
static const float DEFAULT_ARROW_MASS = 0.1f;
static const float DEFAULT_K = 4500.0f;
static const float DEFAULT_GRAVITY = 49.05f;
static const float DEFAULT_SENSITIVITY_X = 2.0f;
static const float DEFAULT_SENSITIVITY_Y = 1.25f;

static const float MIN_X = -1.0f;
static const float MAX_X = 1.0f;
static const float MIN_Y = -0.5f;
static const float MAX_Y = 0.75f;

static const float MAX_DRAW = 0.5f;
static const float DRAWBACK_TIME = 2000.0f;	// Time to fully draw back bow in ms.

static const float ARROW_LENGTH = 0.9f;
static const float MIDDLE_ARROWHEAD_DISTANCE = 0.33f;

static const float TARGET_TRANS_TIME = 250.0f; // Time to show and hide target in ms.

static const float GROUND_LEVEL = -3.0f;

static const float MAX_TIME_INTERVAL = 5.0f;	// Max time allowed to update direction
