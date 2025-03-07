/**
 * cgltf - a single-file glTF 2.0 parser written in C99.
 *
 * Version: 1.2
 *
 * Website: https://github.com/jkuhlmann/cgltf
 *
 * Distributed under the MIT License, see notice at the end of this file.
 *
 * Building:
 * Include this file where you need the struct and function
 * declarations. Have exactly one source file where you define
 * `CGLTF_IMPLEMENTATION` before including this file to get the
 * function definitions.
 *
 * Reference:
 * `cgltf_result cgltf_parse(const cgltf_options*, const void*,
 * cgltf_size, cgltf_data**)` parses both glTF and GLB data. If
 * this function returns `cgltf_result_success`, you have to call
 * `cgltf_free()` on the created `cgltf_data*` variable.
 * Note that contents of external files for buffers and images are not
 * automatically loaded. You'll need to read these files yourself using
 * URIs in the `cgltf_data` structure.
 *
 * `cgltf_options` is the struct passed to `cgltf_parse()` to control
 * parts of the parsing process. You can use it to force the file type
 * and provide memory allocation callbacks. Should be zero-initialized
 * to trigger default behavior.
 *
 * `cgltf_data` is the struct allocated and filled by `cgltf_parse()`.
 * It generally mirrors the glTF format as described by the spec (see
 * https://github.com/KhronosGroup/glTF/tree/master/specification/2.0).
 *
 * `void cgltf_free(cgltf_data*)` frees the allocated `cgltf_data`
 * variable.
 *
 * `cgltf_result cgltf_load_buffers(const cgltf_options*, cgltf_data*,
 * const char* gltf_path)` can be optionally called to open and read buffer
 * files using the `FILE*` APIs. The `gltf_path` argument is the path to
 * the original glTF file, which allows the parser to resolve the path to
 * buffer files.
 *
 * `cgltf_result cgltf_load_buffer_base64(const cgltf_options* options,
 * cgltf_size size, const char* base64, void** out_data)` decodes
 * base64-encoded data content. Used internally by `cgltf_load_buffers()`
 * and may be useful if you're not dealing with normal files.
 *
 * `cgltf_result cgltf_parse_file(const cgltf_options* options, const
 * char* path, cgltf_data** out_data)` can be used to open the given
 * file using `FILE*` APIs and parse the data using `cgltf_parse()`.
 *
 * `cgltf_result cgltf_validate(cgltf_data*)` can be used to do additional
 * checks to make sure the parsed glTF data is valid.
 *
 * `cgltf_node_transform_local` converts the translation / rotation / scale properties of a node
 * into a mat4.
 *
 * `cgltf_node_transform_world` calls `cgltf_node_transform_local` on every ancestor in order
 * to compute the root-to-node transformation.
 *
 * `cgltf_accessor_read_float` reads a certain element from an accessor and converts it to
 * floating point, assuming that `cgltf_load_buffers` has already been called. The passed-in element
 * size is the number of floats in the output buffer, which should be in the range [1, 16]. Returns
 * false if the passed-in element_size is too small, or if the accessor is sparse.
 *
 * `cgltf_accessor_read_index` is similar to its floating-point counterpart, but it returns size_t
 * and only works with single-component data types.
 *
 * `cgltf_result cgltf_copy_extras_json(const cgltf_data*, const cgltf_extras*,
 * char* dest, cgltf_size* dest_size)` allows to retrieve the "extras" data that
 * can be attached to many glTF objects (which can be arbitrary JSON data). The
 * `cgltf_extras` struct stores the offsets of the start and end of the extras JSON data
 * as it appears in the complete glTF JSON data. This function copies the extras data
 * into the provided buffer. If `dest` is NULL, the length of the data is written into
 * `dest_size`. You can then parse this data using your own JSON parser
 * or, if you've included the cgltf implementation using the integrated JSMN JSON parser.
 */
#ifndef CGLTF_H_INCLUDED__
#define CGLTF_H_INCLUDED__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t cgltf_size;
typedef float cgltf_float;
typedef int cgltf_int;
typedef int cgltf_bool;

typedef enum cgltf_file_type
{
	cgltf_file_type_invalid,
	cgltf_file_type_gltf,
	cgltf_file_type_glb,
} cgltf_file_type;

typedef struct cgltf_options
{
	cgltf_file_type type; /* invalid == auto detect */
	cgltf_size json_token_count; /* 0 == auto */
	void* (*memory_alloc)(void* user, cgltf_size size);
	void (*memory_free) (void* user, void* ptr);
	void* memory_user_data;
} cgltf_options;

typedef enum cgltf_result
{
	cgltf_result_success,
	cgltf_result_data_too_short,
	cgltf_result_unknown_format,
	cgltf_result_invalid_json,
	cgltf_result_invalid_gltf,
	cgltf_result_invalid_options,
	cgltf_result_file_not_found,
	cgltf_result_io_error,
	cgltf_result_out_of_memory,
} cgltf_result;

typedef enum cgltf_buffer_view_type
{
	cgltf_buffer_view_type_invalid,
	cgltf_buffer_view_type_indices,
	cgltf_buffer_view_type_vertices,
} cgltf_buffer_view_type;

typedef enum cgltf_attribute_type
{
	cgltf_attribute_type_invalid,
	cgltf_attribute_type_position,
	cgltf_attribute_type_normal,
	cgltf_attribute_type_tangent,
	cgltf_attribute_type_texcoord,
	cgltf_attribute_type_color,
	cgltf_attribute_type_joints,
	cgltf_attribute_type_weights,
} cgltf_attribute_type;

typedef enum cgltf_component_type
{
	cgltf_component_type_invalid,
	cgltf_component_type_r_8, /* BYTE */
	cgltf_component_type_r_8u, /* UNSIGNED_BYTE */
	cgltf_component_type_r_16, /* SHORT */
	cgltf_component_type_r_16u, /* UNSIGNED_SHORT */
	cgltf_component_type_r_32u, /* UNSIGNED_INT */
	cgltf_component_type_r_32f, /* FLOAT */
} cgltf_component_type;

typedef enum cgltf_type
{
	cgltf_type_invalid,
	cgltf_type_scalar,
	cgltf_type_vec2,
	cgltf_type_vec3,
	cgltf_type_vec4,
	cgltf_type_mat2,
	cgltf_type_mat3,
	cgltf_type_mat4,
} cgltf_type;

typedef enum cgltf_primitive_type
{
	cgltf_primitive_type_points,
	cgltf_primitive_type_lines,
	cgltf_primitive_type_line_loop,
	cgltf_primitive_type_line_strip,
	cgltf_primitive_type_triangles,
	cgltf_primitive_type_triangle_strip,
	cgltf_primitive_type_triangle_fan,
} cgltf_primitive_type;

typedef enum cgltf_alpha_mode
{
	cgltf_alpha_mode_opaque,
	cgltf_alpha_mode_mask,
	cgltf_alpha_mode_blend,
} cgltf_alpha_mode;

typedef enum cgltf_animation_path_type {
	cgltf_animation_path_type_invalid,
	cgltf_animation_path_type_translation,
	cgltf_animation_path_type_rotation,
	cgltf_animation_path_type_scale,
	cgltf_animation_path_type_weights,
} cgltf_animation_path_type;

typedef enum cgltf_interpolation_type {
	cgltf_interpolation_type_linear,
	cgltf_interpolation_type_step,
	cgltf_interpolation_type_cubic_spline,
} cgltf_interpolation_type;

typedef enum cgltf_camera_type {
	cgltf_camera_type_invalid,
	cgltf_camera_type_perspective,
	cgltf_camera_type_orthographic,
} cgltf_camera_type;

typedef enum cgltf_light_type {
	cgltf_light_type_invalid,
	cgltf_light_type_directional,
	cgltf_light_type_point,
	cgltf_light_type_spot,
} cgltf_light_type;

typedef struct cgltf_extras {
	cgltf_size start_offset;
	cgltf_size end_offset;
} cgltf_extras;

typedef struct cgltf_buffer
{
	cgltf_size size;
	char* uri;
	void* data; /* loaded by cgltf_load_buffers */
	cgltf_extras extras;
} cgltf_buffer;

typedef struct cgltf_buffer_view
{
	cgltf_buffer* buffer;
	cgltf_size offset;
	cgltf_size size;
	cgltf_size stride; /* 0 == automatically determined by accessor */
	cgltf_buffer_view_type type;
	cgltf_extras extras;
} cgltf_buffer_view;

typedef struct cgltf_accessor_sparse
{
	cgltf_size count;
	cgltf_buffer_view* indices_buffer_view;
	cgltf_size indices_byte_offset;
	cgltf_component_type indices_component_type;
	cgltf_buffer_view* values_buffer_view;
	cgltf_size values_byte_offset;
	cgltf_extras extras;
	cgltf_extras indices_extras;
	cgltf_extras values_extras;
} cgltf_accessor_sparse;

typedef struct cgltf_accessor
{
	cgltf_component_type component_type;
	cgltf_bool normalized;
	cgltf_type type;
	cgltf_size offset;
	cgltf_size count;
	cgltf_size stride;
	cgltf_buffer_view* buffer_view;
	cgltf_bool has_min;
	cgltf_float min[16];
	cgltf_bool has_max;
	cgltf_float max[16];
	cgltf_bool is_sparse;
	cgltf_accessor_sparse sparse;
	cgltf_extras extras;
} cgltf_accessor;

typedef struct cgltf_attribute
{
	char* name;
	cgltf_attribute_type type;
	cgltf_int index;
	cgltf_accessor* data;
} cgltf_attribute;

typedef struct cgltf_image 
{
	char* name;
	char* uri;
	cgltf_buffer_view* buffer_view;
	char* mime_type;
	cgltf_extras extras;
} cgltf_image;

typedef struct cgltf_sampler
{
	cgltf_int mag_filter;
	cgltf_int min_filter;
	cgltf_int wrap_s;
	cgltf_int wrap_t;
	cgltf_extras extras;
} cgltf_sampler;

typedef struct cgltf_texture
{
	char* name;
	cgltf_image* image;
	cgltf_sampler* sampler;
	cgltf_extras extras;
} cgltf_texture;

typedef struct cgltf_texture_transform
{
	cgltf_float offset[2];
	cgltf_float rotation;
	cgltf_float scale[2];
	cgltf_int texcoord;
} cgltf_texture_transform;

typedef struct cgltf_texture_view
{	
	cgltf_texture* texture;
	cgltf_int texcoord;
	cgltf_float scale; /* equivalent to strength for occlusion_texture */
	cgltf_bool has_transform;
	cgltf_texture_transform transform;
	cgltf_extras extras;
} cgltf_texture_view;

typedef struct cgltf_pbr_metallic_roughness
{
	cgltf_texture_view base_color_texture;
	cgltf_texture_view metallic_roughness_texture;

	cgltf_float base_color_factor[4];
	cgltf_float metallic_factor;
	cgltf_float roughness_factor;

	cgltf_extras extras;
} cgltf_pbr_metallic_roughness;

typedef struct cgltf_pbr_specular_glossiness
{
	cgltf_texture_view diffuse_texture;
	cgltf_texture_view specular_glossiness_texture;

	cgltf_float diffuse_factor[4];
	cgltf_float specular_factor[3];
	cgltf_float glossiness_factor;
} cgltf_pbr_specular_glossiness;

typedef struct cgltf_material
{
	char* name;
	cgltf_bool has_pbr_metallic_roughness;
	cgltf_bool has_pbr_specular_glossiness;
	cgltf_pbr_metallic_roughness pbr_metallic_roughness;
	cgltf_pbr_specular_glossiness pbr_specular_glossiness;
	cgltf_texture_view normal_texture;
	cgltf_texture_view occlusion_texture;
	cgltf_texture_view emissive_texture;
	cgltf_float emissive_factor[3];
	cgltf_alpha_mode alpha_mode;
	cgltf_float alpha_cutoff;
	cgltf_bool double_sided;
	cgltf_bool unlit;
	cgltf_extras extras;
} cgltf_material;

typedef struct cgltf_morph_target {
	cgltf_attribute* attributes;
	cgltf_size attributes_count;
} cgltf_morph_target;

typedef struct cgltf_primitive {
	cgltf_primitive_type type;
	cgltf_accessor* indices;
	cgltf_material* material;
	cgltf_attribute* attributes;
	cgltf_size attributes_count;
	cgltf_morph_target* targets;
	cgltf_size targets_count;
	cgltf_extras extras;
} cgltf_primitive;

typedef struct cgltf_mesh {
	char* name;
	cgltf_primitive* primitives;
	cgltf_size primitives_count;
	cgltf_float* weights;
	cgltf_size weights_count;
	cgltf_extras extras;
} cgltf_mesh;

typedef struct cgltf_node cgltf_node;

typedef struct cgltf_skin {
	char* name;
	cgltf_node** joints;
	cgltf_size joints_count;
	cgltf_node* skeleton;
	cgltf_accessor* inverse_bind_matrices;
	cgltf_extras extras;
} cgltf_skin;

typedef struct cgltf_camera_perspective {
	cgltf_float aspect_ratio;
	cgltf_float yfov;
	cgltf_float zfar;
	cgltf_float znear;
	cgltf_extras extras;
} cgltf_camera_perspective;

typedef struct cgltf_camera_orthographic {
	cgltf_float xmag;
	cgltf_float ymag;
	cgltf_float zfar;
	cgltf_float znear;
	cgltf_extras extras;
} cgltf_camera_orthographic;

typedef struct cgltf_camera {
	char* name;
	cgltf_camera_type type;
	union {
		cgltf_camera_perspective perspective;
		cgltf_camera_orthographic orthographic;
	};
	cgltf_extras extras;
} cgltf_camera;

typedef struct cgltf_light {
	char* name;
	cgltf_float color[3];
	cgltf_float intensity;
	cgltf_light_type type;
	cgltf_float range;
	cgltf_float spot_inner_cone_angle;
	cgltf_float spot_outer_cone_angle;
} cgltf_light;

typedef struct cgltf_node {
	char* name;
	cgltf_node* parent;
	cgltf_node** children;
	cgltf_size children_count;
	cgltf_skin* skin;
	cgltf_mesh* mesh;
	cgltf_camera* camera;
	cgltf_light* light;
	cgltf_float* weights;
	cgltf_size weights_count;
	cgltf_bool has_translation;
	cgltf_bool has_rotation;
	cgltf_bool has_scale;
	cgltf_bool has_matrix;
	cgltf_float translation[3];
	cgltf_float rotation[4];
	cgltf_float scale[3];
	cgltf_float matrix[16];
	cgltf_extras extras;
} cgltf_node;

