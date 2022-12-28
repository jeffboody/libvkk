VKK VG (Vector Graphics)
========================

VKK VG is a minimal vector graphics library that is built
on top of the VKK Core. The vector graphics library
includes line and polygon primitives. These primitive
objects encapsulate the geometry used for rendering and
is defined using their corresponding builder object. The
builder objects are essential to providing an efficient
process to generate the primitivies geometry. Primitive
geometry is immutable once created however their attributes
are defined at runtime when the primitives are drawn.

The following sections describe the process to build and
render vector graphics primitives.

Line Builder
------------

Line primitives may be created by using a line builder
object. The line builder may be used to build one line at a
time by specifying the center points of the line contour.
Lines may be built in parallel by defining a line builder
per worker thread.

	vkk_vgLineBuilder_t* vkk_vgLineBuilder_new(vkk_engine_t* engine);
	void                 vkk_vgLineBuilder_delete(vkk_vgLineBuilder_t** _self);
	void                 vkk_vgLineBuilder_reset(vkk_vgLineBuilder_t* self);
	vkk_vgLine_t*        vkk_vgLineBuilder_build(vkk_vgLineBuilder_t* self);
	int                  vkk_vgLineBuilder_isDup(vkk_vgLineBuilder_t* self,
	                                             float x,
	                                             float y);
	uint32_t             vkk_vgLineBuilder_count(vkk_vgLineBuilder_t* self);
	int                  vkk_vgLineBuilder_point(vkk_vgLineBuilder_t* self,
	                                             float x,
	                                             float y);

The resulting line geometry has undefined width which is
specified at runtime via the vkk\_line\_draw() function
style parameter. The line width is defined procedurally at
runtime using the normal vectors to the line. This approach
can result in line rendering artifacts for very sharp
angles where the geometry extends much further than
intended. To avoid this problem, the user should split lines
with very sharp corners (e.g. greater than 135 degrees) into
multiple lines at these sharp corners.

Another important feature of the undefined line width is
that lines may be drawn multiple times in the same frame
with different line styles to achieve certain effects. For
example, a hiking trail might be represented by a thin
dashed line on top of a thicker solid line. To achieve this
effect, draw the line twice in a back-to-front order. The
first draw using a thick solid line style and a second draw
using a thin dashed line style.

Lines are anti-aliased.

Polygon Builder
---------------

Polygon primitives may be created by using a polygon builder
object. The polygon builder may be used to build one
polygon at a time by specifying the center points of the
polygon contour. Polygons may be built in parallel by
defining a polygon builder per worker thread.

	vkk_vgPolygonBuilder_t* vkk_vgPolygonBuilder_new(vkk_engine_t* engine);
	void                    vkk_vgPolygonBuilder_delete(vkk_vgPolygonBuilder_t** _self);
	void                    vkk_vgPolygonBuilder_reset(vkk_vgPolygonBuilder_t* self);
	vkk_vgPolygon_t*        vkk_vgPolygonBuilder_build(vkk_vgPolygonBuilder_t* self);
	int                     vkk_vgPolygonBuilder_point(vkk_vgPolygonBuilder_t* self,
	                                                   int first,
	                                                   float x,
	                                                   float y);

Polygon geometry can be very complex and may even include
holes. To enable this complicated geometry the point
function includes a first flag. This flag should be set
for the first point in each contour of which there are two
types. Outer contours define the boundry of a polygon such
as a lake while inner contours define the boundary of holes
such as an island. The outer contour must be fully
specified before specifying any inner contours. The outer
contour may also be specified in multiple parts until a
complete loop is formed while inner contours must each
form a complete loop. The contour type is not explicitly
specified to the point function.

Polygons are not anti-aliased however the user may draw
anti-aliased lines around the contour of the polygons to
achieve polygon anti-aliasing.

Rendering
---------

A VG renderer is required to manage the vector graphics
rendering state. This internal state is intentionally
opaque to the user to provide the simplest API possible.

	vkk_vgRenderer_t* vkk_vgRenderer_new(vkk_renderer_t* rend);
	void              vkk_vgRenderer_delete(vkk_vgRenderer_t** _self);

