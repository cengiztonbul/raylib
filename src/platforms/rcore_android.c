/**********************************************************************************************
*
*   rcore_android - Functions to manage window, graphics device and inputs
*
*   PLATFORM: ANDROID
*       - Android (ARM, ARM64)
*
*   LIMITATIONS:
*       - Limitation 01
*       - Limitation 02
*
*   POSSIBLE IMPROVEMENTS:
*       - Improvement 01
*       - Improvement 02
*
*   ADDITIONAL NOTES:
*       - RAYLIB_TRACELOG() function is located in raylib [utils] module
*
*   CONFIGURATION:
*       #define RCORE_PLATFORM_CUSTOM_FLAG
*           Custom flag for rcore on target platform -not used-
*
*   DEPENDENCIES:
*       - Android NDK: Provides C API to access Android functionality
*       - gestures: Gestures system for touch-ready devices (or simulated from mouse inputs)
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2013-2024 Ramon Santamaria (@raysan5) and contributors
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include <android_native_app_glue.h>    // Required for: android_app struct and activity management
#include <android/window.h>             // Required for: AWINDOW_FLAG_FULLSCREEN definition and others
//#include <android/sensor.h>           // Required for: Android sensors functions (accelerometer, gyroscope, light...)
#include <jni.h>                        // Required for: JNIEnv and JavaVM [Used in RaylibOpenURL()]

#include <EGL/egl.h>                    // Native platform windowing system interface

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct {
    // Application data
    struct android_app *app;            // Android activity
    struct android_poll_source *source; // Android events polling source
    bool appEnabled;                    // Flag to detect if app is active ** = true
    bool contextRebindRequired;         // Used to know context rebind required

    // Display data
    EGLDisplay device;                  // Native display device (physical screen connection)
    EGLSurface surface;                 // Surface to draw on, framebuffers (connected to context)
    EGLContext context;                 // Graphic context, mode in which drawing can be done
    EGLConfig config;                   // Graphic config
} PlatformData;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
extern CoreData CORE;                   // Global CORE state context

static PlatformData platform = { 0 };   // Platform specific data

//----------------------------------------------------------------------------------
// Local Variables Definition
//----------------------------------------------------------------------------------
#define KEYCODE_MAP_SIZE 162
static const RaylibKeyboardKey KeycodeMap[KEYCODE_MAP_SIZE] = {
    RAYLIB_KEY_NULL,           // AKEYCODE_UNKNOWN
    0,                  // AKEYCODE_SOFT_LEFT
    0,                  // AKEYCODE_SOFT_RIGHT
    0,                  // AKEYCODE_HOME
    RAYLIB_KEY_BACK,           // AKEYCODE_BACK
    0,                  // AKEYCODE_CALL
    0,                  // AKEYCODE_ENDCALL
    RAYLIB_KEY_ZERO,           // AKEYCODE_0
    RAYLIB_KEY_ONE,            // AKEYCODE_1
    RAYLIB_KEY_TWO,            // AKEYCODE_2
    RAYLIB_KEY_THREE,          // AKEYCODE_3
    RAYLIB_KEY_FOUR,           // AKEYCODE_4
    RAYLIB_KEY_FIVE,           // AKEYCODE_5
    RAYLIB_KEY_SIX,            // AKEYCODE_6
    RAYLIB_KEY_SEVEN,          // AKEYCODE_7
    RAYLIB_KEY_EIGHT,          // AKEYCODE_8
    RAYLIB_KEY_NINE,           // AKEYCODE_9
    0,                  // AKEYCODE_STAR
    0,                  // AKEYCODE_POUND
    RAYLIB_KEY_UP,             // AKEYCODE_DPAD_UP
    RAYLIB_KEY_DOWN,           // AKEYCODE_DPAD_DOWN
    RAYLIB_KEY_LEFT,           // AKEYCODE_DPAD_LEFT
    RAYLIB_KEY_RIGHT,          // AKEYCODE_DPAD_RIGHT
    0,                  // AKEYCODE_DPAD_CENTER
    RAYLIB_KEY_VOLUME_UP,      // AKEYCODE_VOLUME_UP
    RAYLIB_KEY_VOLUME_DOWN,    // AKEYCODE_VOLUME_DOWN
    0,                  // AKEYCODE_POWER
    0,                  // AKEYCODE_CAMERA
    0,                  // AKEYCODE_CLEAR
    RAYLIB_KEY_A,              // AKEYCODE_A
    RAYLIB_KEY_B,              // AKEYCODE_B
    RAYLIB_KEY_C,              // AKEYCODE_C
    RAYLIB_KEY_D,              // AKEYCODE_D
    RAYLIB_KEY_E,              // AKEYCODE_E
    RAYLIB_KEY_F,              // AKEYCODE_F
    RAYLIB_KEY_G,              // AKEYCODE_G
    RAYLIB_KEY_H,              // AKEYCODE_H
    RAYLIB_KEY_I,              // AKEYCODE_I
    RAYLIB_KEY_J,              // AKEYCODE_J
    RAYLIB_KEY_K,              // AKEYCODE_K
    RAYLIB_KEY_L,              // AKEYCODE_L
    RAYLIB_KEY_M,              // AKEYCODE_M
    RAYLIB_KEY_N,              // AKEYCODE_N
    RAYLIB_KEY_O,              // AKEYCODE_O
    RAYLIB_KEY_P,              // AKEYCODE_P
    RAYLIB_KEY_Q,              // AKEYCODE_Q
    RAYLIB_KEY_R,              // AKEYCODE_R
    RAYLIB_KEY_S,              // AKEYCODE_S
    RAYLIB_KEY_T,              // AKEYCODE_T
    RAYLIB_KEY_U,              // AKEYCODE_U
    RAYLIB_KEY_V,              // AKEYCODE_V
    RAYLIB_KEY_W,              // AKEYCODE_W
    RAYLIB_KEY_X,              // AKEYCODE_X
    RAYLIB_KEY_Y,              // AKEYCODE_Y
    RAYLIB_KEY_Z,              // AKEYCODE_Z
    RAYLIB_KEY_COMMA,          // AKEYCODE_COMMA
    RAYLIB_KEY_PERIOD,         // AKEYCODE_PERIOD
    RAYLIB_KEY_LEFT_ALT,       // AKEYCODE_ALT_LEFT
    RAYLIB_KEY_RIGHT_ALT,      // AKEYCODE_ALT_RIGHT
    RAYLIB_KEY_LEFT_SHIFT,     // AKEYCODE_SHIFT_LEFT
    RAYLIB_KEY_RIGHT_SHIFT,    // AKEYCODE_SHIFT_RIGHT
    RAYLIB_KEY_TAB,            // AKEYCODE_TAB
    RAYLIB_KEY_SPACE,          // AKEYCODE_SPACE
    0,                  // AKEYCODE_SYM
    0,                  // AKEYCODE_EXPLORER
    0,                  // AKEYCODE_ENVELOPE
    RAYLIB_KEY_ENTER,          // AKEYCODE_ENTER
    RAYLIB_KEY_BACKSPACE,      // AKEYCODE_DEL
    RAYLIB_KEY_GRAVE,          // AKEYCODE_GRAVE
    RAYLIB_KEY_MINUS,          // AKEYCODE_MINUS
    RAYLIB_KEY_EQUAL,          // AKEYCODE_EQUALS
    RAYLIB_KEY_LEFT_BRACKET,   // AKEYCODE_LEFT_BRACKET
    RAYLIB_KEY_RIGHT_BRACKET,  // AKEYCODE_RIGHT_BRACKET
    RAYLIB_KEY_BACKSLASH,      // AKEYCODE_BACKSLASH
    RAYLIB_KEY_SEMICOLON,      // AKEYCODE_SEMICOLON
    RAYLIB_KEY_APOSTROPHE,     // AKEYCODE_APOSTROPHE
    RAYLIB_KEY_SLASH,          // AKEYCODE_SLASH
    0,                  // AKEYCODE_AT
    0,                  // AKEYCODE_NUM
    0,                  // AKEYCODE_HEADSETHOOK
    0,                  // AKEYCODE_FOCUS
    0,                  // AKEYCODE_PLUS
    RAYLIB_KEY_MENU,           // AKEYCODE_MENU
    0,                  // AKEYCODE_NOTIFICATION
    0,                  // AKEYCODE_SEARCH
    0,                  // AKEYCODE_MEDIA_PLAY_PAUSE
    0,                  // AKEYCODE_MEDIA_STOP
    0,                  // AKEYCODE_MEDIA_NEXT
    0,                  // AKEYCODE_MEDIA_PREVIOUS
    0,                  // AKEYCODE_MEDIA_REWIND
    0,                  // AKEYCODE_MEDIA_FAST_FORWARD
    0,                  // AKEYCODE_MUTE
    RAYLIB_KEY_PAGE_UP,        // AKEYCODE_PAGE_UP
    RAYLIB_KEY_PAGE_DOWN,      // AKEYCODE_PAGE_DOWN
    0,                  // AKEYCODE_PICTSYMBOLS
    0,                  // AKEYCODE_SWITCH_CHARSET
    0,                  // AKEYCODE_BUTTON_A
    0,                  // AKEYCODE_BUTTON_B
    0,                  // AKEYCODE_BUTTON_C
    0,                  // AKEYCODE_BUTTON_X
    0,                  // AKEYCODE_BUTTON_Y
    0,                  // AKEYCODE_BUTTON_Z
    0,                  // AKEYCODE_BUTTON_L1
    0,                  // AKEYCODE_BUTTON_R1
    0,                  // AKEYCODE_BUTTON_L2
    0,                  // AKEYCODE_BUTTON_R2
    0,                  // AKEYCODE_BUTTON_THUMBL
    0,                  // AKEYCODE_BUTTON_THUMBR
    0,                  // AKEYCODE_BUTTON_START
    0,                  // AKEYCODE_BUTTON_SELECT
    0,                  // AKEYCODE_BUTTON_MODE
    RAYLIB_KEY_ESCAPE,         // AKEYCODE_ESCAPE
    RAYLIB_KEY_DELETE,         // AKEYCODE_FORWARD_DELL
    RAYLIB_KEY_LEFT_CONTROL,   // AKEYCODE_CTRL_LEFT
    RAYLIB_KEY_RIGHT_CONTROL,  // AKEYCODE_CTRL_RIGHT
    RAYLIB_KEY_CAPS_LOCK,      // AKEYCODE_CAPS_LOCK
    RAYLIB_KEY_SCROLL_LOCK,    // AKEYCODE_SCROLL_LOCK
    RAYLIB_KEY_LEFT_SUPER,     // AKEYCODE_META_LEFT
    RAYLIB_KEY_RIGHT_SUPER,    // AKEYCODE_META_RIGHT
    0,                  // AKEYCODE_FUNCTION
    RAYLIB_KEY_PRINT_SCREEN,   // AKEYCODE_SYSRQ
    RAYLIB_KEY_PAUSE,          // AKEYCODE_BREAK
    RAYLIB_KEY_HOME,           // AKEYCODE_MOVE_HOME
    RAYLIB_KEY_END,            // AKEYCODE_MOVE_END
    RAYLIB_KEY_INSERT,         // AKEYCODE_INSERT
    0,                  // AKEYCODE_FORWARD
    0,                  // AKEYCODE_MEDIA_PLAY
    0,                  // AKEYCODE_MEDIA_PAUSE
    0,                  // AKEYCODE_MEDIA_CLOSE
    0,                  // AKEYCODE_MEDIA_EJECT
    0,                  // AKEYCODE_MEDIA_RECORD
    RAYLIB_KEY_F1,             // AKEYCODE_F1
    RAYLIB_KEY_F2,             // AKEYCODE_F2
    RAYLIB_KEY_F3,             // AKEYCODE_F3
    RAYLIB_KEY_F4,             // AKEYCODE_F4
    RAYLIB_KEY_F5,             // AKEYCODE_F5
    RAYLIB_KEY_F6,             // AKEYCODE_F6
    RAYLIB_KEY_F7,             // AKEYCODE_F7
    RAYLIB_KEY_F8,             // AKEYCODE_F8
    RAYLIB_KEY_F9,             // AKEYCODE_F9
    RAYLIB_KEY_F10,            // AKEYCODE_F10
    RAYLIB_KEY_F11,            // AKEYCODE_F11
    RAYLIB_KEY_F12,            // AKEYCODE_F12
    RAYLIB_KEY_NUM_LOCK,       // AKEYCODE_NUM_LOCK
    RAYLIB_KEY_KP_0,           // AKEYCODE_NUMPAD_0
    RAYLIB_KEY_KP_1,           // AKEYCODE_NUMPAD_1
    RAYLIB_KEY_KP_2,           // AKEYCODE_NUMPAD_2
    RAYLIB_KEY_KP_3,           // AKEYCODE_NUMPAD_3
    RAYLIB_KEY_KP_4,           // AKEYCODE_NUMPAD_4
    RAYLIB_KEY_KP_5,           // AKEYCODE_NUMPAD_5
    RAYLIB_KEY_KP_6,           // AKEYCODE_NUMPAD_6
    RAYLIB_KEY_KP_7,           // AKEYCODE_NUMPAD_7
    RAYLIB_KEY_KP_8,           // AKEYCODE_NUMPAD_8
    RAYLIB_KEY_KP_9,           // AKEYCODE_NUMPAD_9
    RAYLIB_KEY_KP_DIVIDE,      // AKEYCODE_NUMPAD_DIVIDE
    RAYLIB_KEY_KP_MULTIPLY,    // AKEYCODE_NUMPAD_MULTIPLY
    RAYLIB_KEY_KP_SUBTRACT,    // AKEYCODE_NUMPAD_SUBTRACT
    RAYLIB_KEY_KP_ADD,         // AKEYCODE_NUMPAD_ADD
    RAYLIB_KEY_KP_DECIMAL,     // AKEYCODE_NUMPAD_DOT
    0,                  // AKEYCODE_NUMPAD_COMMA
    RAYLIB_KEY_KP_ENTER,       // AKEYCODE_NUMPAD_ENTER
    RAYLIB_KEY_KP_EQUAL        // AKEYCODE_NUMPAD_EQUALS
};

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
int InitPlatform(void);          // Initialize platform (graphics, inputs and more)
void ClosePlatform(void);        // Close platform

static void AndroidCommandCallback(struct android_app *app, int32_t cmd);           // Process Android activity lifecycle commands
static int32_t AndroidInputCallback(struct android_app *app, AInputEvent *event);   // Process Android inputs
static RaylibGamepadButton AndroidTranslateGamepadButton(int button);                     // Map Android gamepad button to raylib gamepad button

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
// NOTE: Functions declaration is provided by raylib.h

//----------------------------------------------------------------------------------
// Module Functions Definition: Application
//----------------------------------------------------------------------------------

// To allow easier porting to android, we allow the user to define a
// main function which we call from android_main, defined by ourselves
extern int main(int argc, char *argv[]);

// Android main function
void android_main(struct android_app *app)
{
    char arg0[] = "raylib";     // NOTE: argv[] are mutable
    platform.app = app;

    // NOTE: Return from main is ignored
    (void)main(1, (char *[]) { arg0, NULL });

    // Request to end the native activity
    ANativeActivity_finish(app->activity);

    // Android ALooper_pollAll() variables
    int pollResult = 0;
    int pollEvents = 0;

    // Waiting for application events before complete finishing
    while (!app->destroyRequested)
    {
        while ((pollResult = ALooper_pollAll(0, NULL, &pollEvents, (void **)&platform.source)) >= 0)
        {
            if (platform.source != NULL) platform.source->process(app, platform.source);
        }
    }
}

// NOTE: Add this to header (if apps really need it)
struct android_app *GetAndroidApp(void)
{
    return platform.app;
}

//----------------------------------------------------------------------------------
// Module Functions Definition: Window and Graphics Device
//----------------------------------------------------------------------------------

// Check if application should close
bool RaylibWindowShouldClose(void)
{
    if (CORE.Window.ready) return CORE.Window.shouldClose;
    else return true;
}

// Toggle fullscreen mode
void RaylibToggleFullscreen(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibToggleFullscreen() not available on target platform");
}

// Toggle borderless windowed mode
void RaylibToggleBorderlessWindowed(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibToggleBorderlessWindowed() not available on target platform");
}

// Set window state: maximized, if resizable
void RaylibMaximizeWindow(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibMaximizeWindow() not available on target platform");
}

// Set window state: minimized
void RaylibMinimizeWindow(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibMinimizeWindow() not available on target platform");
}

// Set window state: not minimized/maximized
void RaylibRestoreWindow(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibRestoreWindow() not available on target platform");
}

// Set window configuration state using flags
void RaylibSetWindowState(unsigned int flags)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibSetWindowState() not available on target platform");
}

// Clear window configuration state flags
void RaylibClearWindowState(unsigned int flags)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibClearWindowState() not available on target platform");
}

// Set icon for window
void RaylibSetWindowIcon(RaylibImage image)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibSetWindowIcon() not available on target platform");
}

// Set icon for window
void RaylibSetWindowIcons(RaylibImage *images, int count)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibSetWindowIcons() not available on target platform");
}

// Set title for window
void RaylibSetWindowTitle(const char *title)
{
    CORE.Window.title = title;
}

// Set window position on screen (windowed mode)
void RaylibSetWindowPosition(int x, int y)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibSetWindowPosition() not available on target platform");
}

// Set monitor for the current window
void RaylibSetWindowMonitor(int monitor)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibSetWindowMonitor() not available on target platform");
}

// Set window minimum dimensions (RAYLIB_FLAG_WINDOW_RESIZABLE)
void RaylibSetWindowMinSize(int width, int height)
{
    CORE.Window.screenMin.width = width;
    CORE.Window.screenMin.height = height;
}

// Set window maximum dimensions (RAYLIB_FLAG_WINDOW_RESIZABLE)
void RaylibSetWindowMaxSize(int width, int height)
{
    CORE.Window.screenMax.width = width;
    CORE.Window.screenMax.height = height;
}

// Set window dimensions
void RaylibSetWindowSize(int width, int height)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibSetWindowSize() not available on target platform");
}

// Set window opacity, value opacity is between 0.0 and 1.0
void RaylibSetWindowOpacity(float opacity)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibSetWindowOpacity() not available on target platform");
}

// Set window focused
void RaylibSetWindowFocused(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibSetWindowFocused() not available on target platform");
}

// Get native window handle
void *RaylibGetWindowHandle(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetWindowHandle() not implemented on target platform");
    return NULL;
}

// Get number of monitors
int RaylibGetMonitorCount(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetMonitorCount() not implemented on target platform");
    return 1;
}

// Get number of monitors
int RaylibGetCurrentMonitor(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetCurrentMonitor() not implemented on target platform");
    return 0;
}

// Get selected monitor position
RaylibVector2 RaylibGetMonitorPosition(int monitor)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetMonitorPosition() not implemented on target platform");
    return (RaylibVector2){ 0, 0 };
}

// Get selected monitor width (currently used by monitor)
int RaylibGetMonitorWidth(int monitor)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetMonitorWidth() not implemented on target platform");
    return 0;
}

// Get selected monitor height (currently used by monitor)
int RaylibGetMonitorHeight(int monitor)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetMonitorHeight() not implemented on target platform");
    return 0;
}

// Get selected monitor physical width in millimetres
int RaylibGetMonitorPhysicalWidth(int monitor)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetMonitorPhysicalWidth() not implemented on target platform");
    return 0;
}

// Get selected monitor physical height in millimetres
int RaylibGetMonitorPhysicalHeight(int monitor)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetMonitorPhysicalHeight() not implemented on target platform");
    return 0;
}

// Get selected monitor refresh rate
int RaylibGetMonitorRefreshRate(int monitor)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetMonitorRefreshRate() not implemented on target platform");
    return 0;
}

// Get the human-readable, UTF-8 encoded name of the selected monitor
const char *RaylibGetMonitorName(int monitor)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetMonitorName() not implemented on target platform");
    return "";
}

// Get window position XY on monitor
RaylibVector2 RaylibGetWindowPosition(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetWindowPosition() not implemented on target platform");
    return (RaylibVector2){ 0, 0 };
}

// Get window scale DPI factor for current monitor
RaylibVector2 RaylibGetWindowScaleDPI(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetWindowScaleDPI() not implemented on target platform");
    return (RaylibVector2){ 1.0f, 1.0f };
}

// Set clipboard text content
void RaylibSetClipboardText(const char *text)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibSetClipboardText() not implemented on target platform");
}

// Get clipboard text content
// NOTE: returned string is allocated and freed by GLFW
const char *RaylibGetClipboardText(void)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibGetClipboardText() not implemented on target platform");
    return NULL;
}

// Show mouse cursor
void RaylibShowCursor(void)
{
    CORE.Input.Mouse.cursorHidden = false;
}

// Hides mouse cursor
void RaylibHideCursor(void)
{
    CORE.Input.Mouse.cursorHidden = true;
}

// Enables cursor (unlock cursor)
void RaylibEnableCursor(void)
{
    // Set cursor position in the middle
    RaylibSetMousePosition(CORE.Window.screen.width/2, CORE.Window.screen.height/2);

    CORE.Input.Mouse.cursorHidden = false;
}

// Disables cursor (lock cursor)
void RaylibDisableCursor(void)
{
    // Set cursor position in the middle
    RaylibSetMousePosition(CORE.Window.screen.width/2, CORE.Window.screen.height/2);

    CORE.Input.Mouse.cursorHidden = true;
}

// Swap back buffer with front buffer (screen drawing)
void RaylibSwapScreenBuffer(void)
{
    eglSwapBuffers(platform.device, platform.surface);
}

//----------------------------------------------------------------------------------
// Module Functions Definition: Misc
//----------------------------------------------------------------------------------

// Get elapsed time measure in seconds since InitTimer()
double RaylibGetTime(void)
{
    double time = 0.0;
    struct timespec ts = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned long long int nanoSeconds = (unsigned long long int)ts.tv_sec*1000000000LLU + (unsigned long long int)ts.tv_nsec;

    time = (double)(nanoSeconds - CORE.Time.base)*1e-9;  // Elapsed time since InitTimer()

    return time;
}

// Open URL with default system browser (if available)
// NOTE: This function is only safe to use if you control the URL given.
// A user could craft a malicious string performing another action.
// Only call this function yourself not with user input or make sure to check the string yourself.
// Ref: https://github.com/raysan5/raylib/issues/686
void RaylibOpenURL(const char *url)
{
    // Security check to (partially) avoid malicious code
    if (strchr(url, '\'') != NULL) RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "SYSTEM: Provided URL could be potentially malicious, avoid [\'] character");
    else
    {
        JNIEnv *env = NULL;
        JavaVM *vm = platform.app->activity->vm;
        (*vm)->AttachCurrentThread(vm, &env, NULL);

        jstring urlString = (*env)->NewStringUTF(env, url);
        jclass uriClass = (*env)->FindClass(env, "android/net/Uri");
        jmethodID uriParse = (*env)->GetStaticMethodID(env, uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
        jobject uri = (*env)->CallStaticObjectMethod(env, uriClass, uriParse, urlString);

        jclass intentClass = (*env)->FindClass(env, "android/content/Intent");
        jfieldID actionViewId = (*env)->GetStaticFieldID(env, intentClass, "ACTION_VIEW", "Ljava/lang/String;");
        jobject actionView = (*env)->GetStaticObjectField(env, intentClass, actionViewId);
        jmethodID newIntent = (*env)->GetMethodID(env, intentClass, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
        jobject intent = (*env)->AllocObject(env, intentClass);

        (*env)->CallVoidMethod(env, intent, newIntent, actionView, uri);
        jclass activityClass = (*env)->FindClass(env, "android/app/Activity");
        jmethodID startActivity = (*env)->GetMethodID(env, activityClass, "startActivity", "(Landroid/content/Intent;)V");
        (*env)->CallVoidMethod(env, platform.app->activity->clazz, startActivity, intent);

        (*vm)->DetachCurrentThread(vm);
    }
}

//----------------------------------------------------------------------------------
// Module Functions Definition: Inputs
//----------------------------------------------------------------------------------

// Set internal gamepad mappings
int RaylibSetGamepadMappings(const char *mappings)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibSetGamepadMappings() not implemented on target platform");
    return 0;
}

// Set gamepad vibration
void RaylibSetGamepadVibration(int gamepad, float leftMotor, float rightMotor)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "GamepadSetVibration() not implemented on target platform");
}

// Set mouse position XY
void RaylibSetMousePosition(int x, int y)
{
    CORE.Input.Mouse.currentPosition = (RaylibVector2){ (float)x, (float)y };
    CORE.Input.Mouse.previousPosition = CORE.Input.Mouse.currentPosition;
}

// Set mouse cursor
void RaylibSetMouseCursor(int cursor)
{
    RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "RaylibSetMouseCursor() not implemented on target platform");
}

// Register all input events
void RaylibPollInputEvents(void)
{
#if defined(RAYLIB_SUPPORT_GESTURES_SYSTEM)
    // NOTE: Gestures update must be called every frame to reset gestures correctly
    // because ProcessGestureEvent() is just called on an event, not every frame
    UpdateGestures();
#endif

    // Reset keys/chars pressed registered
    CORE.Input.Keyboard.keyPressedQueueCount = 0;
    CORE.Input.Keyboard.charPressedQueueCount = 0;
    // Reset key repeats
    for (int i = 0; i < RAYLIB_MAX_KEYBOARD_KEYS; i++) CORE.Input.Keyboard.keyRepeatInFrame[i] = 0;

    // Reset last gamepad button/axis registered state
    CORE.Input.Gamepad.lastButtonPressed = 0;       // RAYLIB_GAMEPAD_BUTTON_UNKNOWN
    //CORE.Input.Gamepad.axisCount = 0;

    for (int i = 0; i < RAYLIB_MAX_GAMEPADS; i++)
    {
        if (CORE.Input.Gamepad.ready[i])     // Check if gamepad is available
        {
            // Register previous gamepad states
            for (int k = 0; k < RAYLIB_MAX_GAMEPAD_BUTTONS; k++)
                CORE.Input.Gamepad.previousButtonState[i][k] = CORE.Input.Gamepad.currentButtonState[i][k];
        }
    }
    
    // Register previous touch states
    for (int i = 0; i < RAYLIB_MAX_TOUCH_POINTS; i++) CORE.Input.Touch.previousTouchState[i] = CORE.Input.Touch.currentTouchState[i];

    // Reset touch positions
    //for (int i = 0; i < RAYLIB_MAX_TOUCH_POINTS; i++) CORE.Input.Touch.position[i] = (RaylibVector2){ 0, 0 };

    // Register previous keys states
    // NOTE: Android supports up to 260 keys
    for (int i = 0; i < 260; i++)
    {
        CORE.Input.Keyboard.previousKeyState[i] = CORE.Input.Keyboard.currentKeyState[i];
        CORE.Input.Keyboard.keyRepeatInFrame[i] = 0;
    }

    // Android ALooper_pollAll() variables
    int pollResult = 0;
    int pollEvents = 0;

    // Poll Events (registered events)
    // NOTE: Activity is paused if not enabled (platform.appEnabled)
    while ((pollResult = ALooper_pollAll(platform.appEnabled? 0 : -1, NULL, &pollEvents, (void**)&platform.source)) >= 0)
    {
        // Process this event
        if (platform.source != NULL) platform.source->process(platform.app, platform.source);

        // NOTE: Never close window, native activity is controlled by the system!
        if (platform.app->destroyRequested != 0)
        {
            //CORE.Window.shouldClose = true;
            //ANativeActivity_finish(platform.app->activity);
        }
    }
}


//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

// Initialize platform: graphics, inputs and more
int InitPlatform(void)
{
    // Initialize display basic configuration
    //----------------------------------------------------------------------------
    CORE.Window.currentFbo.width = CORE.Window.screen.width;
    CORE.Window.currentFbo.height = CORE.Window.screen.height;

    // Set desired windows flags before initializing anything
    ANativeActivity_setWindowFlags(platform.app->activity, AWINDOW_FLAG_FULLSCREEN, 0);  //AWINDOW_FLAG_SCALED, AWINDOW_FLAG_DITHER

    int orientation = AConfiguration_getOrientation(platform.app->config);

    if (orientation == ACONFIGURATION_ORIENTATION_PORT) RAYLIB_TRACELOG(RAYLIB_LOG_INFO, "ANDROID: Window orientation set as portrait");
    else if (orientation == ACONFIGURATION_ORIENTATION_LAND) RAYLIB_TRACELOG(RAYLIB_LOG_INFO, "ANDROID: Window orientation set as landscape");

    // TODO: Automatic orientation doesn't seem to work
    if (CORE.Window.screen.width <= CORE.Window.screen.height)
    {
        AConfiguration_setOrientation(platform.app->config, ACONFIGURATION_ORIENTATION_PORT);
        RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "ANDROID: Window orientation changed to portrait");
    }
    else
    {
        AConfiguration_setOrientation(platform.app->config, ACONFIGURATION_ORIENTATION_LAND);
        RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "ANDROID: Window orientation changed to landscape");
    }

    //AConfiguration_getDensity(platform.app->config);
    //AConfiguration_getKeyboard(platform.app->config);
    //AConfiguration_getScreenSize(platform.app->config);
    //AConfiguration_getScreenLong(platform.app->config);

    // Set some default window flags
    CORE.Window.flags &= ~RAYLIB_FLAG_WINDOW_HIDDEN;       // false
    CORE.Window.flags &= ~RAYLIB_FLAG_WINDOW_MINIMIZED;    // false
    CORE.Window.flags |= RAYLIB_FLAG_WINDOW_MAXIMIZED;     // true
    CORE.Window.flags &= ~RAYLIB_FLAG_WINDOW_UNFOCUSED;    // false
    //----------------------------------------------------------------------------

    // Initialize App command system
    // NOTE: On APP_CMD_INIT_WINDOW -> InitGraphicsDevice(), InitTimer(), LoadFontDefault()...
    //----------------------------------------------------------------------------
    platform.app->onAppCmd = AndroidCommandCallback;
    //----------------------------------------------------------------------------

    // Initialize input events system
    //----------------------------------------------------------------------------
    platform.app->onInputEvent = AndroidInputCallback;
    //----------------------------------------------------------------------------

    // Initialize storage system
    //----------------------------------------------------------------------------
    InitAssetManager(platform.app->activity->assetManager, platform.app->activity->internalDataPath);   // Initialize assets manager

    CORE.Storage.basePath = platform.app->activity->internalDataPath;   // Define base path for storage
    //----------------------------------------------------------------------------

    RAYLIB_TRACELOG(RAYLIB_LOG_INFO, "PLATFORM: ANDROID: Initialized successfully");

    // Android ALooper_pollAll() variables
    int pollResult = 0;
    int pollEvents = 0;

    // Wait for window to be initialized (display and context)
    while (!CORE.Window.ready)
    {
        // Process events loop
        while ((pollResult = ALooper_pollAll(0, NULL, &pollEvents, (void**)&platform.source)) >= 0)
        {
            // Process this event
            if (platform.source != NULL) platform.source->process(platform.app, platform.source);

            // NOTE: Never close window, native activity is controlled by the system!
            //if (platform.app->destroyRequested != 0) CORE.Window.shouldClose = true;
        }
    }

    return 0;
}

// Close platform
void ClosePlatform(void)
{
    // Close surface, context and display
    if (platform.device != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(platform.device, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (platform.surface != EGL_NO_SURFACE)
        {
            eglDestroySurface(platform.device, platform.surface);
            platform.surface = EGL_NO_SURFACE;
        }

        if (platform.context != EGL_NO_CONTEXT)
        {
            eglDestroyContext(platform.device, platform.context);
            platform.context = EGL_NO_CONTEXT;
        }

        eglTerminate(platform.device);
        platform.device = EGL_NO_DISPLAY;
    }
}

// Initialize display device and framebuffer
// NOTE: width and height represent the screen (framebuffer) desired size, not actual display size
// If width or height are 0, default display size will be used for framebuffer size
// NOTE: returns false in case graphic device could not be created
static int InitGraphicsDevice(void)
{
    CORE.Window.fullscreen = true;
    CORE.Window.flags |= RAYLIB_FLAG_FULLSCREEN_MODE;

    EGLint samples = 0;
    EGLint sampleBuffer = 0;
    if (CORE.Window.flags & RAYLIB_FLAG_MSAA_4X_HINT)
    {
        samples = 4;
        sampleBuffer = 1;
        RAYLIB_TRACELOG(RAYLIB_LOG_INFO, "DISPLAY: Trying to enable MSAA x4");
    }

    const EGLint framebufferAttribs[] =
    {
        EGL_RENDERABLE_TYPE, (rlGetVersion() == RL_OPENGL_ES_30)? EGL_OPENGL_ES3_BIT : EGL_OPENGL_ES2_BIT,      // Type of context support
        EGL_RED_SIZE, 8,            // RAYLIB_RED color bit depth (alternative: 5)
        EGL_GREEN_SIZE, 8,          // RAYLIB_GREEN color bit depth (alternative: 6)
        EGL_BLUE_SIZE, 8,           // RAYLIB_BLUE color bit depth (alternative: 5)
        //EGL_TRANSPARENT_TYPE, EGL_NONE, // Request transparent framebuffer (EGL_TRANSPARENT_RGB does not work on RPI)
        EGL_DEPTH_SIZE, 16,         // Depth buffer size (Required to use Depth testing!)
        //EGL_STENCIL_SIZE, 8,      // Stencil buffer size
        EGL_SAMPLE_BUFFERS, sampleBuffer,    // Activate MSAA
        EGL_SAMPLES, samples,       // 4x Antialiasing if activated (Free on MALI GPUs)
        EGL_NONE
    };

    const EGLint contextAttribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLint numConfigs = 0;

    // Get an EGL device connection
    platform.device = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (platform.device == EGL_NO_DISPLAY)
    {
        RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "DISPLAY: Failed to initialize EGL device");
        return -1;
    }

    // Initialize the EGL device connection
    if (eglInitialize(platform.device, NULL, NULL) == EGL_FALSE)
    {
        // If all of the calls to eglInitialize returned EGL_FALSE then an error has occurred.
        RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "DISPLAY: Failed to initialize EGL device");
        return -1;
    }

    // Get an appropriate EGL framebuffer configuration
    eglChooseConfig(platform.device, framebufferAttribs, &platform.config, 1, &numConfigs);

    // Set rendering API
    eglBindAPI(EGL_OPENGL_ES_API);

    // Create an EGL rendering context
    platform.context = eglCreateContext(platform.device, platform.config, EGL_NO_CONTEXT, contextAttribs);
    if (platform.context == EGL_NO_CONTEXT)
    {
        RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "DISPLAY: Failed to create EGL context");
        return -1;
    }

    // Create an EGL window surface
    //---------------------------------------------------------------------------------
    EGLint displayFormat = 0;

    // EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is guaranteed to be accepted by ANativeWindow_setBuffersGeometry()
    // As soon as we picked a EGLConfig, we can safely reconfigure the ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID
    eglGetConfigAttrib(platform.device, platform.config, EGL_NATIVE_VISUAL_ID, &displayFormat);

    // At this point we need to manage render size vs screen size
    // NOTE: This function use and modify global module variables:
    //  -> CORE.Window.screen.width/CORE.Window.screen.height
    //  -> CORE.Window.render.width/CORE.Window.render.height
    //  -> CORE.Window.screenScale
    SetupFramebuffer(CORE.Window.display.width, CORE.Window.display.height);

    ANativeWindow_setBuffersGeometry(platform.app->window, CORE.Window.render.width, CORE.Window.render.height, displayFormat);
    //ANativeWindow_setBuffersGeometry(platform.app->window, 0, 0, displayFormat);       // Force use of native display size

    platform.surface = eglCreateWindowSurface(platform.device, platform.config, platform.app->window, NULL);

    // There must be at least one frame displayed before the buffers are swapped
    //eglSwapInterval(platform.device, 1);

    if (eglMakeCurrent(platform.device, platform.surface, platform.surface, platform.context) == EGL_FALSE)
    {
        RAYLIB_TRACELOG(RAYLIB_LOG_WARNING, "DISPLAY: Failed to attach EGL rendering context to EGL surface");
        return -1;
    }
    else
    {
        CORE.Window.render.width = CORE.Window.screen.width;
        CORE.Window.render.height = CORE.Window.screen.height;
        CORE.Window.currentFbo.width = CORE.Window.render.width;
        CORE.Window.currentFbo.height = CORE.Window.render.height;

        RAYLIB_TRACELOG(RAYLIB_LOG_INFO, "DISPLAY: Device initialized successfully");
        RAYLIB_TRACELOG(RAYLIB_LOG_INFO, "    > Display size: %i x %i", CORE.Window.display.width, CORE.Window.display.height);
        RAYLIB_TRACELOG(RAYLIB_LOG_INFO, "    > Screen size:  %i x %i", CORE.Window.screen.width, CORE.Window.screen.height);
        RAYLIB_TRACELOG(RAYLIB_LOG_INFO, "    > Render size:  %i x %i", CORE.Window.render.width, CORE.Window.render.height);
        RAYLIB_TRACELOG(RAYLIB_LOG_INFO, "    > Viewport offsets: %i, %i", CORE.Window.renderOffset.x, CORE.Window.renderOffset.y);
    }

    // Load OpenGL extensions
    // NOTE: GL procedures address loader is required to load extensions
    rlLoadExtensions(eglGetProcAddress);

    CORE.Window.ready = true;

    if ((CORE.Window.flags & RAYLIB_FLAG_WINDOW_MINIMIZED) > 0) RaylibMinimizeWindow();

    return 0;
}

// ANDROID: Process activity lifecycle commands
static void AndroidCommandCallback(struct android_app *app, int32_t cmd)
{
    switch (cmd)
    {
        case APP_CMD_START:
        {
            //rendering = true;
        } break;
        case APP_CMD_RESUME: break;
        case APP_CMD_INIT_WINDOW:
        {
            if (app->window != NULL)
            {
                if (platform.contextRebindRequired)
                {
                    // Reset screen scaling to full display size
                    EGLint displayFormat = 0;
                    eglGetConfigAttrib(platform.device, platform.config, EGL_NATIVE_VISUAL_ID, &displayFormat);

                    // Adding renderOffset here feels rather hackish, but the viewport scaling is wrong after the
                    // context rebinding if the screen is scaled unless offsets are added. There's probably a more
                    // appropriate way to fix this
                    ANativeWindow_setBuffersGeometry(app->window,
                        CORE.Window.render.width + CORE.Window.renderOffset.x,
                        CORE.Window.render.height + CORE.Window.renderOffset.y,
                        displayFormat);

                    // Recreate display surface and re-attach OpenGL context
                    platform.surface = eglCreateWindowSurface(platform.device, platform.config, app->window, NULL);
                    eglMakeCurrent(platform.device, platform.surface, platform.surface, platform.context);

                    platform.contextRebindRequired = false;
                }
                else
                {
                    CORE.Window.display.width = ANativeWindow_getWidth(platform.app->window);
                    CORE.Window.display.height = ANativeWindow_getHeight(platform.app->window);

                    // Initialize graphics device (display device and OpenGL context)
                    InitGraphicsDevice();

                    // Initialize OpenGL context (states and resources)
                    // NOTE: CORE.Window.currentFbo.width and CORE.Window.currentFbo.height not used, just stored as globals in rlgl
                    rlglInit(CORE.Window.currentFbo.width, CORE.Window.currentFbo.height);

                    // Setup default viewport
                    // NOTE: It updated CORE.Window.render.width and CORE.Window.render.height
                    SetupViewport(CORE.Window.currentFbo.width, CORE.Window.currentFbo.height);

                    // Initialize hi-res timer
                    InitTimer();

                #if defined(RAYLIB_SUPPORT_MODULE_RTEXT) && defined(RAYLIB_SUPPORT_DEFAULT_FONT)
                    // Load default font
                    // WARNING: External function: Module required: rtext
                    LoadFontDefault();
                    #if defined(RAYLIB_SUPPORT_MODULE_RSHAPES)
                    // Set font white rectangle for shapes drawing, so shapes and text can be batched together
                    // WARNING: rshapes module is required, if not available, default internal white rectangle is used
                    RaylibRectangle rec = RaylibGetFontDefault().recs[95];
                    if (CORE.Window.flags & RAYLIB_FLAG_MSAA_4X_HINT)
                    {
                        // NOTE: We try to maxime rec padding to avoid pixel bleeding on MSAA filtering
                        RaylibSetShapesTexture(RaylibGetFontDefault().texture, (RaylibRectangle){ rec.x + 2, rec.y + 2, 1, 1 });
                    }
                    else
                    {
                        // NOTE: We set up a 1px padding on char rectangle to avoid pixel bleeding
                        RaylibSetShapesTexture(RaylibGetFontDefault().texture, (RaylibRectangle){ rec.x + 1, rec.y + 1, rec.width - 2, rec.height - 2 });
                    }
                    #endif
                #else
                    #if defined(RAYLIB_SUPPORT_MODULE_RSHAPES)
                    // Set default texture and rectangle to be used for shapes drawing
                    // NOTE: rlgl default texture is a 1x1 pixel UNCOMPRESSED_R8G8B8A8
                    Texture2D texture = { rlGetTextureIdDefault(), 1, 1, 1, RAYLIB_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
                    RaylibSetShapesTexture(texture, (RaylibRectangle){ 0.0f, 0.0f, 1.0f, 1.0f });    // WARNING: Module required: rshapes
                    #endif
                #endif

                    // Initialize random seed
                    RaylibSetRandomSeed((unsigned int)time(NULL));

                    // TODO: GPU assets reload in case of lost focus (lost context)
                    // NOTE: This problem has been solved just unbinding and rebinding context from display
                    /*
                    if (assetsReloadRequired)
                    {
                        for (int i = 0; i < assetCount; i++)
                        {
                            // TODO: Unload old asset if required

                            // Load texture again to pointed texture
                            (*textureAsset + i) = RaylibLoadTexture(assetPath[i]);
                        }
                    }
                    */
                }
            }
        } break;
        case APP_CMD_GAINED_FOCUS:
        {
            platform.appEnabled = true;
            CORE.Window.flags &= ~RAYLIB_FLAG_WINDOW_UNFOCUSED;
            //RaylibResumeMusicStream();
        } break;
        case APP_CMD_PAUSE: break;
        case APP_CMD_LOST_FOCUS:
        {
            platform.appEnabled = false;
            CORE.Window.flags |= RAYLIB_FLAG_WINDOW_UNFOCUSED;
            //RaylibPauseMusicStream();
        } break;
        case APP_CMD_TERM_WINDOW:
        {
            // Detach OpenGL context and destroy display surface
            // NOTE 1: This case is used when the user exits the app without closing it. We detach the context to ensure everything is recoverable upon resuming.
            // NOTE 2: Detaching context before destroying display surface avoids losing our resources (textures, shaders, VBOs...)
            // NOTE 3: In some cases (too many context loaded), OS could unload context automatically... :(
            if (platform.device != EGL_NO_DISPLAY)
            {
                eglMakeCurrent(platform.device, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

                if (platform.surface != EGL_NO_SURFACE)
                {
                    eglDestroySurface(platform.device, platform.surface);
                    platform.surface = EGL_NO_SURFACE;
                }

                platform.contextRebindRequired = true;
            }
            // If 'platform.device' is already set to 'EGL_NO_DISPLAY'
            // this means that the user has already called 'RaylibCloseWindow()'

        } break;
        case APP_CMD_SAVE_STATE: break;
        case APP_CMD_STOP: break;
        case APP_CMD_DESTROY: break;
        case APP_CMD_CONFIG_CHANGED:
        {
            //AConfiguration_fromAssetManager(platform.app->config, platform.app->activity->assetManager);
            //print_cur_config(platform.app);

            // Check screen orientation here!
        } break;
        default: break;
    }
}