typedef struct cgltf_scene {
	char* name;
	cgltf_node** nodes;
	cgltf_size nodes_count;
	cgltf_extras extras;
} cgltf_scene;

typedef struct cgltf_animation_sampler {
	cgltf_accessor* input;
	cgltf_accessor* output;
	cgltf_interpolation_type interpolation;
	cgltf_extras extras;
} cgltf_animation_sampler;

typedef struct cgltf_animation_channel {
	cgltf_animation_sampler* sampler;
	cgltf_node* target_node;
	cgltf_animation_path_type target_path;
	cgltf_extras extras;
} cgltf_animation_channel;

typedef struct cgltf_animation {
	char* name;
	cgltf_animation_sampler* samplers;
	cgltf_size samplers_count;
	cgltf_animation_channel* channels;
	cgltf_size channels_count;
	cgltf_extras extras;
} cgltf_animation;

typedef struct cgltf_asset {
	char* copyright;
	char* generator;
	char* version;
	char* min_version;
	cgltf_extras extras;
} cgltf_asset;

typedef struct cgltf_data
{
	cgltf_file_type file_type;
	void* file_data;

	cgltf_asset asset;

	cgltf_mesh* meshes;
	cgltf_size meshes_count;

	cgltf_material* materials;
	cgltf_size materials_count;

	cgltf_accessor* accessors;
	cgltf_size accessors_count;

	cgltf_buffer_view* buffer_views;
	cgltf_size buffer_views_count;

	cgltf_buffer* buffers;
	cgltf_size buffers_count;

	cgltf_image* images;
	cgltf_size images_count;

	cgltf_texture* textures;
	cgltf_size textures_count;

	cgltf_sampler* samplers;
	cgltf_size samplers_count;

	cgltf_skin* skins;
	cgltf_size skins_count;

	cgltf_camera* cameras;
	cgltf_size cameras_count;

	cgltf_light* lights;
	cgltf_size lights_count;

	cgltf_node* nodes;
	cgltf_size nodes_count;

	cgltf_scene* scenes;
	cgltf_size scenes_count;

	cgltf_scene* scene;

	cgltf_animation* animations;
	cgltf_size animations_count;

	cgltf_extras extras;

	const char* json;
	cgltf_size json_size;

	char** extensions_used;
	cgltf_size extensions_used_count;

	char** extensions_required;
	cgltf_size extensions_required_count;

	const void* bin;
	cgltf_size bin_size;

	void (*memory_free) (void* user, void* ptr);
	void* memory_user_data;
} cgltf_data;

cgltf_result cgltf_parse(
		const cgltf_options* options,
		const void* data,
		cgltf_size size,
		cgltf_data** out_data);

cgltf_result cgltf_parse_file(
		const cgltf_options* options,
		const char* path,
		cgltf_data** out_data);

cgltf_result cgltf_load_buffers(
		const cgltf_options* options,
		cgltf_data* data,
		const char* gltf_path);


cgltf_result cgltf_load_buffer_base64(const cgltf_options* options, cgltf_size size, const char* base64, void** out_data);

cgltf_result cgltf_validate(
		cgltf_data* data);

void cgltf_free(cgltf_data* data);

void cgltf_node_transform_local(const cgltf_node* node, cgltf_float* out_matrix);
void cgltf_node_transform_world(const cgltf_node* node, cgltf_float* out_matrix);

cgltf_bool cgltf_accessor_read_float(const cgltf_accessor* accessor, cgltf_size index, cgltf_float* out, cgltf_size element_size);
cgltf_size cgltf_accessor_read_index(const cgltf_accessor* accessor, cgltf_size index);

cgltf_result cgltf_copy_extras_json(const cgltf_data* data, const cgltf_extras* extras, char* dest, cgltf_size* dest_size);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef CGLTF_H_INCLUDED__ */

/*
 *
 * Stop now, if you are only interested in the API.
 * Below, you find the implementation.
 *
 */

#ifdef __INTELLISENSE__
/* This makes MSVC intellisense work. */
#define CGLTF_IMPLEMENTATION
#endif

#ifdef CGLTF_IMPLEMENTATION

#include <stdint.h> /* For uint8_t, uint32_t */
#include <string.h> /* For strncpy */
#include <stdlib.h> /* For malloc, free */
#include <stdio.h>  /* For fopen */
#include <limits.h> /* For UINT_MAX etc */

/* JSMN_PARENT_LINKS is necessary to make parsing large structures linear in input size */
#define JSMN_PARENT_LINKS

/*
 * -- jsmn.h start --
 * Source: https://github.com/zserge/jsmn
 * License: MIT
 */
typedef enum {
	JSMN_UNDEFINED = 0,
	JSMN_OBJECT = 1,
	JSMN_ARRAY = 2,
	JSMN_STRING = 3,
	JSMN_PRIMITIVE = 4
} jsmntype_t;
enum jsmnerr {
	/* Not enough tokens were provided */
	JSMN_ERROR_NOMEM = -1,
	/* Invalid character inside JSON string */
	JSMN_ERROR_INVAL = -2,
	/* The string is not a full JSON packet, more bytes expected */
	JSMN_ERROR_PART = -3
};
typedef struct {
	jsmntype_t type;
	int start;
	int end;
	int size;
#ifdef JSMN_PARENT_LINKS
	int parent;
#endif
} jsmntok_t;
typedef struct {
	unsigned int pos; /* offset in the JSON string */
	unsigned int toknext; /* next token to allocate */
	int toksuper; /* superior token node, e.g parent object or array */
} jsmn_parser;
static void jsmn_init(jsmn_parser *parser);
static int jsmn_parse(jsmn_parser *parser, const char *js, size_t len, jsmntok_t *tokens, size_t num_tokens);
/*
 * -- jsmn.h end --
 */


static const cgltf_size GlbHeaderSize = 12;
static const cgltf_size GlbChunkHeaderSize = 8;
static const uint32_t GlbVersion = 2;
static const uint32_t GlbMagic = 0x46546C67;
static const uint32_t GlbMagicJsonChunk = 0x4E4F534A;
static const uint32_t GlbMagicBinChunk = 0x004E4942;

static void* cgltf_default_alloc(void* user, cgltf_size size)
{
	(void)user;
	return malloc(size);
}

static void cgltf_default_free(void* user, void* ptr)
{
	(void)user;
	free(ptr);
}

static void* cgltf_calloc(cgltf_options* options, size_t element_size, cgltf_size count)
{
	if (SIZE_MAX / element_size < count)
	{
		return NULL;
	}
	void* result = options->memory_alloc(options->memory_user_data, element_size * count);
	if (!result)
	{
		return NULL;
	}
	memset(result, 0, element_size * count);
	return result;
}

static cgltf_result cgltf_parse_json(cgltf_options* options, const uint8_t* json_chunk, cgltf_size size, cgltf_data** out_data);

cgltf_result cgltf_parse(const cgltf_options* options, const void* data, cgltf_size size, cgltf_data** out_data)
{
	if (size < GlbHeaderSize)
	{
		return cgltf_result_data_too_short;
	}

	if (options == NULL)
	{
		return cgltf_result_invalid_options;
	}

	cgltf_options fixed_options = *options;
	if (fixed_options.memory_alloc == NULL)
	{
		fixed_options.memory_alloc = &cgltf_default_alloc;
	}
	if (fixed_options.memory_free == NULL)
	{
		fixed_options.memory_free = &cgltf_default_free;
	}

	uint32_t tmp;
	// Magic
	memcpy(&tmp, data, 4);
	if (tmp != GlbMagic)
	{
		if (fixed_options.type == cgltf_file_type_invalid)
		{
			fixed_options.type = cgltf_file_type_gltf;
		}
		else if (fixed_options.type == cgltf_file_type_glb)
		{
			return cgltf_result_unknown_format;
		}
	}

	if (fixed_options.type == cgltf_file_type_gltf)
	{
		cgltf_result json_result = cgltf_parse_json(&fixed_options, (const uint8_t*)data, size, out_data);
		if (json_result != cgltf_result_success)
		{
			return json_result;
		}

		(*out_data)->file_type = cgltf_file_type_gltf;

		return cgltf_result_success;
	}

	const uint8_t* ptr = (const uint8_t*)data;
	// Version
	memcpy(&tmp, ptr + 4, 4);
	uint32_t version = tmp;
	if (version != GlbVersion)
	{
		return cgltf_result_unknown_format;
	}

	// Total length
	memcpy(&tmp, ptr + 8, 4);
	if (tmp > size)
	{
		return cgltf_result_data_too_short;
	}

	const uint8_t* json_chunk = ptr + GlbHeaderSize;

	if (GlbHeaderSize + GlbChunkHeaderSize > size)
	{
		return cgltf_result_data_too_short;
	}

	// JSON chunk: length
	uint32_t json_length;
	memcpy(&json_length, json_chunk, 4);
	if (GlbHeaderSize + GlbChunkHeaderSize + json_length > size)
	{
		return cgltf_result_data_too_short;
	}

	// JSON chunk: magic
	memcpy(&tmp, json_chunk + 4, 4);
	if (tmp != GlbMagicJsonChunk)
	{
		return cgltf_result_unknown_format;
	}

	json_chunk += GlbChunkHeaderSize;

	const void* bin = 0;
	cgltf_size bin_size = 0;

	if (GlbHeaderSize + GlbChunkHeaderSize + json_length + GlbChunkHeaderSize <= size)
	{
		// We can read another chunk
		const uint8_t* bin_chunk = json_chunk + json_length;

		// Bin chunk: length
		uint32_t bin_length;
		memcpy(&bin_length, bin_chunk, 4);
		if (GlbHeaderSize + GlbChunkHeaderSize + json_length + GlbChunkHeaderSize + bin_length > size)
		{
			return cgltf_result_data_too_short;
		}

		// Bin chunk: magic
		memcpy(&tmp, bin_chunk + 4, 4);
		if (tmp != GlbMagicBinChunk)
		{
			return cgltf_result_unknown_format;
		}

		bin_chunk += GlbChunkHeaderSize;

		bin = bin_chunk;
		bin_size = bin_length;
	}

	cgltf_result json_result = cgltf_parse_json(&fixed_options, json_chunk, json_length, out_data);
	if (json_result != cgltf_result_success)
	{
		return json_result;
	}

	(*out_data)->file_type = cgltf_file_type_glb;
	(*out_data)->bin = bin;
	(*out_data)->bin_size = bin_size;

	return cgltf_result_success;
}

cgltf_result cgltf_parse_file(const cgltf_options* options, const char* path, cgltf_data** out_data)
{
	if (options == NULL)
	{
		return cgltf_result_invalid_options;
	}

	void* (*memory_alloc)(void*, cgltf_size) = options->memory_alloc ? options->memory_alloc : &cgltf_default_alloc;
	void (*memory_free)(void*, void*) = options->memory_free ? options->memory_free : &cgltf_default_free;

	FILE* file = fopen(path, "rb");
	if (!file)
	{
		return cgltf_result_file_not_found;
	}

	fseek(file, 0, SEEK_END);

	long length = ftell(file);
	if (length < 0)
	{
		fclose(file);
		return cgltf_result_io_error;
	}

	fseek(file, 0, SEEK_SET);

	char* file_data = (char*)memory_alloc(options->memory_user_data, length);
	if (!file_data)
	{
		fclose(file);
		return cgltf_result_out_of_memory;
	}

	cgltf_size file_size = (cgltf_size)length;
	cgltf_size read_size = fread(file_data, 1, file_size, file);

	fclose(file);

	if (read_size != file_size)
	{
		memory_free(options->memory_user_data, file_data);
		return cgltf_result_io_error;
	}

	cgltf_result result = cgltf_parse(options, file_data, file_size, out_data);

	if (result != cgltf_result_success)
	{
		memory_free(options->memory_user_data, file_data);
		return result;
	}

	(*out_data)->file_data = file_data;

	return cgltf_result_success;
}

static void cgltf_combine_paths(char* path, const char* base, const char* uri)
{
	const char* s0 = strrchr(base, '/');
	const char* s1 = strrchr(base, '\\');
	const char* slash = s0 ? (s1 && s1 > s0 ? s1 : s0) : s1;

	if (slash)
	{
		size_t prefix = slash - base + 1;

		strncpy(path, base, prefix);
		strcpy(path + prefix, uri);
	}
	else
	{
		strcpy(path, uri);
	}
}

static cgltf_result cgltf_load_buffer_file(const cgltf_options* options, cgltf_size size, const char* uri, const char* gltf_path, void** out_data)
{
	void* (*memory_alloc)(void*, cgltf_size) = options->memory_alloc ? options->memory_alloc : &cgltf_default_alloc;
	void (*memory_free)(void*, void*) = options->memory_free ? options->memory_free : &cgltf_default_free;

	char* path = (char*)memory_alloc(options->memory_user_data, strlen(uri) + strlen(gltf_path) + 1);
	if (!path)
	{
		return cgltf_result_out_of_memory;
	}

	cgltf_combine_paths(path, gltf_path, uri);

	FILE* file = fopen(path, "rb");

	memory_free(options->memory_user_data, path);

	if (!file)
	{
		return cgltf_result_file_not_found;
	}

	char* file_data = (char*)memory_alloc(options->memory_user_data, size);
	if (!file_data)
	{
		fclose(file);
		return cgltf_result_out_of_memory;
	}

	cgltf_size read_size = fread(file_data, 1, size, file);

	fclose(file);

	if (read_size != size)
	{
		memory_free(options->memory_user_data, file_data);
		return cgltf_result_io_error;
	}

	*out_data = file_data;

	return cgltf_result_success;
}

