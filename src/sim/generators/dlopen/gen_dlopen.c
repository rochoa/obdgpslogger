/** \file
 \brief Generate dlopen data
*/

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#include "simdl_datasource.h"
#include "datasource.h"

/// The void * generator for dlopen
struct dlopen_gen {
	void *handle; //< Returned by dlopen
	void *gen_gen; //< the void * from the dlopen'd generator

// The generator functions
 	const char *(*simdl_name)();
	int (*simdl_create)(void **gen, const char *seed); //< Create the generator
	void (*simdl_destroy)(void *gen); //< Destroy the generator
	int (*simdl_getvalue)(void *gen, unsigned int PID,
	        unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D);
};

const char *dlopen_simgen_name() {
	return "dlopen";
}

int dlopen_simgen_create(void **gen, const char *seed) {
	void *handle = dlopen(seed, RTLD_NOW);
	if(NULL == handle) {
		fprintf(stderr, "Error dlopening %s: %s\n", seed, dlerror());
		return 1;
	}

	struct dlopen_gen *g = (struct dlopen_gen *)malloc(sizeof(struct dlopen_gen));
	if(NULL == g) {
		fprintf(stderr, "Couldn't allocate memory for dlopen'd generator\n");
		dlclose(handle);
		return 1;
	}

	g->handle = handle;
	dlerror(); // Clear any errors

	char *error;
	// Read the dlopen manpage if you wish to know why
	*(void **)(&g->simdl_name) = dlsym(handle, "simdl_name");
	if(NULL != (error = dlerror())) {
		fprintf(stderr, "Couldn't find symbol simdl_name in library: %s\n", error);
		dlclose(handle);
		free(g);
		return 1;
	}

	*(void **)(&g->simdl_create) = dlsym(handle, "simdl_create");
	if(NULL != (error = dlerror())) {
		fprintf(stderr, "Couldn't find symbol simdl_create in library: %s\n", error);
		dlclose(handle);
		free(g);
		return 1;
	}

	*(void **)(&g->simdl_destroy) = dlsym(handle, "simdl_destroy");
	if(NULL != (error = dlerror())) {
		fprintf(stderr, "Couldn't find symbol simdl_destroy in library: %s\n", error);
		dlclose(handle);
		free(g);
		return 1;
	}

	*(void **)(&g->simdl_getvalue) = dlsym(handle, "simdl_getvalue");
	if(NULL != (error = dlerror())) {
		fprintf(stderr, "Couldn't find symbol simdl_getvalue in library: %s\n", error);
		dlclose(handle);
		free(g);
		return 1;
	}

	printf("Successfully dlopen'd sim generator %s, %s\n", seed, g->simdl_name());

	// TODO: Do something useful with this seed
	if(1 == g->simdl_create(&(g->gen_gen), "")) {
		fprintf(stderr, "dlopene'd library reported error on creation\n");
		dlclose(handle);
		free(g);
		return 1;
	}

	*gen = g;
	return 0;
}

void dlopen_simgen_destroy(void *gen) {
	struct dlopen_gen *g = (struct dlopen_gen *)gen;
	g->simdl_destroy(g->gen_gen);
	dlclose(g->handle);
	free(g);
}

int dlopen_simgen_getvalue(void *gen, unsigned int PID, unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D) {
	struct dlopen_gen *g = (struct dlopen_gen *)gen;
	return g->simdl_getvalue(g->gen_gen, PID, A, B, C, D);
}

// Declare our obdsim_generator. This is pulled in as an extern in obdsim.c
struct obdsim_generator obdsimgen_dlopen = {
	dlopen_simgen_name,
	dlopen_simgen_create,
	dlopen_simgen_destroy,
	dlopen_simgen_getvalue
};

