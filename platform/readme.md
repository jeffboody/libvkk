VKK Platform
============

The platform provides a cross-platform interface which
encapsulates the SDL2 interface on Linux and NativeActivity
interface on Android.

The platform interface requires for the app to declare a
VKK\_PLATFORM\_INFO variable which contains the app
name/version information and create/destroy/pause/draw/event
callback functions. These callback functions are always
called from the main thread except for GPS events which are
called from the UI thread. This is because the app may have
requested GPS recording which can cause events to be
delivered when the app main thread is paused. The onMain
callback may be used on Linux when no display is required
(all other callbacks are unsupported).

	typedef void* (*vkk_platformOnCreate_fn)(vkk_engine_t* engine);
	typedef void  (*vkk_platformOnDestroy_fn)(void** _priv);
	typedef void  (*vkk_platformOnPause_fn)(void* priv);
	typedef void  (*vkk_platformOnDraw_fn)(void* priv);
	typedef void  (*vkk_platformOnEvent_fn)(void* priv,
	                                        vkk_platformEvent_t* event);
	typedef void  (*vkk_platformOnMain_fn)(void* priv, int argc,
	                                       char** argv);

	typedef struct
	{
		const char*              app_name;
		const char*              app_dir;
		vkk_version_t            app_version;
		vkk_platformOnCreate_fn  onCreate;
		vkk_platformOnDestroy_fn onDestroy;
		vkk_platformOnPause_fn   onPause;
		vkk_platformOnDraw_fn    onDraw;
		vkk_platformOnEvent_fn   onEvent;
		vkk_platformOnMain_fn    onMain;
	} vkk_platformInfo_t;

	extern vkk_platformInfo_t VKK_PLATFORM_INFO;

The platform event callbacks provides Android specific
sensor events for accelerometer, magnetometer, gyroscope and
GPS sensors. Additional events are generated for human input
devices such as keyboards, mice, touchscreens and joysticks.
And finally, events are also generated for Android specific
changes to screen density, the content rect and permissions.

	typedef enum
	{
		VKK_PLATFORM_EVENTTYPE_UNDEFINED         = -1,
		VKK_PLATFORM_EVENTTYPE_ACCELEROMETER     = 0,
		VKK_PLATFORM_EVENTTYPE_ACTION_DOWN       = 1,
		VKK_PLATFORM_EVENTTYPE_ACTION_MOVE       = 2,
		VKK_PLATFORM_EVENTTYPE_ACTION_UP         = 3,
		VKK_PLATFORM_EVENTTYPE_AXIS_MOVE         = 4,
		VKK_PLATFORM_EVENTTYPE_BUTTON_DOWN       = 5,
		VKK_PLATFORM_EVENTTYPE_BUTTON_UP         = 6,
		VKK_PLATFORM_EVENTTYPE_DENSITY           = 7,
		VKK_PLATFORM_EVENTTYPE_GPS               = 8,
		VKK_PLATFORM_EVENTTYPE_GYROSCOPE         = 9,
		VKK_PLATFORM_EVENTTYPE_KEY_DOWN          = 10,
		VKK_PLATFORM_EVENTTYPE_KEY_UP            = 11,
		VKK_PLATFORM_EVENTTYPE_MAGNETOMETER      = 12,
		VKK_PLATFORM_EVENTTYPE_CONTENT_RECT      = 13,
		VKK_PLATFORM_EVENTTYPE_PERMISSION_STATUS = 14,
		VKK_PLATFORM_EVENTTYPE_LOW_MEMORY        = 15,
		VKK_PLATFORM_EVENTTYPE_MEMORY_INFO       = 16,
	} vkk_platformEventType_e;

	typedef struct
	{
		float ax;
		float ay;
		float az;
		int   rotation;
	} vkk_platformEventAccelerometer_t;

	typedef struct
	{
		int count;
		cc_vec2f_t coord[VKK_PLATFORM_EVENTACTION_COUNT];
	} vkk_platformEventAction_t;

	typedef struct
	{
		int                id;
		vkk_platformAxis_e axis;
		float              value;
	} vkk_platformEventAxis_t;

	typedef struct
	{
		int                  id;
		vkk_platformButton_e button;
	} vkk_platformEventButton_t;

	typedef struct
	{
		double lat;
		double lon;
		float  accuracy;
		float  altitude;
		float  speed;
		float  bearing;
	} vkk_platformEventGps_t;

	typedef struct
	{
		float ax;
		float ay;
		float az;
	} vkk_platformEventGyroscope_t;

	typedef struct
	{
		int keycode;
		int meta;
		int repeat;
	} vkk_platformEventKey_t;

	typedef struct
	{
		float mx;
		float my;
		float mz;
		float gfx;
		float gfy;
		float gfz;
	} vkk_platformEventMagnetometer_t;

	typedef struct
	{
		int t;
		int l;
		int b;
		int r;
	} vkk_platformEventContentRect_t;

	typedef enum
	{
		VKK_PLATFORM_PERMISSION_FINE_LOCATION       = 1,
		VKK_PLATFORM_PERMISSION_READ_STORAGE        = 2,
		VKK_PLATFORM_PERMISSION_WRITE_STORAGE       = 3,
	} vkk_platformPermission_e;

	typedef struct
	{
		vkk_platformPermission_e permission;
		int status;
	} vkk_platformEventPermission_t;

	typedef struct
	{
		size_t available;
		size_t threshold;
		size_t total;
		int    low;
	} vkk_platformEventMemoryInfo_t;

	typedef struct
	{
		vkk_platformEventType_e type;
		double ts;
		union
		{
			vkk_platformEventAccelerometer_t accelerometer;
			vkk_platformEventAction_t        action;
			vkk_platformEventAxis_t          axis;
			vkk_platformEventButton_t        button;
			float                            density;
			vkk_platformEventGps_t           gps;
			vkk_platformEventGyroscope_t     gyroscope;
			vkk_platformEventKey_t           key;
			vkk_platformEventMagnetometer_t  magnetometer;
			vkk_platformEventContentRect_t   content_rect;
			vkk_platformEventPermission_t    permission;
			vkk_platformEventMemoryInfo_t    memory_info;
		};
	} vkk_platformEvent_t;