cgltf_result cgltf_load_buffer_base64(const cgltf_options* options, cgltf_size size, const char* base64, void** out_data)
{
	void* (*memory_alloc)(void*, cgltf_size) = options->memory_alloc ? options->memory_alloc : &cgltf_default_alloc;
	void (*memory_free)(void*, void*) = options->memory_free ? options->memory_free : &cgltf_default_free;

	unsigned char* data = (unsigned char*)memory_alloc(options->memory_user_data, size);
	if (!data)
	{
		return cgltf_result_out_of_memory;
	}

	unsigned int buffer = 0;
	unsigned int buffer_bits = 0;

	for (cgltf_size i = 0; i < size; ++i)
	{
		while (buffer_bits < 8)
		{
			char ch = *base64++;

			int index =
				(unsigned)(ch - 'A') < 26 ? (ch - 'A') :
				(unsigned)(ch - 'a') < 26 ? (ch - 'a') + 26 :
				(unsigned)(ch - '0') < 10 ? (ch - '0') + 52 :
				ch == '+' ? 62 :
				ch == '/' ? 63 :
				-1;

			if (index < 0)
			{
				memory_free(options->memory_user_data, data);
				return cgltf_result_io_error;
			}

			buffer = (buffer << 6) | index;
			buffer_bits += 6;
		}

		data[i] = (unsigned char)(buffer >> (buffer_bits - 8));
		buffer_bits -= 8;
	}

	*out_data = data;

	return cgltf_result_success;
}

cgltf_result cgltf_load_buffers(const cgltf_options* options, cgltf_data* data, const char* gltf_path)
{
	if (options == NULL)
	{
		return cgltf_result_invalid_options;
	}

	if (data->buffers_count && data->buffers[0].data == NULL && data->buffers[0].uri == NULL && data->bin)
	{
		if (data->bin_size < data->buffers[0].size)
		{
			return cgltf_result_data_too_short;
		}

		data->buffers[0].data = (void*)data->bin;
	}

	for (cgltf_size i = 0; i < data->buffers_count; ++i)
	{
		if (data->buffers[i].data)
		{
			continue;
		}

		const char* uri = data->buffers[i].uri;

		if (uri == NULL)
		{
			continue;
		}

		if (strncmp(uri, "data:", 5) == 0)
		{
			const char* comma = strchr(uri, ',');

			if (comma && comma - uri >= 7 && strncmp(comma - 7, ";base64", 7) == 0)
			{
				cgltf_result res = cgltf_load_buffer_base64(options, data->buffers[i].size, comma + 1, &data->buffers[i].data);

				if (res != cgltf_result_success)
				{
					return res;
				}
			}
			else
			{
				return cgltf_result_unknown_format;
			}
		}
		else if (strstr(uri, "://") == NULL)
		{
			cgltf_result res = cgltf_load_buffer_file(options, data->buffers[i].size, uri, gltf_path, &data->buffers[i].data);

			if (res != cgltf_result_success)
			{
				return res;
			}
		}
		else
		{
			return cgltf_result_unknown_format;
		}
	}

	return cgltf_result_success;
}

static cgltf_size cgltf_calc_size(cgltf_type type, cgltf_component_type component_type);

static cgltf_size cgltf_calc_index_bound(cgltf_buffer_view* buffer_view, cgltf_size offset, cgltf_component_type component_type, cgltf_size count)
{
	char* data = (char*)buffer_view->buffer->data + offset + buffer_view->offset;
	cgltf_size bound = 0;

	switch (component_type)
	{
	case cgltf_component_type_r_8u:
		for (size_t i = 0; i < count; ++i)
		{
			cgltf_size v = ((unsigned char*)data)[i];
			bound = bound > v ? bound : v;
		}
		break;

	case cgltf_component_type_r_16u:
		for (size_t i = 0; i < count; ++i)
		{
			cgltf_size v = ((unsigned short*)data)[i];
			bound = bound > v ? bound : v;
		}
		break;

	case cgltf_component_type_r_32u:
		for (size_t i = 0; i < count; ++i)
		{
			cgltf_size v = ((unsigned int*)data)[i];
			bound = bound > v ? bound : v;
		}
		break;

	default:
		;
	}

	return bound;
}

cgltf_result cgltf_validate(cgltf_data* data)
{
	for (cgltf_size i = 0; i < data->accessors_count; ++i)
	{
		cgltf_accessor* accessor = &data->accessors[i];

		cgltf_size element_size = cgltf_calc_size(accessor->type, accessor->component_type);

		if (accessor->buffer_view)
		{
			cgltf_size req_size = accessor->offset + accessor->stride * (accessor->count - 1) + element_size;

			if (accessor->buffer_view->size < req_size)
			{
				return cgltf_result_data_too_short;
			}
		}

		if (accessor->is_sparse)
		{
			cgltf_accessor_sparse* sparse = &accessor->sparse;

			cgltf_size indices_component_size = cgltf_calc_size(cgltf_type_scalar, sparse->indices_component_type);
			cgltf_size indices_req_size = sparse->indices_byte_offset + indices_component_size * sparse->count;
			cgltf_size values_req_size = sparse->values_byte_offset + element_size * sparse->count;

			if (sparse->indices_buffer_view->size < indices_req_size ||
				sparse->values_buffer_view->size < values_req_size)
			{
				return cgltf_result_data_too_short;
			}

			if (sparse->indices_component_type != cgltf_component_type_r_8u &&
				sparse->indices_component_type != cgltf_component_type_r_16u &&
				sparse->indices_component_type != cgltf_component_type_r_32u)
			{
				return cgltf_result_invalid_gltf;
			}

			if (sparse->indices_buffer_view->buffer->data)
			{
				cgltf_size index_bound = cgltf_calc_index_bound(sparse->indices_buffer_view, sparse->indices_byte_offset, sparse->indices_component_type, sparse->count);

				if (index_bound >= accessor->count)
				{
					return cgltf_result_data_too_short;
				}
			}
		}
	}

	for (cgltf_size i = 0; i < data->buffer_views_count; ++i)
	{
		cgltf_size req_size = data->buffer_views[i].offset + data->buffer_views[i].size;

		if (data->buffer_views[i].buffer && data->buffer_views[i].buffer->size < req_size)
		{
			return cgltf_result_data_too_short;
		}
	}

	for (cgltf_size i = 0; i < data->meshes_count; ++i)
	{
		if (data->meshes[i].weights)
		{
			if (data->meshes[i].primitives_count && data->meshes[i].primitives[0].targets_count != data->meshes[i].weights_count)
			{
				return cgltf_result_invalid_gltf;
			}
		}

		for (cgltf_size j = 0; j < data->meshes[i].primitives_count; ++j)
		{
			if (data->meshes[i].primitives[j].targets_count != data->meshes[i].primitives[0].targets_count)
			{
				return cgltf_result_invalid_gltf;
			}

			if (data->meshes[i].primitives[j].attributes_count)
			{
				cgltf_accessor* first = data->meshes[i].primitives[j].attributes[0].data;

				for (cgltf_size k = 0; k < data->meshes[i].primitives[j].attributes_count; ++k)
				{
					if (data->meshes[i].primitives[j].attributes[k].data->count != first->count)
					{
						return cgltf_result_invalid_gltf;
					}
				}

				for (cgltf_size k = 0; k < data->meshes[i].primitives[j].targets_count; ++k)
				{
					for (cgltf_size m = 0; m < data->meshes[i].primitives[j].targets[k].attributes_count; ++m)
					{
						if (data->meshes[i].primitives[j].targets[k].attributes[m].data->count != first->count)
						{
							return cgltf_result_invalid_gltf;
						}
					}
				}

				cgltf_accessor* indices = data->meshes[i].primitives[j].indices;

				if (indices &&
					indices->component_type != cgltf_component_type_r_8u &&
					indices->component_type != cgltf_component_type_r_16u &&
					indices->component_type != cgltf_component_type_r_32u)
				{
					return cgltf_result_invalid_gltf;
				}

				if (indices && indices->buffer_view && indices->buffer_view->buffer->data)
				{
					cgltf_size index_bound = cgltf_calc_index_bound(indices->buffer_view, indices->offset, indices->component_type, indices->count);

					if (index_bound >= first->count)
					{
						return cgltf_result_data_too_short;
					}
				}
			}
		}
	}

	for (cgltf_size i = 0; i < data->nodes_count; ++i)
	{
		if (data->nodes[i].weights && data->nodes[i].mesh)
		{
			if (data->nodes[i].mesh->primitives_count && data->nodes[i].mesh->primitives[0].targets_count != data->nodes[i].weights_count)
			{
				return cgltf_result_invalid_gltf;
			}
		}
	}

	return cgltf_result_success;
}

cgltf_result cgltf_copy_extras_json(const cgltf_data* data, const cgltf_extras* extras, char* dest, cgltf_size* dest_size)
{
	cgltf_size json_size = extras->end_offset - extras->start_offset;

	if (!dest)
	{
		if (dest_size)
		{
			*dest_size = json_size + 1;
			return cgltf_result_success;
		}
		return cgltf_result_invalid_options;
	}

	if (*dest_size + 1 < json_size)
	{
		strncpy(dest, data->json + extras->start_offset, *dest_size - 1);
		dest[*dest_size - 1] = 0;
	}
	else
	{
		strncpy(dest, data->json + extras->start_offset, json_size);
		dest[json_size] = 0;
	}

	return cgltf_result_success;
}

void cgltf_free(cgltf_data* data)
{
	if (!data)
	{
		return;
	}

	data->memory_free(data->memory_user_data, data->asset.copyright);
	data->memory_free(data->memory_user_data, data->asset.generator);
	data->memory_free(data->memory_user_data, data->asset.version);
	data->memory_free(data->memory_user_data, data->asset.min_version);

	data->memory_free(data->memory_user_data, data->accessors);
	data->memory_free(data->memory_user_data, data->buffer_views);

	for (cgltf_size i = 0; i < data->buffers_count; ++i)
	{
		if (data->buffers[i].data != data->bin)
		{
			data->memory_free(data->memory_user_data, data->buffers[i].data);
		}

		data->memory_free(data->memory_user_data, data->buffers[i].uri);
	}

	data->memory_free(data->memory_user_data, data->buffers);

	for (cgltf_size i = 0; i < data->meshes_count; ++i)
	{
		data->memory_free(data->memory_user_data, data->meshes[i].name);

		for (cgltf_size j = 0; j < data->meshes[i].primitives_count; ++j)
		{
			for (cgltf_size k = 0; k < data->meshes[i].primitives[j].attributes_count; ++k)
			{
				data->memory_free(data->memory_user_data, data->meshes[i].primitives[j].attributes[k].name);
			}

			data->memory_free(data->memory_user_data, data->meshes[i].primitives[j].attributes);

			for (cgltf_size k = 0; k < data->meshes[i].primitives[j].targets_count; ++k)
			{
				for (cgltf_size m = 0; m < data->meshes[i].primitives[j].targets[k].attributes_count; ++m)
				{
					data->memory_free(data->memory_user_data, data->meshes[i].primitives[j].targets[k].attributes[m].name);
				}

				data->memory_free(data->memory_user_data, data->meshes[i].primitives[j].targets[k].attributes);
			}

			data->memory_free(data->memory_user_data, data->meshes[i].primitives[j].targets);
		}

		data->memory_free(data->memory_user_data, data->meshes[i].primitives);
		data->memory_free(data->memory_user_data, data->meshes[i].weights);
	}

	data->memory_free(data->memory_user_data, data->meshes);

	for (cgltf_size i = 0; i < data->materials_count; ++i)
	{
		data->memory_free(data->memory_user_data, data->materials[i].name);
	}

	data->memory_free(data->memory_user_data, data->materials);

	for (cgltf_size i = 0; i < data->images_count; ++i) 
	{
		data->memory_free(data->memory_user_data, data->images[i].name);
		data->memory_free(data->memory_user_data, data->images[i].uri);
		data->memory_free(data->memory_user_data, data->images[i].mime_type);
	}

	data->memory_free(data->memory_user_data, data->images);

	for (cgltf_size i = 0; i < data->textures_count; ++i)
	{
		data->memory_free(data->memory_user_data, data->textures[i].name);
	}

	data->memory_free(data->memory_user_data, data->textures);

	data->memory_free(data->memory_user_data, data->samplers);

	for (cgltf_size i = 0; i < data->skins_count; ++i)
	{
		data->memory_free(data->memory_user_data, data->skins[i].name);
		data->memory_free(data->memory_user_data, data->skins[i].joints);
	}

	data->memory_free(data->memory_user_data, data->skins);

	for (cgltf_size i = 0; i < data->cameras_count; ++i)
	{
		data->memory_free(data->memory_user_data, data->cameras[i].name);
	}

	data->memory_free(data->memory_user_data, data->cameras);

	for (cgltf_size i = 0; i < data->lights_count; ++i)
	{
		data->memory_free(data->memory_user_data, data->lights[i].name);
	}

	data->memory_free(data->memory_user_data, data->lights);

	for (cgltf_size i = 0; i < data->nodes_count; ++i)
	{
		data->memory_free(data->memory_user_data, data->nodes[i].name);
		data->memory_free(data->memory_user_data, data->nodes[i].children);
		data->memory_free(data->memory_user_data, data->nodes[i].weights);
	}

	data->memory_free(data->memory_user_data, data->nodes);

	for (cgltf_size i = 0; i < data->scenes_count; ++i)
	{
		data->memory_free(data->memory_user_data, data->scenes[i].name);
		data->memory_free(data->memory_user_data, data->scenes[i].nodes);
	}

	data->memory_free(data->memory_user_data, data->scenes);

	for (cgltf_size i = 0; i < data->animations_count; ++i)
	{
		data->memory_free(data->memory_user_data, data->animations[i].name);
		data->memory_free(data->memory_user_data, data->animations[i].samplers);
		data->memory_free(data->memory_user_data, data->animations[i].channels);
	}

	data->memory_free(data->memory_user_data, data->animations);

	data->memory_free(data->memory_user_data, data->file_data);

	data->memory_free(data->memory_user_data, data);
}