The VG renderer must be reset once per frame prior to
rendering of any primitives to initialize the rendering
state.

	void vkk_vgRenderer_reset(vkk_vgRenderer_t* self,
	                          cc_mat4f_t* pm);

The primitive type must be bound prior to rendering of one
or more primitives.

	typedef enum
	{
		VKK_SAMPLER_FILTER_NEAREST = 0,
		VKK_SAMPLER_FILTER_LINEAR  = 1,
	} vkk_samplerFilter_e;

	typedef enum
	{
		VKK_SAMPLER_MIPMAP_MODE_NEAREST = 0,
		VKK_SAMPLER_MIPMAP_MODE_LINEAR  = 1,
	} vkk_samplerMipmapMode_e;

	typedef struct
	{
		vkk_samplerFilter_e     min_filter;
		vkk_samplerFilter_e     mag_filter;
		vkk_samplerMipmapMode_e mipmap_mode;
		int                     anisotropy;
		float                   max_anisotropy;
	} vkk_samplerInfo_t;

	void vkk_vgRenderer_bindLines(vkk_vgRenderer_t* self);
	void vkk_vgRenderer_bindPolygons(vkk_vgRenderer_t* self);
	void vkk_vgRenderer_bindImages(vkk_vgRenderer_t* self,
	                               vkk_samplerInfo_t* si);

An optional model view matrix may be applied to the
rendering primitive.

	int  vkk_vgRenderer_pushMatrix(vkk_vgRenderer_t* self,
	                               cc_mat4f_t* mvm);
	void vkk_vgRenderer_popMatrix(vkk_vgRenderer_t* self);

The primitive drawing functions are as follows.

	typedef enum
	{
		VKK_VG_LINECAP_SQUARE  = 0,
		VKK_VG_LINECAP_ROUNDED = 1,
	} vkk_vgLineCap_e;

	typedef struct vkk_vgLineStyle_s
	{
		cc_vec4f_t      color;
		float           brush1;
		float           brush2;
		float           width;
		vkk_vgLineCap_e cap;
	} vkk_vgLineStyle_t;

	typedef struct vkk_vgPolygonStyle_s
	{
		cc_vec4f_t color;
	} vkk_vgPolygonStyle_t;

	void vkk_vgRenderer_drawLine(vkk_vgRenderer_t* self,
	                             vkk_vgLineStyle_t* style,
	                             vkk_vgLine_t* line);
	void vkk_vgRenderer_drawPolygon(vkk_vgRenderer_t* self,
	                                vkk_vgPolygonStyle_t* style,
	                                vkk_vgPolygon_t* polygon);
	void vkk_vgRenderer_drawImage(vkk_vgRenderer_t* self,
	                              vkk_image_t* image);

A typical rendering sequence might be as follows.

	cc_mat4f_t pm;
	cc_mat4f_orthoVK(&pm, 1, l, r,
	                 b, t, 0.0f, 2.0f);
	vkk_renderer_beginDefault(default, ...);
	vkk_vgRenderer_reset(vg_rend, &pm);
	vkk_vgRenderer_bindPolygons(vg_rend);
	vkk_vgRenderer_drawPolygon(vg_rend, poly_style, poly1);
	vkk_vgRenderer_drawPolygon(vg_rend, poly_style, poly2);
	...
	vkk_vgRenderer_bindLines(vg_rend);
	vkk_vgRenderer_drawLine(vg_rend, line_style, line1);
	vkk_vgRenderer_drawLine(vg_rend, line_style, line2);
	...
	vkk_renderer_end(default);

Primitives must be drawn in back-to-front order due to
transparency and anti-aliasing.

Primitive Destructors
---------------------

The primitive destructor functions are as follows.

	void vkk_vgLine_delete(vkk_vgLine_t** _self);
	void vkk_vgPolygon_delete(vkk_vgPolygon_t** _self);

Setup
=====

VKK VG is an optional library and must be enabled
by exporting the VKK\_USE\_VG variable in the top level
Makefile or CMakeLists.txt.

VKK VG requires that resources are packed into the
app resource file.

VKK VG requires libtess2 support.
