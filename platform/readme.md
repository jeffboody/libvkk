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
delivered when the app main thread is paused.

	typedef void* (*vkk_platformOnCreate_fn)(vkk_engine_t* engine);
	typedef void  (*vkk_platformOnDestroy_fn)(void** _priv);
	typedef void  (*vkk_platformOnPause_fn)(void* priv);
	typedef void  (*vkk_platformOnDraw_fn)(void* priv);
	typedef void  (*vkk_platformOnEvent_fn)(void* priv,
	                                        vkk_event_t* event);

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
		VKK_EVENT_TYPE_UNDEFINED         = -1,
		VKK_EVENT_TYPE_ACCELEROMETER     = 0,
		VKK_EVENT_TYPE_ACTION_DOWN       = 1,
		VKK_EVENT_TYPE_ACTION_MOVE       = 2,
		VKK_EVENT_TYPE_ACTION_UP         = 3,
		VKK_EVENT_TYPE_AXIS_MOVE         = 4,
		VKK_EVENT_TYPE_BUTTON_DOWN       = 5,
		VKK_EVENT_TYPE_BUTTON_UP         = 6,
		VKK_EVENT_TYPE_DENSITY           = 7,
		VKK_EVENT_TYPE_DOCUMENT          = 8,
		VKK_EVENT_TYPE_GPS               = 9,
		VKK_EVENT_TYPE_GYROSCOPE         = 10,
		VKK_EVENT_TYPE_KEY_DOWN          = 11,
		VKK_EVENT_TYPE_KEY_UP            = 12,
		VKK_EVENT_TYPE_MAGNETOMETER      = 13,
		VKK_EVENT_TYPE_CONTENT_RECT      = 14,
		VKK_EVENT_TYPE_PERMISSION_STATUS = 15,
		VKK_EVENT_TYPE_LOW_MEMORY        = 16,
		VKK_EVENT_TYPE_MEMORY_INFO       = 17,
	} vkk_eventType_e;

	typedef struct
	{
		float ax;
		float ay;
		float az;
		int   rotation;
	} vkk_eventAccelerometer_t;

	typedef struct
	{
		int count;
		cc_vec2f_t coord[VKK_EVENT_ACTION_COUNT];
	} vkk_eventAction_t;

	typedef struct
	{
		int        id;
		vkk_axis_e axis;
		float      value;
	} vkk_eventAxis_t;

	typedef struct
	{
		int          id;
		vkk_button_e button;
	} vkk_eventButton_t;

	typedef struct
	{
		char uri[256];
		int  fd;
	} vkk_eventDocument_t;

	typedef struct
	{
		double lat;
		double lon;
		float  accuracy;
		float  altitude;
		float  speed;
		float  bearing;
	} vkk_eventGps_t;

	typedef struct
	{
		float ax;
		float ay;
		float az;
	} vkk_eventGyroscope_t;

	typedef struct
	{
		int keycode;
		int meta;
		int repeat;
	} vkk_eventKey_t;

	typedef struct
	{
		float mx;
		float my;
		float mz;
		float gfx;
		float gfy;
		float gfz;
	} vkk_eventMagnetometer_t;

	typedef struct
	{
		int t;
		int l;
		int b;
		int r;
	} vkk_eventContentRect_t;

	typedef enum
	{
		VKK_PERMISSION_FINE_LOCATION       = 1,
		VKK_PERMISSION_READ_STORAGE        = 2,
		VKK_PERMISSION_WRITE_STORAGE       = 3,
	} vkk_permission_e;

	typedef struct
	{
		vkk_permission_e permission;
		int status;
	} vkk_eventPermission_t;

	typedef struct
	{
		size_t available;
		size_t threshold;
		size_t total;
		int    low;
	} vkk_eventMemoryInfo_t;

	typedef struct
	{
		vkk_eventType_e type;
		double ts;
		union
		{
			vkk_eventAccelerometer_t accelerometer;
			vkk_eventAction_t        action;
			vkk_eventAxis_t          axis;
			vkk_eventButton_t        button;
			vkk_eventDocument_t      document;
			float                    density;
			vkk_eventGps_t           gps;
			vkk_eventGyroscope_t     gyroscope;
			vkk_eventKey_t           key;
			vkk_eventMagnetometer_t  magnetometer;
			vkk_eventContentRect_t   content_rect;
			vkk_eventPermission_t    permission;
			vkk_eventMemoryInfo_t    memory_info;
		};
	} vkk_event_t;

The keycodes use the standard ASCII keycode except for the
following special keys.

	#define VKK_KEYCODE_ENTER     0x00D
	#define VKK_KEYCODE_ESCAPE    0x01B
	#define VKK_KEYCODE_BACKSPACE 0x008
	#define VKK_KEYCODE_DELETE    0x07F
	#define VKK_KEYCODE_UP        0x100
	#define VKK_KEYCODE_DOWN      0x101
	#define VKK_KEYCODE_LEFT      0x102
	#define VKK_KEYCODE_RIGHT     0x103
	#define VKK_KEYCODE_HOME      0x104
	#define VKK_KEYCODE_END       0x105
	#define VKK_KEYCODE_PGUP      0x106
	#define VKK_KEYCODE_PGDOWN    0x107
	#define VKK_KEYCODE_INSERT    0x108

The meta field is a mask containing the following values.

	#define VKK_META_ALT     0x00000032
	#define VKK_META_ALT_L   0x00000010
	#define VKK_META_ALT_R   0x00000020
	#define VKK_META_CTRL    0x00007000
	#define VKK_META_CTRL_L  0x00002000
	#define VKK_META_CTRL_R  0x00004000
	#define VKK_META_SHIFT   0x000000C1
	#define VKK_META_SHIFT_L 0x00000040
	#define VKK_META_SHIFT_R 0x00000080
	#define VKK_META_CAPS    0x00100000

See
[vkk-java](https://github.com/jeffboody/vkk-java)
for details on the Android platform interface.

See the
[Engine](../core/readme.md)
section for details on platform commands.