void cgltf_node_transform_local(const cgltf_node* node, cgltf_float* out_matrix)
{
	cgltf_float* lm = out_matrix;

	if (node->has_matrix)
	{
		memcpy(lm, node->matrix, sizeof(float) * 16);
	}
	else
	{
		float tx = node->translation[0];
		float ty = node->translation[1];
		float tz = node->translation[2];

		float qx = node->rotation[0];
		float qy = node->rotation[1];
		float qz = node->rotation[2];
		float qw = node->rotation[3];

		float sx = node->scale[0];
		float sy = node->scale[1];
		float sz = node->scale[2];

		lm[0] = (1 - 2 * qy*qy - 2 * qz*qz) * sx;
		lm[1] = (2 * qx*qy + 2 * qz*qw) * sy;
		lm[2] = (2 * qx*qz - 2 * qy*qw) * sz;
		lm[3] = 0.f;

		lm[4] = (2 * qx*qy - 2 * qz*qw) * sx;
		lm[5] = (1 - 2 * qx*qx - 2 * qz*qz) * sy;
		lm[6] = (2 * qy*qz + 2 * qx*qw) * sz;
		lm[7] = 0.f;

		lm[8] = (2 * qx*qz + 2 * qy*qw) * sx;
		lm[9] = (2 * qy*qz - 2 * qx*qw) * sy;
		lm[10] = (1 - 2 * qx*qx - 2 * qy*qy) * sz;
		lm[11] = 0.f;

		lm[12] = tx;
		lm[13] = ty;
		lm[14] = tz;
		lm[15] = 1.f;
	}
}

void cgltf_node_transform_world(const cgltf_node* node, cgltf_float* out_matrix)
{
	cgltf_float* lm = out_matrix;
	cgltf_node_transform_local(node, lm);

	const cgltf_node* parent = node->parent;

	while (parent)
	{
		float pm[16];
		cgltf_node_transform_local(parent, pm);

		for (int i = 0; i < 4; ++i)
		{
			float l0 = lm[i * 4 + 0];
			float l1 = lm[i * 4 + 1];
			float l2 = lm[i * 4 + 2];

			float r0 = l0 * pm[0] + l1 * pm[4] + l2 * pm[8];
			float r1 = l0 * pm[1] + l1 * pm[5] + l2 * pm[9];
			float r2 = l0 * pm[2] + l1 * pm[6] + l2 * pm[10];

			lm[i * 4 + 0] = r0;
			lm[i * 4 + 1] = r1;
			lm[i * 4 + 2] = r2;
		}

		lm[12] += pm[12];
		lm[13] += pm[13];
		lm[14] += pm[14];

		parent = parent->parent;
	}
}

static cgltf_size cgltf_component_read_index(const void* in, cgltf_component_type component_type)
{
	switch (component_type)
	{
		case cgltf_component_type_r_16:
			return *((const int16_t*) in);
		case cgltf_component_type_r_16u:
			return *((const uint16_t*) in);
		case cgltf_component_type_r_32u:
			return *((const uint32_t*) in);
		case cgltf_component_type_r_32f:
			return (cgltf_size)*((const float*) in);
		case cgltf_component_type_r_8:
			return *((const int8_t*) in);
		case cgltf_component_type_r_8u:
		case cgltf_component_type_invalid:
		default:
			return *((const uint8_t*) in);
	}
}

static cgltf_float cgltf_component_read_float(const void* in, cgltf_component_type component_type, cgltf_bool normalized)
{
	if (component_type == cgltf_component_type_r_32f)
	{
		return *((const float*) in);
	}

	if (normalized)
	{
		switch (component_type)
		{
			case cgltf_component_type_r_32u:
				return *((const uint32_t*) in) / (float) UINT_MAX;
			case cgltf_component_type_r_16:
				return *((const int16_t*) in) / (float) SHRT_MAX;
			case cgltf_component_type_r_16u:
				return *((const uint16_t*) in) / (float) USHRT_MAX;
			case cgltf_component_type_r_8:
				return *((const int8_t*) in) / (float) SCHAR_MAX;
			case cgltf_component_type_r_8u:
			case cgltf_component_type_invalid:
			default:
				return *((const uint8_t*) in) / (float) CHAR_MAX;
		}
	}

	return (cgltf_float)cgltf_component_read_index(in, component_type);
}

static cgltf_size cgltf_num_components(cgltf_type type);
static cgltf_size cgltf_component_size(cgltf_component_type component_type);

static cgltf_bool cgltf_element_read_float(const uint8_t* element, cgltf_type type, cgltf_component_type component_type, cgltf_bool normalized, cgltf_float* out, cgltf_size element_size)
{
	cgltf_size num_components = cgltf_num_components(type);

	if (element_size < num_components) {
		return 0;
	}

	// There are three special cases for component extraction, see #data-alignment in the 2.0 spec.

	cgltf_size component_size = cgltf_component_size(component_type);

	if (type == cgltf_type_mat2 && component_size == 1)
	{
		out[0] = cgltf_component_read_float(element, component_type, normalized);
		out[1] = cgltf_component_read_float(element + 1, component_type, normalized);
		out[2] = cgltf_component_read_float(element + 4, component_type, normalized);
		out[3] = cgltf_component_read_float(element + 5, component_type, normalized);
		return 1;
	}

	if (type == cgltf_type_mat3 && component_size == 1)
	{
		out[0] = cgltf_component_read_float(element, component_type, normalized);
		out[1] = cgltf_component_read_float(element + 1, component_type, normalized);
		out[2] = cgltf_component_read_float(element + 2, component_type, normalized);
		out[3] = cgltf_component_read_float(element + 4, component_type, normalized);
		out[4] = cgltf_component_read_float(element + 5, component_type, normalized);
		out[5] = cgltf_component_read_float(element + 6, component_type, normalized);
		out[6] = cgltf_component_read_float(element + 8, component_type, normalized);
		out[7] = cgltf_component_read_float(element + 9, component_type, normalized);
		out[8] = cgltf_component_read_float(element + 10, component_type, normalized);
		return 1;
	}

	if (type == cgltf_type_mat3 && component_size == 2)
	{
		out[0] = cgltf_component_read_float(element, component_type, normalized);
		out[1] = cgltf_component_read_float(element + 2, component_type, normalized);
		out[2] = cgltf_component_read_float(element + 4, component_type, normalized);
		out[3] = cgltf_component_read_float(element + 8, component_type, normalized);
		out[4] = cgltf_component_read_float(element + 10, component_type, normalized);
		out[5] = cgltf_component_read_float(element + 12, component_type, normalized);
		out[6] = cgltf_component_read_float(element + 16, component_type, normalized);
		out[7] = cgltf_component_read_float(element + 18, component_type, normalized);
		out[8] = cgltf_component_read_float(element + 20, component_type, normalized);
		return 1;
	}

	for (cgltf_size i = 0; i < num_components; ++i)
	{
		out[i] = cgltf_component_read_float(element + component_size * i, component_type, normalized);
	}
	return 1;
}


cgltf_bool cgltf_accessor_read_float(const cgltf_accessor* accessor, cgltf_size index, cgltf_float* out, cgltf_size element_size)
{
	if (accessor->is_sparse || accessor->buffer_view == NULL)
	{
		return 0;
	}

	cgltf_size offset = accessor->offset + accessor->buffer_view->offset;
	const uint8_t* element = (const uint8_t*) accessor->buffer_view->buffer->data;
	element += offset + accessor->stride * index;
	return cgltf_element_read_float(element, accessor->type, accessor->component_type, accessor->normalized, out, element_size);
}

cgltf_size cgltf_accessor_read_index(const cgltf_accessor* accessor, cgltf_size index)
{
	if (accessor->buffer_view)
	{
		cgltf_size offset = accessor->offset + accessor->buffer_view->offset;
		const uint8_t* element = (const uint8_t*) accessor->buffer_view->buffer->data;
		element += offset + accessor->stride * index;
		return cgltf_component_read_index(element, accessor->component_type);
	}

	return 0;
}

#define CGLTF_ERROR_JSON -1
#define CGLTF_ERROR_NOMEM -2

#define CGLTF_CHECK_TOKTYPE(tok_, type_) if ((tok_).type != (type_)) { return CGLTF_ERROR_JSON; }
#define CGLTF_CHECK_KEY(tok_) if ((tok_).type != JSMN_STRING || (tok_).size == 0) { return CGLTF_ERROR_JSON; } /* checking size for 0 verifies that a value follows the key */

#define CGLTF_PTRINDEX(type, idx) (type*)(cgltf_size)(idx + 1)
#define CGLTF_PTRFIXUP(var, data, size) if (var) { if ((cgltf_size)var > size) { return CGLTF_ERROR_JSON; } var = &data[(cgltf_size)var-1]; }
#define CGLTF_PTRFIXUP_REQ(var, data, size) if (!var || (cgltf_size)var > size) { return CGLTF_ERROR_JSON; } var = &data[(cgltf_size)var-1];

static int cgltf_json_strcmp(jsmntok_t const* tok, const uint8_t* json_chunk, const char* str)
{
	CGLTF_CHECK_TOKTYPE(*tok, JSMN_STRING);
	size_t const str_len = strlen(str);
	size_t const name_length = tok->end - tok->start;
	return (str_len == name_length) ? strncmp((const char*)json_chunk + tok->start, str, str_len) : 128;
}

static int cgltf_json_to_int(jsmntok_t const* tok, const uint8_t* json_chunk)
{
	CGLTF_CHECK_TOKTYPE(*tok, JSMN_PRIMITIVE);
	char tmp[128];
	int size = (cgltf_size)(tok->end - tok->start) < sizeof(tmp) ? tok->end - tok->start : sizeof(tmp) - 1;
	strncpy(tmp, (const char*)json_chunk + tok->start, size);
	tmp[size] = 0;
	return atoi(tmp);
}

static cgltf_float cgltf_json_to_float(jsmntok_t const* tok, const uint8_t* json_chunk)
{
	CGLTF_CHECK_TOKTYPE(*tok, JSMN_PRIMITIVE);
	char tmp[128];
	int size = (cgltf_size)(tok->end - tok->start) < sizeof(tmp) ? tok->end - tok->start : sizeof(tmp) - 1;
	strncpy(tmp, (const char*)json_chunk + tok->start, size);
	tmp[size] = 0;
	return (cgltf_float)atof(tmp);
}

static cgltf_bool cgltf_json_to_bool(jsmntok_t const* tok, const uint8_t* json_chunk)
{
	int size = tok->end - tok->start;
	return size == 4 && memcmp(json_chunk + tok->start, "true", 4) == 0;
}

static int cgltf_skip_json(jsmntok_t const* tokens, int i)
{
	if (tokens[i].type == JSMN_ARRAY)
	{
		int size = tokens[i].size;
		++i;
		for (int j = 0; j < size; ++j)
		{
			i = cgltf_skip_json(tokens, i);
			if (i < 0)
			{
				return i;
			}
		}
	}
	else if (tokens[i].type == JSMN_OBJECT)
	{
		int size = tokens[i].size;
		++i;
		for (int j = 0; j < size; ++j)
		{
			CGLTF_CHECK_KEY(tokens[i]);
			++i;
			i = cgltf_skip_json(tokens, i);
			if (i < 0)
			{
				return i;
			}
		}
	}
	else if (tokens[i].type == JSMN_PRIMITIVE
		 || tokens[i].type == JSMN_STRING)
	{
		return i + 1;
	}
	return i;
}

static void cgltf_fill_float_array(float* out_array, int size, float value)
{
	for (int j = 0; j < size; ++j)
	{
		out_array[j] = value;
	}
}

static int cgltf_parse_json_float_array(jsmntok_t const* tokens, int i, const uint8_t* json_chunk, float* out_array, int size)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_ARRAY);
	if (tokens[i].size != size)
	{
		return CGLTF_ERROR_JSON;
	}
	++i;
	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
		out_array[j] = cgltf_json_to_float(tokens + i, json_chunk);
		++i;
	}
	return i;
}

static int cgltf_parse_json_string(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, char** out_string)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_STRING);
	if (*out_string)
	{
		return CGLTF_ERROR_JSON;
	}
	int size = tokens[i].end - tokens[i].start;
	char* result = (char*)options->memory_alloc(options->memory_user_data, size + 1);
	if (!result)
	{
		return CGLTF_ERROR_NOMEM;
	}
	strncpy(result, (const char*)json_chunk + tokens[i].start, size);
	result[size] = 0;
	*out_string = result;
	return i + 1;
}

static int cgltf_parse_json_array(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, size_t element_size, void** out_array, cgltf_size* out_size)
{
	(void)json_chunk;
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_ARRAY);
	if (*out_array)
	{
		return CGLTF_ERROR_JSON;
	}
	int size = tokens[i].size;
	void* result = cgltf_calloc(options, element_size, size);
	if (!result)
	{
		return CGLTF_ERROR_NOMEM;
	}
	*out_array = result;
	*out_size = size;
	return i + 1;
}

static int cgltf_parse_json_string_array(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, char*** out_array, cgltf_size* out_size)
{
    CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_ARRAY);
    i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(char*), (void**)out_array, out_size);

    for (cgltf_size j = 0; j < *out_size; ++j)
    {
        i = cgltf_parse_json_string(options, tokens, i, json_chunk, j + (*out_array));
        if (i < 0)
        {
            return i;
        }
    }
    return i;
}

static void cgltf_parse_attribute_type(const char* name, cgltf_attribute_type* out_type, int* out_index)
{
	const char* us = strchr(name, '_');
	size_t len = us ? us - name : strlen(name);

	if (len == 8 && strncmp(name, "POSITION", 8) == 0)
	{
		*out_type = cgltf_attribute_type_position;
	}
	else if (len == 6 && strncmp(name, "NORMAL", 6) == 0)
	{
		*out_type = cgltf_attribute_type_normal;
	}
	else if (len == 7 && strncmp(name, "TANGENT", 7) == 0)
	{
		*out_type = cgltf_attribute_type_tangent;
	}
	else if (len == 8 && strncmp(name, "TEXCOORD", 8) == 0)
	{
		*out_type = cgltf_attribute_type_texcoord;
	}
	else if (len == 5 && strncmp(name, "COLOR", 5) == 0)
	{
		*out_type = cgltf_attribute_type_color;
	}
	else if (len == 6 && strncmp(name, "JOINTS", 6) == 0)
	{
		*out_type = cgltf_attribute_type_joints;
	}
	else if (len == 7 && strncmp(name, "WEIGHTS", 7) == 0)
	{
		*out_type = cgltf_attribute_type_weights;
	}
	else
	{
		*out_type = cgltf_attribute_type_invalid;
	}

	if (us && *out_type != cgltf_attribute_type_invalid)
	{
		*out_index = atoi(us + 1);
	}
}