// ANDROID: Map Android gamepad button to raylib gamepad button
static RaylibGamepadButton AndroidTranslateGamepadButton(int button)
{
    switch (button)
    {
        case AKEYCODE_BUTTON_A: return RAYLIB_GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
        case AKEYCODE_BUTTON_B: return RAYLIB_GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
        case AKEYCODE_BUTTON_X: return RAYLIB_GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
        case AKEYCODE_BUTTON_Y: return RAYLIB_GAMEPAD_BUTTON_RIGHT_FACE_UP;
        case AKEYCODE_BUTTON_L1: return RAYLIB_GAMEPAD_BUTTON_LEFT_TRIGGER_1;
        case AKEYCODE_BUTTON_R1: return RAYLIB_GAMEPAD_BUTTON_RIGHT_TRIGGER_1;
        case AKEYCODE_BUTTON_L2: return RAYLIB_GAMEPAD_BUTTON_LEFT_TRIGGER_2;
        case AKEYCODE_BUTTON_R2: return RAYLIB_GAMEPAD_BUTTON_RIGHT_TRIGGER_2;
        case AKEYCODE_BUTTON_THUMBL: return RAYLIB_GAMEPAD_BUTTON_LEFT_THUMB;
        case AKEYCODE_BUTTON_THUMBR: return RAYLIB_GAMEPAD_BUTTON_RIGHT_THUMB;
        case AKEYCODE_BUTTON_START: return RAYLIB_GAMEPAD_BUTTON_MIDDLE_RIGHT;
        case AKEYCODE_BUTTON_SELECT: return RAYLIB_GAMEPAD_BUTTON_MIDDLE_LEFT;
        case AKEYCODE_BUTTON_MODE: return RAYLIB_GAMEPAD_BUTTON_MIDDLE;
        // On some (most?) gamepads dpad events are reported as axis motion instead
        case AKEYCODE_DPAD_DOWN: return RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_DOWN;
        case AKEYCODE_DPAD_RIGHT: return RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
        case AKEYCODE_DPAD_LEFT: return RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_LEFT;
        case AKEYCODE_DPAD_UP: return RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_UP;
        default: return RAYLIB_GAMEPAD_BUTTON_UNKNOWN;
    }
}

