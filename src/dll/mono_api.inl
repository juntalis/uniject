/**
 * Included multiple times with different implementations of MONO_API.
 */

#ifndef MONO_API
#error MONO_API must be defined prior to including mono_api.inl!
#endif

MONO_API(void, mono_trace_set_level_string, const char*level)
MONO_API(void, mono_trace_set_mask_string, const char *mask)

MONO_API(void, mono_set_commandline_arguments, int, const char* argv[], const char*)
MONO_API(void, mono_jit_parse_options, int argc, char* argv[])
MONO_API(void, mono_debug_init, int format)

MONO_API(MonoDomain*, mono_get_root_domain, void)
MONO_API(MonoThread*, mono_thread_attach, MonoDomain*)
MONO_API(MonoThread*, mono_thread_current, void)
MONO_API(void, mono_thread_detach, MonoThread*)

MONO_API(MonoAssembly*, mono_domain_assembly_open, MonoDomain*, const char*)
MONO_API(MonoImage*, mono_assembly_get_image, MonoAssembly*)

MONO_API(MonoMethodDesc*, mono_method_desc_new, const char*, gboolean)
MONO_API(MonoMethod*, mono_method_desc_search_in_image, MonoMethodDesc*, MonoImage*)
MONO_API(void, mono_method_desc_free, MonoMethodDesc*)

MONO_API(MonoObject*, mono_runtime_invoke, MonoMethod*, void*, void**, MonoObject**)

/** Save us some lines in the including file */
#undef MONO_API