static int cgltf_parse_json_attribute_list(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_attribute** out_attributes, cgltf_size* out_attributes_count)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	if (*out_attributes)
	{
		return CGLTF_ERROR_JSON;
	}

	*out_attributes_count = tokens[i].size;
	*out_attributes = (cgltf_attribute*)cgltf_calloc(options, sizeof(cgltf_attribute), *out_attributes_count);
	++i;

	if (!*out_attributes)
	{
		return CGLTF_ERROR_NOMEM;
	}

	for (cgltf_size j = 0; j < *out_attributes_count; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		i = cgltf_parse_json_string(options, tokens, i, json_chunk, &(*out_attributes)[j].name);
		if (i < 0)
		{
			return CGLTF_ERROR_JSON;
		}

		cgltf_parse_attribute_type((*out_attributes)[j].name, &(*out_attributes)[j].type, &(*out_attributes)[j].index);

		(*out_attributes)[j].data = CGLTF_PTRINDEX(cgltf_accessor, cgltf_json_to_int(tokens + i, json_chunk));
		++i;
	}

	return i;
}

static int cgltf_parse_json_extras(jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_extras* out_extras)
{
	(void)json_chunk;
	out_extras->start_offset = tokens[i].start;
	out_extras->end_offset = tokens[i].end;
	i = cgltf_skip_json(tokens, i);
	return i;
}

static int cgltf_parse_json_primitive(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_primitive* out_prim)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	out_prim->type = cgltf_primitive_type_triangles;

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "mode") == 0)
		{
			++i;
			out_prim->type
					= (cgltf_primitive_type)
					cgltf_json_to_int(tokens+i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "indices") == 0)
		{
			++i;
			out_prim->indices = CGLTF_PTRINDEX(cgltf_accessor, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "material") == 0)
		{
			++i;
			out_prim->material = CGLTF_PTRINDEX(cgltf_material, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "attributes") == 0)
		{
			i = cgltf_parse_json_attribute_list(options, tokens, i + 1, json_chunk, &out_prim->attributes, &out_prim->attributes_count);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "targets") == 0)
		{
			i = cgltf_parse_json_array(options, tokens, i + 1, json_chunk, sizeof(cgltf_morph_target), (void**)&out_prim->targets, &out_prim->targets_count);
			if (i < 0)
			{
				return i;
			}

			for (cgltf_size k = 0; k < out_prim->targets_count; ++k)
			{
				i = cgltf_parse_json_attribute_list(options, tokens, i, json_chunk, &out_prim->targets[k].attributes, &out_prim->targets[k].attributes_count);
				if (i < 0)
				{
					return i;
				}
			}
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_prim->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_mesh(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_mesh* out_mesh)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "name") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_mesh->name);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "primitives") == 0)
		{
			i = cgltf_parse_json_array(options, tokens, i + 1, json_chunk, sizeof(cgltf_primitive), (void**)&out_mesh->primitives, &out_mesh->primitives_count);
			if (i < 0)
			{
				return i;
			}

			for (cgltf_size prim_index = 0; prim_index < out_mesh->primitives_count; ++prim_index)
			{
				i = cgltf_parse_json_primitive(options, tokens, i, json_chunk, &out_mesh->primitives[prim_index]);
				if (i < 0)
				{
					return i;
				}
			}
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "weights") == 0)
		{
			i = cgltf_parse_json_array(options, tokens, i + 1, json_chunk, sizeof(cgltf_float), (void**)&out_mesh->weights, &out_mesh->weights_count);
			if (i < 0)
			{
				return i;
			}

			i = cgltf_parse_json_float_array(tokens, i - 1, json_chunk, out_mesh->weights, (int)out_mesh->weights_count);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_mesh->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_meshes(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_mesh), (void**)&out_data->meshes, &out_data->meshes_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->meshes_count; ++j)
	{
		i = cgltf_parse_json_mesh(options, tokens, i, json_chunk, &out_data->meshes[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static cgltf_component_type cgltf_json_to_component_type(jsmntok_t const* tok, const uint8_t* json_chunk)
{
	int type = cgltf_json_to_int(tok, json_chunk);

	switch (type)
	{
	case 5120:
		return cgltf_component_type_r_8;
	case 5121:
		return cgltf_component_type_r_8u;
	case 5122:
		return cgltf_component_type_r_16;
	case 5123:
		return cgltf_component_type_r_16u;
	case 5125:
		return cgltf_component_type_r_32u;
	case 5126:
		return cgltf_component_type_r_32f;
	default:
		return cgltf_component_type_invalid;
	}
}

static int cgltf_parse_json_accessor_sparse(jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_accessor_sparse* out_sparse)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "count") == 0)
		{
			++i;
			out_sparse->count = cgltf_json_to_int(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "indices") == 0)
		{
			++i;
			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

			int indices_size = tokens[i].size;
			++i;

			for (int k = 0; k < indices_size; ++k)
			{
				CGLTF_CHECK_KEY(tokens[i]);

				if (cgltf_json_strcmp(tokens+i, json_chunk, "bufferView") == 0)
				{
					++i;
					out_sparse->indices_buffer_view = CGLTF_PTRINDEX(cgltf_buffer_view, cgltf_json_to_int(tokens + i, json_chunk));
					++i;
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "byteOffset") == 0)
				{
					++i;
					out_sparse->indices_byte_offset = cgltf_json_to_int(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "componentType") == 0)
				{
					++i;
					out_sparse->indices_component_type = cgltf_json_to_component_type(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
				{
					i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_sparse->indices_extras);
				}
				else
				{
					i = cgltf_skip_json(tokens, i+1);
				}

				if (i < 0)
				{
					return i;
				}
			}
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "values") == 0)
		{
			++i;
			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

			int values_size = tokens[i].size;
			++i;

			for (int k = 0; k < values_size; ++k)
			{
				CGLTF_CHECK_KEY(tokens[i]);

				if (cgltf_json_strcmp(tokens+i, json_chunk, "bufferView") == 0)
				{
					++i;
					out_sparse->values_buffer_view = CGLTF_PTRINDEX(cgltf_buffer_view, cgltf_json_to_int(tokens + i, json_chunk));
					++i;
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "byteOffset") == 0)
				{
					++i;
					out_sparse->values_byte_offset = cgltf_json_to_int(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
				{
					i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_sparse->values_extras);
				}
				else
				{
					i = cgltf_skip_json(tokens, i+1);
				}

				if (i < 0)
				{
					return i;
				}
			}
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_sparse->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_accessor(jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_accessor* out_accessor)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "bufferView") == 0)
		{
			++i;
			out_accessor->buffer_view = CGLTF_PTRINDEX(cgltf_buffer_view, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "byteOffset") == 0)
		{
			++i;
			out_accessor->offset =
					cgltf_json_to_int(tokens+i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "componentType") == 0)
		{
			++i;
			out_accessor->component_type = cgltf_json_to_component_type(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "normalized") == 0)
		{
			++i;
			out_accessor->normalized = cgltf_json_to_bool(tokens+i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "count") == 0)
		{
			++i;
			out_accessor->count =
					cgltf_json_to_int(tokens+i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "type") == 0)
		{
			++i;
			if (cgltf_json_strcmp(tokens+i, json_chunk, "SCALAR") == 0)
			{
				out_accessor->type = cgltf_type_scalar;
			}
			else if (cgltf_json_strcmp(tokens+i, json_chunk, "VEC2") == 0)
			{
				out_accessor->type = cgltf_type_vec2;
			}
			else if (cgltf_json_strcmp(tokens+i, json_chunk, "VEC3") == 0)
			{
				out_accessor->type = cgltf_type_vec3;
			}
			else if (cgltf_json_strcmp(tokens+i, json_chunk, "VEC4") == 0)
			{
				out_accessor->type = cgltf_type_vec4;
			}
			else if (cgltf_json_strcmp(tokens+i, json_chunk, "MAT2") == 0)
			{
				out_accessor->type = cgltf_type_mat2;
			}
			else if (cgltf_json_strcmp(tokens+i, json_chunk, "MAT3") == 0)
			{
				out_accessor->type = cgltf_type_mat3;
			}
			else if (cgltf_json_strcmp(tokens+i, json_chunk, "MAT4") == 0)
			{
				out_accessor->type = cgltf_type_mat4;
			}
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "min") == 0)
		{
			++i;
			out_accessor->has_min = 1;
			// note: we can't parse the precise number of elements since type may not have been computed yet
			int min_size = tokens[i].size > 16 ? 16 : tokens[i].size;
			i = cgltf_parse_json_float_array(tokens, i, json_chunk, out_accessor->min, min_size);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "max") == 0)
		{
			++i;
			out_accessor->has_max = 1;
			// note: we can't parse the precise number of elements since type may not have been computed yet
			int max_size = tokens[i].size > 16 ? 16 : tokens[i].size;
			i = cgltf_parse_json_float_array(tokens, i, json_chunk, out_accessor->max, max_size);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "sparse") == 0)
		{
			out_accessor->is_sparse = 1;
			i = cgltf_parse_json_accessor_sparse(tokens, i + 1, json_chunk, &out_accessor->sparse);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_accessor->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_texture_transform(jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_texture_transform* out_texture_transform)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens + i, json_chunk, "offset") == 0)
		{
			i = cgltf_parse_json_float_array(tokens, i + 1, json_chunk, out_texture_transform->offset, 2);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "rotation") == 0)
		{
			++i;
			out_texture_transform->rotation = cgltf_json_to_float(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "scale") == 0)
		{
			i = cgltf_parse_json_float_array(tokens, i + 1, json_chunk, out_texture_transform->scale, 2);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "texCoord") == 0)
		{
			++i;
			out_texture_transform->texcoord = cgltf_json_to_int(tokens + i, json_chunk);
			++i;
		}
		else
		{
			i = cgltf_skip_json(tokens, i + 1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_texture_view(jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_texture_view* out_texture_view)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	out_texture_view->scale = 1.0f;
	cgltf_fill_float_array(out_texture_view->transform.scale, 2, 1.0f);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens + i, json_chunk, "index") == 0)
		{
			++i;
			out_texture_view->texture = CGLTF_PTRINDEX(cgltf_texture, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "texCoord") == 0)
		{
			++i;
			out_texture_view->texcoord = cgltf_json_to_int(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "scale") == 0) 
		{
			++i;
			out_texture_view->scale = cgltf_json_to_float(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "strength") == 0)
		{
			++i;
			out_texture_view->scale = cgltf_json_to_float(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_texture_view->extras);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extensions") == 0)
		{
			++i;

			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

			int extensions_size = tokens[i].size;
			++i;

			for (int k = 0; k < extensions_size; ++k)
			{
				if (cgltf_json_strcmp(tokens+i, json_chunk, "KHR_texture_transform") == 0)
				{
					out_texture_view->has_transform = 1;
					i = cgltf_parse_json_texture_transform(tokens, i + 1, json_chunk, &out_texture_view->transform);
				}
				else
				{
					i = cgltf_skip_json(tokens, i+1);
				}
			}
		}
		else
		{
			i = cgltf_skip_json(tokens, i + 1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_pbr_metallic_roughness(jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_pbr_metallic_roughness* out_pbr)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "metallicFactor") == 0)
		{
			++i;
			out_pbr->metallic_factor = 
				cgltf_json_to_float(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "roughnessFactor") == 0) 
		{
			++i;
			out_pbr->roughness_factor =
				cgltf_json_to_float(tokens+i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "baseColorFactor") == 0)
		{
			i = cgltf_parse_json_float_array(tokens, i + 1, json_chunk, out_pbr->base_color_factor, 4);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "baseColorTexture") == 0)
		{
			i = cgltf_parse_json_texture_view(tokens, i + 1, json_chunk,
				&out_pbr->base_color_texture);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "metallicRoughnessTexture") == 0)
		{
			i = cgltf_parse_json_texture_view(tokens, i + 1, json_chunk,
				&out_pbr->metallic_roughness_texture);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_pbr->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_pbr_specular_glossiness(jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_pbr_specular_glossiness* out_pbr)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "diffuseFactor") == 0)
		{
			i = cgltf_parse_json_float_array(tokens, i + 1, json_chunk, out_pbr->diffuse_factor, 4);
			if (i < 0)
			{
				return i;
			}
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "specularFactor") == 0)
		{
			i = cgltf_parse_json_float_array(tokens, i + 1, json_chunk, out_pbr->specular_factor, 3);
			if (i < 0)
			{
				return i;
			}
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "glossinessFactor") == 0)
		{
			++i;
			out_pbr->glossiness_factor = cgltf_json_to_float(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "diffuseTexture") == 0)
		{
			i = cgltf_parse_json_texture_view(tokens, i + 1, json_chunk, &out_pbr->diffuse_texture);
			if (i < 0)
			{
				return i;
			}
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "specularGlossinessTexture") == 0)
		{
			i = cgltf_parse_json_texture_view(tokens, i + 1, json_chunk, &out_pbr->specular_glossiness_texture);
			if (i < 0)
			{
				return i;
			}
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}

	}

	return i;
}