// ANDROID: Get input events
static int32_t AndroidInputCallback(struct android_app *app, AInputEvent *event)
{
    // If additional inputs are required check:
    // https://developer.android.com/ndk/reference/group/input
    // https://developer.android.com/training/game-controllers/controller-input

    int type = AInputEvent_getType(event);
    int source = AInputEvent_getSource(event);

    if (type == AINPUT_EVENT_TYPE_MOTION)
    {
        if (((source & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK) ||
            ((source & AINPUT_SOURCE_GAMEPAD) == AINPUT_SOURCE_GAMEPAD))
        {
            // For now we'll assume a single gamepad which we "detect" on its input event
            CORE.Input.Gamepad.ready[0] = true;

            CORE.Input.Gamepad.axisState[0][RAYLIB_GAMEPAD_AXIS_LEFT_X] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_X, 0);
            CORE.Input.Gamepad.axisState[0][RAYLIB_GAMEPAD_AXIS_LEFT_Y] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_Y, 0);
            CORE.Input.Gamepad.axisState[0][RAYLIB_GAMEPAD_AXIS_RIGHT_X] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_Z, 0);
            CORE.Input.Gamepad.axisState[0][RAYLIB_GAMEPAD_AXIS_RIGHT_Y] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_RZ, 0);
            CORE.Input.Gamepad.axisState[0][RAYLIB_GAMEPAD_AXIS_LEFT_TRIGGER] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_BRAKE, 0) * 2.0f - 1.0f;
            CORE.Input.Gamepad.axisState[0][RAYLIB_GAMEPAD_AXIS_RIGHT_TRIGGER] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_GAS, 0) * 2.0f - 1.0f;

            // dpad is reported as an axis on android
            float dpadX = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_X, 0);
            float dpadY = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_Y, 0);

            if (dpadX == 1.0f)
            {
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_RIGHT] = 1;
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_LEFT] = 0;
            }
            else if (dpadX == -1.0f)
            {
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_RIGHT] = 0;
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_LEFT] = 1;
            }
            else
            {
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_RIGHT] = 0;
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_LEFT] = 0;
            }

            if (dpadY == 1.0f)
            {
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_DOWN] = 1;
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_UP] = 0;
            }
            else if (dpadY == -1.0f)
            {
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_DOWN] = 0;
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_UP] = 1;
            }
            else
            {
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_DOWN] = 0;
                CORE.Input.Gamepad.currentButtonState[0][RAYLIB_GAMEPAD_BUTTON_LEFT_FACE_UP] = 0;
            }

            return 1; // Handled gamepad axis motion
        }
    }
    else if (type == AINPUT_EVENT_TYPE_KEY)
    {
        int32_t keycode = AKeyEvent_getKeyCode(event);
        //int32_t AKeyEvent_getMetaState(event);

        // Handle gamepad button presses and releases
        if (((source & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK) ||
            ((source & AINPUT_SOURCE_GAMEPAD) == AINPUT_SOURCE_GAMEPAD))
        {
            // For now we'll assume a single gamepad which we "detect" on its input event
            CORE.Input.Gamepad.ready[0] = true;

            RaylibGamepadButton button = AndroidTranslateGamepadButton(keycode);

            if (button == RAYLIB_GAMEPAD_BUTTON_UNKNOWN) return 1;

            if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN)
            {
                CORE.Input.Gamepad.currentButtonState[0][button] = 1;
            }
            else CORE.Input.Gamepad.currentButtonState[0][button] = 0;  // Key up

            return 1; // Handled gamepad button
        }

        RaylibKeyboardKey key = (keycode > 0 && keycode < KEYCODE_MAP_SIZE) ? KeycodeMap[keycode] : RAYLIB_KEY_NULL;
        if (key != RAYLIB_KEY_NULL)
        {
            // Save current key and its state
            // NOTE: Android key action is 0 for down and 1 for up
            if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN)
            {
                CORE.Input.Keyboard.currentKeyState[key] = 1;   // Key down

                CORE.Input.Keyboard.keyPressedQueue[CORE.Input.Keyboard.keyPressedQueueCount] = key;
                CORE.Input.Keyboard.keyPressedQueueCount++;
            }
            else if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_MULTIPLE) CORE.Input.Keyboard.keyRepeatInFrame[key] = 1;
            else CORE.Input.Keyboard.currentKeyState[key] = 0;  // Key up
        }

        if (keycode == AKEYCODE_POWER)
        {
            // Let the OS handle input to avoid app stuck. Behaviour: CMD_PAUSE -> CMD_SAVE_STATE -> CMD_STOP -> CMD_CONFIG_CHANGED -> CMD_LOST_FOCUS
            // Resuming Behaviour: CMD_START -> CMD_RESUME -> CMD_CONFIG_CHANGED -> CMD_CONFIG_CHANGED -> CMD_GAINED_FOCUS
            // It seems like locking mobile, screen size (CMD_CONFIG_CHANGED) is affected.
            // NOTE: AndroidManifest.xml must have <activity android:configChanges="orientation|keyboardHidden|screenSize" >
            // Before that change, activity was calling CMD_TERM_WINDOW and CMD_DESTROY when locking mobile, so that was not a normal behaviour
            return 0;
        }
        else if ((keycode == AKEYCODE_BACK) || (keycode == AKEYCODE_MENU))
        {
            // Eat BACK_BUTTON and AKEYCODE_MENU, just do nothing... and don't let to be handled by OS!
            return 1;
        }
        else if ((keycode == AKEYCODE_VOLUME_UP) || (keycode == AKEYCODE_VOLUME_DOWN))
        {
            // Set default OS behaviour
            return 0;
        }

        return 0;
    }

    // Register touch points count
    CORE.Input.Touch.pointCount = AMotionEvent_getPointerCount(event);

    for (int i = 0; (i < CORE.Input.Touch.pointCount) && (i < RAYLIB_MAX_TOUCH_POINTS); i++)
    {
        // Register touch points id
        CORE.Input.Touch.pointId[i] = AMotionEvent_getPointerId(event, i);

        // Register touch points position
        CORE.Input.Touch.position[i] = (RaylibVector2){ AMotionEvent_getX(event, i), AMotionEvent_getY(event, i) };

        // RaylibNormalize CORE.Input.Touch.position[i] for CORE.Window.screen.width and CORE.Window.screen.height
        float widthRatio = (float)(CORE.Window.screen.width + CORE.Window.renderOffset.x) / (float)CORE.Window.display.width;
        float heightRatio = (float)(CORE.Window.screen.height + CORE.Window.renderOffset.y) / (float)CORE.Window.display.height;
        CORE.Input.Touch.position[i].x = CORE.Input.Touch.position[i].x * widthRatio - (float)CORE.Window.renderOffset.x / 2;
        CORE.Input.Touch.position[i].y = CORE.Input.Touch.position[i].y * heightRatio - (float)CORE.Window.renderOffset.y / 2;
    }

    int32_t action = AMotionEvent_getAction(event);
    unsigned int flags = action & AMOTION_EVENT_ACTION_MASK;