The keycodes use the standard ASCII keycode except for the
following special keys.

	#define VKK_PLATFORM_KEYCODE_ENTER     0x00D
	#define VKK_PLATFORM_KEYCODE_ESCAPE    0x01B
	#define VKK_PLATFORM_KEYCODE_BACKSPACE 0x008
	#define VKK_PLATFORM_KEYCODE_DELETE    0x07F
	#define VKK_PLATFORM_KEYCODE_UP        0x100
	#define VKK_PLATFORM_KEYCODE_DOWN      0x101
	#define VKK_PLATFORM_KEYCODE_LEFT      0x102
	#define VKK_PLATFORM_KEYCODE_RIGHT     0x103
	#define VKK_PLATFORM_KEYCODE_HOME      0x104
	#define VKK_PLATFORM_KEYCODE_END       0x105
	#define VKK_PLATFORM_KEYCODE_PGUP      0x106
	#define VKK_PLATFORM_KEYCODE_PGDOWN    0x107
	#define VKK_PLATFORM_KEYCODE_INSERT    0x108

The meta field is a mask containing the following values.

	#define VKK_PLATFORM_META_ALT     0x00000032
	#define VKK_PLATFORM_META_ALT_L   0x00000010
	#define VKK_PLATFORM_META_ALT_R   0x00000020
	#define VKK_PLATFORM_META_CTRL    0x00007000
	#define VKK_PLATFORM_META_CTRL_L  0x00002000
	#define VKK_PLATFORM_META_CTRL_R  0x00004000
	#define VKK_PLATFORM_META_SHIFT   0x000000C1
	#define VKK_PLATFORM_META_SHIFT_L 0x00000040
	#define VKK_PLATFORM_META_SHIFT_R 0x00000080
	#define VKK_PLATFORM_META_CAPS    0x00100000

See
[vkk-java](https://github.com/jeffboody/vkk-java)
for details on the Android platform interface.

See the
[Engine](../core/readme.md)
section for details on platform commands.