static int cgltf_parse_json_image(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_image* out_image)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j) 
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens + i, json_chunk, "uri") == 0) 
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_image->uri);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "bufferView") == 0)
		{
			++i;
			out_image->buffer_view = CGLTF_PTRINDEX(cgltf_buffer_view, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "mimeType") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_image->mime_type);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "name") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_image->name);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_image->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i + 1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_sampler(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_sampler* out_sampler)
{
	(void)options;
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	out_sampler->wrap_s = 10497;
	out_sampler->wrap_t = 10497;

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens + i, json_chunk, "magFilter") == 0) 
		{
			++i;
			out_sampler->mag_filter
				= cgltf_json_to_int(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "minFilter") == 0)
		{
			++i;
			out_sampler->min_filter
				= cgltf_json_to_int(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "wrapS") == 0)
		{
			++i;
			out_sampler->wrap_s
				= cgltf_json_to_int(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "wrapT") == 0) 
		{
			++i;
			out_sampler->wrap_t
				= cgltf_json_to_int(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_sampler->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i + 1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}


static int cgltf_parse_json_texture(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_texture* out_texture)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "name") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_texture->name);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "sampler") == 0)
		{
			++i;
			out_texture->sampler = CGLTF_PTRINDEX(cgltf_sampler, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "source") == 0) 
		{
			++i;
			out_texture->image = CGLTF_PTRINDEX(cgltf_image, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_texture->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i + 1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_material(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_material* out_material)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	cgltf_fill_float_array(out_material->pbr_metallic_roughness.base_color_factor, 4, 1.0f);
	out_material->pbr_metallic_roughness.metallic_factor = 1.0f;
	out_material->pbr_metallic_roughness.roughness_factor = 1.0f;

	cgltf_fill_float_array(out_material->pbr_specular_glossiness.diffuse_factor, 4, 1.0f);
	cgltf_fill_float_array(out_material->pbr_specular_glossiness.specular_factor, 3, 1.0f);
	out_material->pbr_specular_glossiness.glossiness_factor = 1.0f;

	out_material->alpha_cutoff = 0.5f;

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "name") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_material->name);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "pbrMetallicRoughness") == 0)
		{
			out_material->has_pbr_metallic_roughness = 1;
			i = cgltf_parse_json_pbr_metallic_roughness(tokens, i + 1, json_chunk, &out_material->pbr_metallic_roughness);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "emissiveFactor") == 0)
		{
			i = cgltf_parse_json_float_array(tokens, i + 1, json_chunk, out_material->emissive_factor, 3);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "normalTexture") == 0)
		{
			i = cgltf_parse_json_texture_view(tokens, i + 1, json_chunk,
				&out_material->normal_texture);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "occlusionTexture") == 0)
		{
			i = cgltf_parse_json_texture_view(tokens, i + 1, json_chunk,
				&out_material->occlusion_texture);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "emissiveTexture") == 0)
		{
			i = cgltf_parse_json_texture_view(tokens, i + 1, json_chunk,
				&out_material->emissive_texture);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "alphaMode") == 0)
		{
			++i;
			if (cgltf_json_strcmp(tokens + i, json_chunk, "OPAQUE") == 0)
			{
				out_material->alpha_mode = cgltf_alpha_mode_opaque;
			}
			else if (cgltf_json_strcmp(tokens + i, json_chunk, "MASK") == 0)
			{
				out_material->alpha_mode = cgltf_alpha_mode_mask;
			}
			else if (cgltf_json_strcmp(tokens + i, json_chunk, "BLEND") == 0)
			{
				out_material->alpha_mode = cgltf_alpha_mode_blend;
			}
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "alphaCutoff") == 0)
		{
			++i;
			out_material->alpha_cutoff = cgltf_json_to_float(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "doubleSided") == 0)
		{
			++i;
			out_material->double_sided =
				cgltf_json_to_bool(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_material->extras);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extensions") == 0)
		{
			++i;

			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

			int extensions_size = tokens[i].size;
			++i;

			for (int k = 0; k < extensions_size; ++k)
			{
				CGLTF_CHECK_KEY(tokens[i]);

				if (cgltf_json_strcmp(tokens+i, json_chunk, "KHR_materials_pbrSpecularGlossiness") == 0)
				{
					out_material->has_pbr_specular_glossiness = 1;
					i = cgltf_parse_json_pbr_specular_glossiness(tokens, i + 1, json_chunk, &out_material->pbr_specular_glossiness);
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "KHR_materials_unlit") == 0)
				{
					out_material->unlit = 1;
					i = cgltf_skip_json(tokens, i+1);
				}
				else
				{
					i = cgltf_skip_json(tokens, i+1);
				}

				if (i < 0)
				{
					return i;
				}
			}
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_accessors(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_accessor), (void**)&out_data->accessors, &out_data->accessors_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->accessors_count; ++j)
	{
		i = cgltf_parse_json_accessor(tokens, i, json_chunk, &out_data->accessors[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_materials(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_material), (void**)&out_data->materials, &out_data->materials_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->materials_count; ++j)
	{
		i = cgltf_parse_json_material(options, tokens, i, json_chunk, &out_data->materials[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_images(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_image), (void**)&out_data->images, &out_data->images_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->images_count; ++j)
	{
		i = cgltf_parse_json_image(options, tokens, i, json_chunk, &out_data->images[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_textures(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_texture), (void**)&out_data->textures, &out_data->textures_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->textures_count; ++j)
	{
		i = cgltf_parse_json_texture(options, tokens, i, json_chunk, &out_data->textures[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_samplers(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_sampler), (void**)&out_data->samplers, &out_data->samplers_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->samplers_count; ++j)
	{
		i = cgltf_parse_json_sampler(options, tokens, i, json_chunk, &out_data->samplers[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_buffer_view(jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_buffer_view* out_buffer_view)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "buffer") == 0)
		{
			++i;
			out_buffer_view->buffer = CGLTF_PTRINDEX(cgltf_buffer, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "byteOffset") == 0)
		{
			++i;
			out_buffer_view->offset =
					cgltf_json_to_int(tokens+i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "byteLength") == 0)
		{
			++i;
			out_buffer_view->size =
					cgltf_json_to_int(tokens+i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "byteStride") == 0)
		{
			++i;
			out_buffer_view->stride =
					cgltf_json_to_int(tokens+i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "target") == 0)
		{
			++i;
			int type = cgltf_json_to_int(tokens+i, json_chunk);
			switch (type)
			{
			case 34962:
				type = cgltf_buffer_view_type_vertices;
				break;
			case 34963:
				type = cgltf_buffer_view_type_indices;
				break;
			default:
				type = cgltf_buffer_view_type_invalid;
				break;
			}
			out_buffer_view->type = (cgltf_buffer_view_type)type;
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_buffer_view->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_buffer_views(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_buffer_view), (void**)&out_data->buffer_views, &out_data->buffer_views_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->buffer_views_count; ++j)
	{
		i = cgltf_parse_json_buffer_view(tokens, i, json_chunk, &out_data->buffer_views[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_buffer(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_buffer* out_buffer)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "byteLength") == 0)
		{
			++i;
			out_buffer->size =
					cgltf_json_to_int(tokens+i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "uri") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_buffer->uri);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_buffer->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_buffers(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_buffer), (void**)&out_data->buffers, &out_data->buffers_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->buffers_count; ++j)
	{
		i = cgltf_parse_json_buffer(options, tokens, i, json_chunk, &out_data->buffers[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_skin(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_skin* out_skin)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "name") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_skin->name);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "joints") == 0)
		{
			i = cgltf_parse_json_array(options, tokens, i + 1, json_chunk, sizeof(cgltf_node*), (void**)&out_skin->joints, &out_skin->joints_count);
			if (i < 0)
			{
				return i;
			}

			for (cgltf_size k = 0; k < out_skin->joints_count; ++k)
			{
				out_skin->joints[k] = CGLTF_PTRINDEX(cgltf_node, cgltf_json_to_int(tokens + i, json_chunk));
				++i;
			}
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "skeleton") == 0)
		{
			++i;
			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
			out_skin->skeleton = CGLTF_PTRINDEX(cgltf_node, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "inverseBindMatrices") == 0)
		{
			++i;
			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
			out_skin->inverse_bind_matrices = CGLTF_PTRINDEX(cgltf_accessor, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_skin->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_skins(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_skin), (void**)&out_data->skins, &out_data->skins_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->skins_count; ++j)
	{
		i = cgltf_parse_json_skin(options, tokens, i, json_chunk, &out_data->skins[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_camera(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_camera* out_camera)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "name") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_camera->name);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "type") == 0)
		{
			++i;
			if (cgltf_json_strcmp(tokens + i, json_chunk, "perspective") == 0)
			{
				out_camera->type = cgltf_camera_type_perspective;
			}
			else if (cgltf_json_strcmp(tokens + i, json_chunk, "orthographic") == 0)
			{
				out_camera->type = cgltf_camera_type_orthographic;
			}
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "perspective") == 0)
		{
			++i;

			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

			int data_size = tokens[i].size;
			++i;

			out_camera->type = cgltf_camera_type_perspective;

			for (int k = 0; k < data_size; ++k)
			{
				CGLTF_CHECK_KEY(tokens[i]);

				if (cgltf_json_strcmp(tokens+i, json_chunk, "aspectRatio") == 0)
				{
					++i;
					out_camera->perspective.aspect_ratio = cgltf_json_to_float(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "yfov") == 0)
				{
					++i;
					out_camera->perspective.yfov = cgltf_json_to_float(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "zfar") == 0)
				{
					++i;
					out_camera->perspective.zfar = cgltf_json_to_float(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "znear") == 0)
				{
					++i;
					out_camera->perspective.znear = cgltf_json_to_float(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
				{
					i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_camera->perspective.extras);
				}
				else
				{
					i = cgltf_skip_json(tokens, i+1);
				}

				if (i < 0)
				{
					return i;
				}
			}
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "orthographic") == 0)
		{
			++i;

			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

			int data_size = tokens[i].size;
			++i;

			out_camera->type = cgltf_camera_type_orthographic;

			for (int k = 0; k < data_size; ++k)
			{
				CGLTF_CHECK_KEY(tokens[i]);

				if (cgltf_json_strcmp(tokens+i, json_chunk, "xmag") == 0)
				{
					++i;
					out_camera->orthographic.xmag = cgltf_json_to_float(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "ymag") == 0)
				{
					++i;
					out_camera->orthographic.ymag = cgltf_json_to_float(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "zfar") == 0)
				{
					++i;
					out_camera->orthographic.zfar = cgltf_json_to_float(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "znear") == 0)
				{
					++i;
					out_camera->orthographic.znear = cgltf_json_to_float(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
				{
					i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_camera->orthographic.extras);
				}
				else
				{
					i = cgltf_skip_json(tokens, i+1);
				}

				if (i < 0)
				{
					return i;
				}
			}
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_camera->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_cameras(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_camera), (void**)&out_data->cameras, &out_data->cameras_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->cameras_count; ++j)
	{
		i = cgltf_parse_json_camera(options, tokens, i, json_chunk, &out_data->cameras[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_light(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_light* out_light)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "name") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_light->name);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "color") == 0)
		{
			i = cgltf_parse_json_float_array(tokens, i + 1, json_chunk, out_light->color, 3);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "intensity") == 0)
		{
			++i;
			out_light->intensity = cgltf_json_to_float(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "type") == 0)
		{
			++i;
			if (cgltf_json_strcmp(tokens + i, json_chunk, "directional") == 0)
			{
				out_light->type = cgltf_light_type_directional;
			}
			else if (cgltf_json_strcmp(tokens + i, json_chunk, "point") == 0)
			{
				out_light->type = cgltf_light_type_point;
			}
			else if (cgltf_json_strcmp(tokens + i, json_chunk, "spot") == 0)
			{
				out_light->type = cgltf_light_type_spot;
			}
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "range") == 0)
		{
			++i;
			out_light->range = cgltf_json_to_float(tokens + i, json_chunk);
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "spot") == 0)
		{
			++i;

			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

			int data_size = tokens[i].size;
			++i;

			for (int k = 0; k < data_size; ++k)
			{
				CGLTF_CHECK_KEY(tokens[i]);

				if (cgltf_json_strcmp(tokens+i, json_chunk, "innerConeAngle") == 0)
				{
					++i;
					out_light->spot_inner_cone_angle = cgltf_json_to_float(tokens + i, json_chunk);
					++i;
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "outerConeAngle") == 0)
				{
					++i;
					out_light->spot_outer_cone_angle = cgltf_json_to_float(tokens + i, json_chunk);
					++i;
				}
				else
				{
					i = cgltf_skip_json(tokens, i+1);
				}

				if (i < 0)
				{
					return i;
				}
			}
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_lights(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_light), (void**)&out_data->lights, &out_data->lights_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->lights_count; ++j)
	{
		i = cgltf_parse_json_light(options, tokens, i, json_chunk, &out_data->lights[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_node(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_node* out_node)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	out_node->rotation[3] = 1.0f;
	out_node->scale[0] = 1.0f;
	out_node->scale[1] = 1.0f;
	out_node->scale[2] = 1.0f;
	out_node->matrix[0] = 1.0f;
	out_node->matrix[5] = 1.0f;
	out_node->matrix[10] = 1.0f;
	out_node->matrix[15] = 1.0f;

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "name") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_node->name);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "children") == 0)
		{
			i = cgltf_parse_json_array(options, tokens, i + 1, json_chunk, sizeof(cgltf_node*), (void**)&out_node->children, &out_node->children_count);
			if (i < 0)
			{
				return i;
			}

			for (cgltf_size k = 0; k < out_node->children_count; ++k)
			{
				out_node->children[k] = CGLTF_PTRINDEX(cgltf_node, cgltf_json_to_int(tokens + i, json_chunk));
				++i;
			}
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "mesh") == 0)
		{
			++i;
			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
			out_node->mesh = CGLTF_PTRINDEX(cgltf_mesh, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "skin") == 0)
		{
			++i;
			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
			out_node->skin = CGLTF_PTRINDEX(cgltf_skin, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "camera") == 0)
		{
			++i;
			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
			out_node->camera = CGLTF_PTRINDEX(cgltf_camera, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "translation") == 0)
		{
			out_node->has_translation = 1;
			i = cgltf_parse_json_float_array(tokens, i + 1, json_chunk, out_node->translation, 3);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "rotation") == 0)
		{
			out_node->has_rotation = 1;
			i = cgltf_parse_json_float_array(tokens, i + 1, json_chunk, out_node->rotation, 4);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "scale") == 0)
		{
			out_node->has_scale = 1;
			i = cgltf_parse_json_float_array(tokens, i + 1, json_chunk, out_node->scale, 3);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "matrix") == 0)
		{
			out_node->has_matrix = 1;
			i = cgltf_parse_json_float_array(tokens, i + 1, json_chunk, out_node->matrix, 16);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "weights") == 0)
		{
			i = cgltf_parse_json_array(options, tokens, i + 1, json_chunk, sizeof(cgltf_float), (void**)&out_node->weights, &out_node->weights_count);
			if (i < 0)
			{
				return i;
			}

			i = cgltf_parse_json_float_array(tokens, i - 1, json_chunk, out_node->weights, (int)out_node->weights_count);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_node->extras);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extensions") == 0)
		{
			++i;

			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

			int extensions_size = tokens[i].size;
			++i;

			for (int k = 0; k < extensions_size; ++k)
			{
				CGLTF_CHECK_KEY(tokens[i]);

				if (cgltf_json_strcmp(tokens+i, json_chunk, "KHR_lights_punctual") == 0)
				{
					++i;

					CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

					int data_size = tokens[i].size;
					++i;

					for (int m = 0; m < data_size; ++m)
					{
						CGLTF_CHECK_KEY(tokens[i]);

						if (cgltf_json_strcmp(tokens + i, json_chunk, "light") == 0)
						{
							++i;
							CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
							out_node->light = CGLTF_PTRINDEX(cgltf_light, cgltf_json_to_int(tokens + i, json_chunk));
							++i;
						}
						else
						{
							i = cgltf_skip_json(tokens, i + 1);
						}

						if (i < 0)
						{
							return i;
						}
					}
				}
				else
				{
					i = cgltf_skip_json(tokens, i+1);
				}

				if (i < 0)
				{
					return i;
				}
			}
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_nodes(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_node), (void**)&out_data->nodes, &out_data->nodes_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->nodes_count; ++j)
	{
		i = cgltf_parse_json_node(options, tokens, i, json_chunk, &out_data->nodes[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_scene(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_scene* out_scene)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "name") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_scene->name);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "nodes") == 0)
		{
			i = cgltf_parse_json_array(options, tokens, i + 1, json_chunk, sizeof(cgltf_node*), (void**)&out_scene->nodes, &out_scene->nodes_count);
			if (i < 0)
			{
				return i;
			}

			for (cgltf_size k = 0; k < out_scene->nodes_count; ++k)
			{
				out_scene->nodes[k] = CGLTF_PTRINDEX(cgltf_node, cgltf_json_to_int(tokens + i, json_chunk));
				++i;
			}
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_scene->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_scenes(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_scene), (void**)&out_data->scenes, &out_data->scenes_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->scenes_count; ++j)
	{
		i = cgltf_parse_json_scene(options, tokens, i, json_chunk, &out_data->scenes[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_animation_sampler(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_animation_sampler* out_sampler)
{
	(void)options;
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "input") == 0)
		{
			++i;
			out_sampler->input = CGLTF_PTRINDEX(cgltf_accessor, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "output") == 0)
		{
			++i;
			out_sampler->output = CGLTF_PTRINDEX(cgltf_accessor, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "interpolation") == 0)
		{
			++i;
			if (cgltf_json_strcmp(tokens + i, json_chunk, "LINEAR") == 0)
			{
				out_sampler->interpolation = cgltf_interpolation_type_linear;
			}
			else if (cgltf_json_strcmp(tokens + i, json_chunk, "STEP") == 0)
			{
				out_sampler->interpolation = cgltf_interpolation_type_step;
			}
			else if (cgltf_json_strcmp(tokens + i, json_chunk, "CUBICSPLINE") == 0)
			{
				out_sampler->interpolation = cgltf_interpolation_type_cubic_spline;
			}
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_sampler->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_animation_channel(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_animation_channel* out_channel)
{
	(void)options;
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "sampler") == 0)
		{
			++i;
			out_channel->sampler = CGLTF_PTRINDEX(cgltf_animation_sampler, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "target") == 0)
		{
			++i;

			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

			int target_size = tokens[i].size;
			++i;

			for (int k = 0; k < target_size; ++k)
			{
				CGLTF_CHECK_KEY(tokens[i]);

				if (cgltf_json_strcmp(tokens+i, json_chunk, "node") == 0)
				{
					++i;
					out_channel->target_node = CGLTF_PTRINDEX(cgltf_node, cgltf_json_to_int(tokens + i, json_chunk));
					++i;
				}
				else if (cgltf_json_strcmp(tokens+i, json_chunk, "path") == 0)
				{
					++i;
					if (cgltf_json_strcmp(tokens+i, json_chunk, "translation") == 0)
					{
						out_channel->target_path = cgltf_animation_path_type_translation;
					}
					else if (cgltf_json_strcmp(tokens+i, json_chunk, "rotation") == 0)
					{
						out_channel->target_path = cgltf_animation_path_type_rotation;
					}
					else if (cgltf_json_strcmp(tokens+i, json_chunk, "scale") == 0)
					{
						out_channel->target_path = cgltf_animation_path_type_scale;
					}
					else if (cgltf_json_strcmp(tokens+i, json_chunk, "weights") == 0)
					{
						out_channel->target_path = cgltf_animation_path_type_weights;
					}
					++i;
				}
				else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
				{
					i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_channel->extras);
				}
				else
				{
					i = cgltf_skip_json(tokens, i+1);
				}

				if (i < 0)
				{
					return i;
				}
			}
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_animation(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_animation* out_animation)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "name") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_animation->name);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "samplers") == 0)
		{
			i = cgltf_parse_json_array(options, tokens, i + 1, json_chunk, sizeof(cgltf_animation_sampler), (void**)&out_animation->samplers, &out_animation->samplers_count);
			if (i < 0)
			{
				return i;
			}

			for (cgltf_size k = 0; k < out_animation->samplers_count; ++k)
			{
				i = cgltf_parse_json_animation_sampler(options, tokens, i, json_chunk, &out_animation->samplers[k]);
				if (i < 0)
				{
					return i;
				}
			}
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "channels") == 0)
		{
			i = cgltf_parse_json_array(options, tokens, i + 1, json_chunk, sizeof(cgltf_animation_channel), (void**)&out_animation->channels, &out_animation->channels_count);
			if (i < 0)
			{
				return i;
			}

			for (cgltf_size k = 0; k < out_animation->channels_count; ++k)
			{
				i = cgltf_parse_json_animation_channel(options, tokens, i, json_chunk, &out_animation->channels[k]);
				if (i < 0)
				{
					return i;
				}
			}
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_animation->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static int cgltf_parse_json_animations(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	i = cgltf_parse_json_array(options, tokens, i, json_chunk, sizeof(cgltf_animation), (void**)&out_data->animations, &out_data->animations_count);
	if (i < 0)
	{
		return i;
	}

	for (cgltf_size j = 0; j < out_data->animations_count; ++j)
	{
		i = cgltf_parse_json_animation(options, tokens, i, json_chunk, &out_data->animations[j]);
		if (i < 0)
		{
			return i;
		}
	}
	return i;
}

static int cgltf_parse_json_asset(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_asset* out_asset)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens+i, json_chunk, "copyright") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_asset->copyright);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "generator") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_asset->generator);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "version") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_asset->version);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "minVersion") == 0)
		{
			i = cgltf_parse_json_string(options, tokens, i + 1, json_chunk, &out_asset->min_version);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_asset->extras);
		}
		else
		{
			i = cgltf_skip_json(tokens, i+1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

static cgltf_size cgltf_num_components(cgltf_type type) {
	switch (type)
	{
	case cgltf_type_vec2:
		return 2;
	case cgltf_type_vec3:
		return 3;
	case cgltf_type_vec4:
		return 4;
	case cgltf_type_mat2:
		return 4;
	case cgltf_type_mat3:
		return 9;
	case cgltf_type_mat4:
		return 16;
	case cgltf_type_invalid:
	case cgltf_type_scalar:
	default:
		return 1;
	}
}

static cgltf_size cgltf_component_size(cgltf_component_type component_type) {
	switch (component_type)
	{
	case cgltf_component_type_r_8:
	case cgltf_component_type_r_8u:
		return 1;
	case cgltf_component_type_r_16:
	case cgltf_component_type_r_16u:
		return 2;
	case cgltf_component_type_r_32u:
	case cgltf_component_type_r_32f:
		return 4;
	case cgltf_component_type_invalid:
	default:
		return 0;
	}
}

static cgltf_size cgltf_calc_size(cgltf_type type, cgltf_component_type component_type)
{
	cgltf_size component_size = cgltf_component_size(component_type);
	if (type == cgltf_type_mat2 && component_size == 1)
	{
		return 8 * component_size;
	}
	else if (type == cgltf_type_mat3 && (component_size == 1 || component_size == 2))
	{
		return 12 * component_size;
	}
	return component_size * cgltf_num_components(type);
}

static int cgltf_fixup_pointers(cgltf_data* out_data);

static int cgltf_parse_json_root(cgltf_options* options, jsmntok_t const* tokens, int i, const uint8_t* json_chunk, cgltf_data* out_data)
{
	CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

	int size = tokens[i].size;
	++i;

	for (int j = 0; j < size; ++j)
	{
		CGLTF_CHECK_KEY(tokens[i]);

		if (cgltf_json_strcmp(tokens + i, json_chunk, "asset") == 0)
		{
			i = cgltf_parse_json_asset(options, tokens, i + 1, json_chunk, &out_data->asset);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "meshes") == 0)
		{
			i = cgltf_parse_json_meshes(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "accessors") == 0)
		{
			i = cgltf_parse_json_accessors(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "bufferViews") == 0)
		{
			i = cgltf_parse_json_buffer_views(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "buffers") == 0)
		{
			i = cgltf_parse_json_buffers(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "materials") == 0)
		{
			i = cgltf_parse_json_materials(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "images") == 0)
		{
			i = cgltf_parse_json_images(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "textures") == 0)
		{
			i = cgltf_parse_json_textures(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "samplers") == 0)
		{
			i = cgltf_parse_json_samplers(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "skins") == 0)
		{
			i = cgltf_parse_json_skins(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "cameras") == 0)
		{
			i = cgltf_parse_json_cameras(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "nodes") == 0)
		{
			i = cgltf_parse_json_nodes(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "scenes") == 0)
		{
			i = cgltf_parse_json_scenes(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "scene") == 0)
		{
			++i;
			out_data->scene = CGLTF_PTRINDEX(cgltf_scene, cgltf_json_to_int(tokens + i, json_chunk));
			++i;
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "animations") == 0)
		{
			i = cgltf_parse_json_animations(options, tokens, i + 1, json_chunk, out_data);
		}
		else if (cgltf_json_strcmp(tokens+i, json_chunk, "extras") == 0)
		{
			i = cgltf_parse_json_extras(tokens, i + 1, json_chunk, &out_data->extras);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extensions") == 0)
		{
			++i;

			CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

			int extensions_size = tokens[i].size;
			++i;

			for (int k = 0; k < extensions_size; ++k)
			{
				CGLTF_CHECK_KEY(tokens[i]);

				if (cgltf_json_strcmp(tokens+i, json_chunk, "KHR_lights_punctual") == 0)
				{
					++i;

					CGLTF_CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);

					int data_size = tokens[i].size;
					++i;

					for (int m = 0; m < data_size; ++m)
					{
						CGLTF_CHECK_KEY(tokens[i]);

						if (cgltf_json_strcmp(tokens + i, json_chunk, "lights") == 0)
						{
							i = cgltf_parse_json_lights(options, tokens, i + 1, json_chunk, out_data);
						}
						else
						{
							i = cgltf_skip_json(tokens, i + 1);
						}

						if (i < 0)
						{
							return i;
						}
					}
				}
				else
				{
					i = cgltf_skip_json(tokens, i + 1);
				}

				if (i < 0)
				{
					return i;
				}
			}
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extensionsUsed") == 0)
		{
			i = cgltf_parse_json_string_array(options, tokens, i + 1, json_chunk, &out_data->extensions_used, &out_data->extensions_used_count);
		}
		else if (cgltf_json_strcmp(tokens + i, json_chunk, "extensionsRequired") == 0)
		{
			i = cgltf_parse_json_string_array(options, tokens, i + 1, json_chunk, &out_data->extensions_required, &out_data->extensions_required_count);
		}
		else
		{
			i = cgltf_skip_json(tokens, i + 1);
		}

		if (i < 0)
		{
			return i;
		}
	}

	return i;
}

cgltf_result cgltf_parse_json(cgltf_options* options, const uint8_t* json_chunk, cgltf_size size, cgltf_data** out_data)
{
	jsmn_parser parser = { 0, 0, 0 };

	if (options->json_token_count == 0)
	{
		int token_count = jsmn_parse(&parser, (const char*)json_chunk, size, NULL, 0);

		if (token_count <= 0)
		{
			return cgltf_result_invalid_json;
		}

		options->json_token_count = token_count;
	}

	jsmntok_t* tokens = (jsmntok_t*)options->memory_alloc(options->memory_user_data, sizeof(jsmntok_t) * options->json_token_count);

	if (!tokens)
	{
		return cgltf_result_out_of_memory;
	}

	jsmn_init(&parser);

	int token_count = jsmn_parse(&parser, (const char*)json_chunk, size, tokens, options->json_token_count);

	if (token_count <= 0)
	{
		options->memory_free(options->memory_user_data, tokens);
		return cgltf_result_invalid_json;
	}

	cgltf_data* data = (cgltf_data*)options->memory_alloc(options->memory_user_data, sizeof(cgltf_data));

	if (!data)
	{
		options->memory_free(options->memory_user_data, tokens);
		return cgltf_result_out_of_memory;
	}

	memset(data, 0, sizeof(cgltf_data));
	data->memory_free = options->memory_free;
	data->memory_user_data = options->memory_user_data;

	int i = cgltf_parse_json_root(options, tokens, 0, json_chunk, data);

	options->memory_free(options->memory_user_data, tokens);

	if (i < 0)
	{
		cgltf_free(data);
		return (i == CGLTF_ERROR_NOMEM) ? cgltf_result_out_of_memory : cgltf_result_invalid_gltf;
	}

	if (cgltf_fixup_pointers(data) < 0)
	{
		cgltf_free(data);
		return cgltf_result_invalid_gltf;
	}

	data->json = (const char*)json_chunk;
	data->json_size = size;

	*out_data = data;

	return cgltf_result_success;
}

static int cgltf_fixup_pointers(cgltf_data* data)
{
	for (cgltf_size i = 0; i < data->meshes_count; ++i)
	{
		for (cgltf_size j = 0; j < data->meshes[i].primitives_count; ++j)
		{
			CGLTF_PTRFIXUP(data->meshes[i].primitives[j].indices, data->accessors, data->accessors_count);
			CGLTF_PTRFIXUP(data->meshes[i].primitives[j].material, data->materials, data->materials_count);

			for (cgltf_size k = 0; k < data->meshes[i].primitives[j].attributes_count; ++k)
			{
				CGLTF_PTRFIXUP_REQ(data->meshes[i].primitives[j].attributes[k].data, data->accessors, data->accessors_count);
			}

			for (cgltf_size k = 0; k < data->meshes[i].primitives[j].targets_count; ++k)
			{
				for (cgltf_size m = 0; m < data->meshes[i].primitives[j].targets[k].attributes_count; ++m)
				{
					CGLTF_PTRFIXUP_REQ(data->meshes[i].primitives[j].targets[k].attributes[m].data, data->accessors, data->accessors_count);
				}
			}
		}
	}

	for (cgltf_size i = 0; i < data->accessors_count; ++i)
	{
		CGLTF_PTRFIXUP(data->accessors[i].buffer_view, data->buffer_views, data->buffer_views_count);

		if (data->accessors[i].is_sparse)
		{
			CGLTF_PTRFIXUP_REQ(data->accessors[i].sparse.indices_buffer_view, data->buffer_views, data->buffer_views_count);
			CGLTF_PTRFIXUP_REQ(data->accessors[i].sparse.values_buffer_view, data->buffer_views, data->buffer_views_count);
		}

		if (data->accessors[i].buffer_view)
		{
			data->accessors[i].stride = data->accessors[i].buffer_view->stride;
		}

		if (data->accessors[i].stride == 0)
		{
			data->accessors[i].stride = cgltf_calc_size(data->accessors[i].type, data->accessors[i].component_type);
		}
	}

	for (cgltf_size i = 0; i < data->textures_count; ++i)
	{
		CGLTF_PTRFIXUP(data->textures[i].image, data->images, data->images_count);
		CGLTF_PTRFIXUP(data->textures[i].sampler, data->samplers, data->samplers_count);
	}

	for (cgltf_size i = 0; i < data->images_count; ++i)
	{
		CGLTF_PTRFIXUP(data->images[i].buffer_view, data->buffer_views, data->buffer_views_count);
	}

	for (cgltf_size i = 0; i < data->materials_count; ++i)
	{
		CGLTF_PTRFIXUP(data->materials[i].normal_texture.texture, data->textures, data->textures_count);
		CGLTF_PTRFIXUP(data->materials[i].emissive_texture.texture, data->textures, data->textures_count);
		CGLTF_PTRFIXUP(data->materials[i].occlusion_texture.texture, data->textures, data->textures_count);

		CGLTF_PTRFIXUP(data->materials[i].pbr_metallic_roughness.base_color_texture.texture, data->textures, data->textures_count);
		CGLTF_PTRFIXUP(data->materials[i].pbr_metallic_roughness.metallic_roughness_texture.texture, data->textures, data->textures_count);

		CGLTF_PTRFIXUP(data->materials[i].pbr_specular_glossiness.diffuse_texture.texture, data->textures, data->textures_count);
		CGLTF_PTRFIXUP(data->materials[i].pbr_specular_glossiness.specular_glossiness_texture.texture, data->textures, data->textures_count);
	}

	for (cgltf_size i = 0; i < data->buffer_views_count; ++i)
	{
		CGLTF_PTRFIXUP_REQ(data->buffer_views[i].buffer, data->buffers, data->buffers_count);
	}

	for (cgltf_size i = 0; i < data->skins_count; ++i)
	{
		for (cgltf_size j = 0; j < data->skins[i].joints_count; ++j)
		{
			CGLTF_PTRFIXUP_REQ(data->skins[i].joints[j], data->nodes, data->nodes_count);
		}

		CGLTF_PTRFIXUP(data->skins[i].skeleton, data->nodes, data->nodes_count);
		CGLTF_PTRFIXUP(data->skins[i].inverse_bind_matrices, data->accessors, data->accessors_count);
	}

	for (cgltf_size i = 0; i < data->nodes_count; ++i)
	{
		for (cgltf_size j = 0; j < data->nodes[i].children_count; ++j)
		{
			CGLTF_PTRFIXUP_REQ(data->nodes[i].children[j], data->nodes, data->nodes_count);

			if (data->nodes[i].children[j]->parent)
			{
				return CGLTF_ERROR_JSON;
			}

			data->nodes[i].children[j]->parent = &data->nodes[i];
		}

		CGLTF_PTRFIXUP(data->nodes[i].mesh, data->meshes, data->meshes_count);
		CGLTF_PTRFIXUP(data->nodes[i].skin, data->skins, data->skins_count);
		CGLTF_PTRFIXUP(data->nodes[i].camera, data->cameras, data->cameras_count);
		CGLTF_PTRFIXUP(data->nodes[i].light, data->lights, data->lights_count);
	}

	for (cgltf_size i = 0; i < data->scenes_count; ++i)
	{
		for (cgltf_size j = 0; j < data->scenes[i].nodes_count; ++j)
		{
			CGLTF_PTRFIXUP_REQ(data->scenes[i].nodes[j], data->nodes, data->nodes_count);

			if (data->scenes[i].nodes[j]->parent)
			{
				return CGLTF_ERROR_JSON;
			}
		}
	}

	CGLTF_PTRFIXUP(data->scene, data->scenes, data->scenes_count);

	for (cgltf_size i = 0; i < data->animations_count; ++i)
	{
		for (cgltf_size j = 0; j < data->animations[i].samplers_count; ++j)
		{
			CGLTF_PTRFIXUP_REQ(data->animations[i].samplers[j].input, data->accessors, data->accessors_count);
			CGLTF_PTRFIXUP_REQ(data->animations[i].samplers[j].output, data->accessors, data->accessors_count);
		}

		for (cgltf_size j = 0; j < data->animations[i].channels_count; ++j)
		{
			CGLTF_PTRFIXUP_REQ(data->animations[i].channels[j].sampler, data->animations[i].samplers, data->animations[i].samplers_count);
			CGLTF_PTRFIXUP(data->animations[i].channels[j].target_node, data->nodes, data->nodes_count);
		}
	}

	return 0;
}

/*
 * -- jsmn.c start --
 * Source: https://github.com/zserge/jsmn
 * License: MIT
 *
 * Copyright (c) 2010 Serge A. Zaitsev

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * Allocates a fresh unused token from the token pull.
 */
static jsmntok_t *jsmn_alloc_token(jsmn_parser *parser,
				   jsmntok_t *tokens, size_t num_tokens) {
	jsmntok_t *tok;
	if (parser->toknext >= num_tokens) {
		return NULL;
	}
	tok = &tokens[parser->toknext++];
	tok->start = tok->end = -1;
	tok->size = 0;
#ifdef JSMN_PARENT_LINKS
	tok->parent = -1;
#endif
	return tok;
}

/**
 * Fills token type and boundaries.
 */
static void jsmn_fill_token(jsmntok_t *token, jsmntype_t type,
			    int start, int end) {
	token->type = type;
	token->start = start;
	token->end = end;
	token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
static int jsmn_parse_primitive(jsmn_parser *parser, const char *js,
				size_t len, jsmntok_t *tokens, size_t num_tokens) {
	jsmntok_t *token;
	int start;

	start = parser->pos;

	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		switch (js[parser->pos]) {
#ifndef JSMN_STRICT
		/* In strict mode primitive must be followed by "," or "}" or "]" */
		case ':':
#endif
		case '\t' : case '\r' : case '\n' : case ' ' :
		case ','  : case ']'  : case '}' :
			goto found;
		}
		if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
			parser->pos = start;
			return JSMN_ERROR_INVAL;
		}
	}
#ifdef JSMN_STRICT
	/* In strict mode primitive must be followed by a comma/object/array */
	parser->pos = start;
	return JSMN_ERROR_PART;
#endif

found:
	if (tokens == NULL) {
		parser->pos--;
		return 0;
	}
	token = jsmn_alloc_token(parser, tokens, num_tokens);
	if (token == NULL) {
		parser->pos = start;
		return JSMN_ERROR_NOMEM;
	}
	jsmn_fill_token(token, JSMN_PRIMITIVE, start, parser->pos);
#ifdef JSMN_PARENT_LINKS
	token->parent = parser->toksuper;
#endif
	parser->pos--;
	return 0;
}