#if defined(RAYLIB_SUPPORT_GESTURES_SYSTEM)
    GestureEvent gestureEvent = { 0 };

    gestureEvent.pointCount = CORE.Input.Touch.pointCount;

    // Register touch actions
    if (flags == AMOTION_EVENT_ACTION_DOWN) gestureEvent.touchAction = RAYLIB_TOUCH_ACTION_DOWN;
    else if (flags == AMOTION_EVENT_ACTION_UP) gestureEvent.touchAction = RAYLIB_TOUCH_ACTION_UP;
    else if (flags == AMOTION_EVENT_ACTION_MOVE) gestureEvent.touchAction = RAYLIB_TOUCH_ACTION_MOVE;
    else if (flags == AMOTION_EVENT_ACTION_CANCEL) gestureEvent.touchAction = RAYLIB_TOUCH_ACTION_CANCEL;

    for (int i = 0; (i < gestureEvent.pointCount) && (i < RAYLIB_MAX_TOUCH_POINTS); i++)
    {
        gestureEvent.pointId[i] = CORE.Input.Touch.pointId[i];
        gestureEvent.position[i] = CORE.Input.Touch.position[i];
        gestureEvent.position[i].x /= (float)RaylibGetScreenWidth();
        gestureEvent.position[i].y /= (float)RaylibGetScreenHeight();
    }

    // RaylibGesture data is sent to gestures system for processing
    ProcessGestureEvent(gestureEvent);
