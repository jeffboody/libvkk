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

A context is required to manage the vector graphics
rendering state. This internal state is intentionally
opaque to the user to provide the simplest API possible.
The user simply needs to reset the context once per frame
prior to rendering any primitives and call the bind
function prior to rendering one or more primitives of the
corresponding bind type.

	vkk_vgContext_t* vkk_vgContext_new(vkk_renderer_t* rend);
	void             vkk_vgContext_delete(vkk_vgContext_t** _self);
	void             vkk_vgContext_reset(vkk_vgContext_t* self,
	                                     cc_mat4f_t* pm);
	void             vkk_vgContext_bindLines(vkk_vgContext_t* self);
	void             vkk_vgContext_bindPolygons(vkk_vgContext_t* self);
	int              vkk_vgContext_pushMatrix(vkk_vgContext_t* self,
	                                          cc_mat4f_t* mvm);
	void             vkk_vgContext_popMatrix(vkk_vgContext_t* self);

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

	void vkk_vgLine_draw(vkk_vgLine_t* self,
	                     vkk_vgContext_t* ctx,
	                     vkk_vgLineStyle_t* style);
	void vkk_vgPolygon_draw(vkk_vgPolygon_t* self,
	                        vkk_vgContext_t* ctx,
	                        vkk_vgPolygonStyle_t* style);

A typical rendering sequence might be as follows.

	cc_mat4f_t pm;
	cc_mat4f_orthoVK(&pm, 1, l, r,
	                 b, t, 0.0f, 2.0f);
	vkk_renderer_beginDefault(default, ...);
	vkk_vgContext_reset(ctx, &pm);
	vkk_vgContext_bindPolygons(ctx);
	vkk_vgPolygon_draw(poly1, ctx, poly_style);
	vkk_vgPolygon_draw(poly2, ctx, poly_style);
	...
	vkk_vgContext_bindLines(ctx);
	vkk_vgLine_draw(line1, ctx, line_style);
	vkk_vgLine_draw(line2, ctx, line_style);
	...
	vkk_renderer_end(default);

Primitives should be drawn in back-to-front order due to
transparency and anti-aliasing.

Setup
=====

VKK VG is an optional library and must be enabled
by exporting the VKK\_USE\_VG variable in the top level
Makefile or CMakeLists.txt.

VKK VG requires that resources are packed into the
app resource file.

VKK VG requires libtess2 support.