/**
 * Fills next token with JSON string.
 */
static int jsmn_parse_string(jsmn_parser *parser, const char *js,
			     size_t len, jsmntok_t *tokens, size_t num_tokens) {
	jsmntok_t *token;

	int start = parser->pos;

	parser->pos++;

	/* Skip starting quote */
	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		char c = js[parser->pos];

		/* Quote: end of string */
		if (c == '\"') {
			if (tokens == NULL) {
				return 0;
			}
			token = jsmn_alloc_token(parser, tokens, num_tokens);
			if (token == NULL) {
				parser->pos = start;
				return JSMN_ERROR_NOMEM;
			}
			jsmn_fill_token(token, JSMN_STRING, start+1, parser->pos);
#ifdef JSMN_PARENT_LINKS
			token->parent = parser->toksuper;
#endif
			return 0;
		}

		/* Backslash: Quoted symbol expected */
		if (c == '\\' && parser->pos + 1 < len) {
			int i;
			parser->pos++;
			switch (js[parser->pos]) {
			/* Allowed escaped symbols */
			case '\"': case '/' : case '\\' : case 'b' :
			case 'f' : case 'r' : case 'n'  : case 't' :
				break;
				/* Allows escaped symbol \uXXXX */
			case 'u':
				parser->pos++;
				for(i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++) {
					/* If it isn't a hex character we have an error */
					if(!((js[parser->pos] >= 48 && js[parser->pos] <= 57) || /* 0-9 */
					     (js[parser->pos] >= 65 && js[parser->pos] <= 70) || /* A-F */
					     (js[parser->pos] >= 97 && js[parser->pos] <= 102))) { /* a-f */
						parser->pos = start;
						return JSMN_ERROR_INVAL;
					}
					parser->pos++;
				}
				parser->pos--;
				break;
				/* Unexpected symbol */
			default:
				parser->pos = start;
				return JSMN_ERROR_INVAL;
			}
		}
	}
	parser->pos = start;
	return JSMN_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