#endif

    int32_t pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    if (flags == AMOTION_EVENT_ACTION_POINTER_UP || flags == AMOTION_EVENT_ACTION_UP)
    {
        // One of the touchpoints is released, remove it from touch point arrays
        for (int i = pointerIndex; (i < CORE.Input.Touch.pointCount - 1) && (i < RAYLIB_MAX_TOUCH_POINTS); i++)
        {
            CORE.Input.Touch.pointId[i] = CORE.Input.Touch.pointId[i+1];
            CORE.Input.Touch.position[i] = CORE.Input.Touch.position[i+1];
        }

        CORE.Input.Touch.pointCount--;
    }

    // When all touchpoints are tapped and released really quickly, this event is generated
    if (flags == AMOTION_EVENT_ACTION_CANCEL) CORE.Input.Touch.pointCount = 0;

    if (CORE.Input.Touch.pointCount > 0) CORE.Input.Touch.currentTouchState[RAYLIB_MOUSE_BUTTON_LEFT] = 1;
    else CORE.Input.Touch.currentTouchState[RAYLIB_MOUSE_BUTTON_LEFT] = 0;

    // Stores the previous position of touch[0] only while it's active to calculate the delta.
    if (flags == AMOTION_EVENT_ACTION_MOVE)
    {
        CORE.Input.Mouse.previousPosition = CORE.Input.Mouse.currentPosition;
    }
    else
    {
        CORE.Input.Mouse.previousPosition = CORE.Input.Touch.position[0];
    }

    // Map touch[0] as mouse input for convenience
    CORE.Input.Mouse.currentPosition = CORE.Input.Touch.position[0];
    CORE.Input.Mouse.currentWheelMove = (RaylibVector2){ 0.0f, 0.0f };

    return 0;
}

// EOF