static int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
	       jsmntok_t *tokens, size_t num_tokens) {
	int r;
	int i;
	jsmntok_t *token;
	int count = parser->toknext;

	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		char c;
		jsmntype_t type;

		c = js[parser->pos];
		switch (c) {
		case '{': case '[':
			count++;
			if (tokens == NULL) {
				break;
			}
			token = jsmn_alloc_token(parser, tokens, num_tokens);
			if (token == NULL)
				return JSMN_ERROR_NOMEM;
			if (parser->toksuper != -1) {
				tokens[parser->toksuper].size++;
#ifdef JSMN_PARENT_LINKS
				token->parent = parser->toksuper;
#endif
			}
			token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
			token->start = parser->pos;
			parser->toksuper = parser->toknext - 1;
			break;
		case '}': case ']':
			if (tokens == NULL)
				break;
			type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
#ifdef JSMN_PARENT_LINKS
			if (parser->toknext < 1) {
				return JSMN_ERROR_INVAL;
			}
			token = &tokens[parser->toknext - 1];
			for (;;) {
				if (token->start != -1 && token->end == -1) {
					if (token->type != type) {
						return JSMN_ERROR_INVAL;
					}
					token->end = parser->pos + 1;
					parser->toksuper = token->parent;
					break;
				}
				if (token->parent == -1) {
					if(token->type != type || parser->toksuper == -1) {
						return JSMN_ERROR_INVAL;
					}
					break;
				}
				token = &tokens[token->parent];
			}
#else
			for (i = parser->toknext - 1; i >= 0; i--) {
				token = &tokens[i];
				if (token->start != -1 && token->end == -1) {
					if (token->type != type) {
						return JSMN_ERROR_INVAL;
					}
					parser->toksuper = -1;
					token->end = parser->pos + 1;
					break;
				}
			}
			/* Error if unmatched closing bracket */
			if (i == -1) return JSMN_ERROR_INVAL;
			for (; i >= 0; i--) {
				token = &tokens[i];
				if (token->start != -1 && token->end == -1) {
					parser->toksuper = i;
					break;
				}
			}
#endif
			break;
		case '\"':
			r = jsmn_parse_string(parser, js, len, tokens, num_tokens);
			if (r < 0) return r;
			count++;
			if (parser->toksuper != -1 && tokens != NULL)
				tokens[parser->toksuper].size++;
			break;
		case '\t' : case '\r' : case '\n' : case ' ':
			break;
		case ':':
			parser->toksuper = parser->toknext - 1;
			break;
		case ',':
			if (tokens != NULL && parser->toksuper != -1 &&
					tokens[parser->toksuper].type != JSMN_ARRAY &&
					tokens[parser->toksuper].type != JSMN_OBJECT) {
#ifdef JSMN_PARENT_LINKS
				parser->toksuper = tokens[parser->toksuper].parent;
#else
				for (i = parser->toknext - 1; i >= 0; i--) {
					if (tokens[i].type == JSMN_ARRAY || tokens[i].type == JSMN_OBJECT) {
						if (tokens[i].start != -1 && tokens[i].end == -1) {
							parser->toksuper = i;
							break;
						}
					}
				}
#endif
			}
			break;
#ifdef JSMN_STRICT
			/* In strict mode primitives are: numbers and booleans */
		case '-': case '0': case '1' : case '2': case '3' : case '4':
		case '5': case '6': case '7' : case '8': case '9':
		case 't': case 'f': case 'n' :
			/* And they must not be keys of the object */
			if (tokens != NULL && parser->toksuper != -1) {
				jsmntok_t *t = &tokens[parser->toksuper];
				if (t->type == JSMN_OBJECT ||
						(t->type == JSMN_STRING && t->size != 0)) {
					return JSMN_ERROR_INVAL;
				}
			}
#else
			/* In non-strict mode every unquoted value is a primitive */
		default:
#endif
			r = jsmn_parse_primitive(parser, js, len, tokens, num_tokens);
			if (r < 0) return r;
			count++;
			if (parser->toksuper != -1 && tokens != NULL)
				tokens[parser->toksuper].size++;
			break;

#ifdef JSMN_STRICT
			/* Unexpected char in strict mode */
		default:
			return JSMN_ERROR_INVAL;
#endif
		}
	}

	if (tokens != NULL) {
		for (i = parser->toknext - 1; i >= 0; i--) {
			/* Unmatched opened object or array */
			if (tokens[i].start != -1 && tokens[i].end == -1) {
				return JSMN_ERROR_PART;
			}
		}
	}

	return count;
}

/**
 * Creates a new parser based over a given  buffer with an array of tokens
 * available.
 */
static void jsmn_init(jsmn_parser *parser) {
	parser->pos = 0;
	parser->toknext = 0;
	parser->toksuper = -1;
}
/*
 * -- jsmn.c end --
 */

#endif /* #ifdef CGLTF_IMPLEMENTATION */

/* cgltf is distributed under MIT license:
 *
 * Copyright (c) 2018 Johannes Kuhlmann

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
