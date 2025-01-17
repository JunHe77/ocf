/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ocf_env.h"
#include <sched.h>
#include <execinfo.h>

struct _env_allocator {
	/*!< Memory pool ID unique name */
	char *name;

	/*!< Size of specific item of memory pool */
	uint32_t item_size;

	/*!< Number of currently allocated items in pool */
	env_atomic count;
};

static inline size_t env_allocator_align(size_t size)
{
	if (size <= 2)
		return size;
	return (1ULL << 32) >> __builtin_clz(size - 1);
}

struct _env_allocator_item {
	uint32_t flags;
	uint32_t cpu;
	char data[];
};

void *env_allocator_new(env_allocator *allocator)
{
	struct _env_allocator_item *item = NULL;

	item = calloc(1, allocator->item_size);

	if (item) {
		item->cpu = 0;
		env_atomic_inc(&allocator->count);
	}

	return &item->data;
}


env_allocator *env_allocator_create(uint32_t size, const char *fmt_name, ...)
{
	char name[OCF_ALLOCATOR_NAME_MAX] = { '\0' };
	int result, error = -1;
	va_list args;

	env_allocator *allocator = calloc(1, sizeof(*allocator));
	if (!allocator) {
		error = __LINE__;
		goto err;
	}

	allocator->item_size = size + sizeof(struct _env_allocator_item);

	/* Format allocator name */
	va_start(args, fmt_name);
	result = vsnprintf(name, sizeof(name), fmt_name, args);
	va_end(args);

	if ((result > 0) && (result < sizeof(name))) {
		allocator->name = strdup(name);

		if (!allocator->name) {
			error = __LINE__;
			goto err;
		}
	} else {
		/* Formated string name exceed max allowed size of name */
		error = __LINE__;
		goto err;
	}

	return allocator;

err:
	printf("Cannot create memory allocator, ERROR %d", error);
	env_allocator_destroy(allocator);

	return NULL;
}

void env_allocator_del(env_allocator *allocator, void *obj)
{
	struct _env_allocator_item *item =
		container_of(obj, struct _env_allocator_item, data);

	env_atomic_dec(&allocator->count);

	free(item);
}

void env_allocator_destroy(env_allocator *allocator)
{
	if (allocator) {
		if (env_atomic_read(&allocator->count)) {
			printf("Not all objects deallocated\n");
			ENV_WARN(true, OCF_PREFIX_SHORT" Cleanup problem\n");
		}

		free(allocator->name);
		free(allocator);
	}
}

/* *** DEBUGING *** */

#define ENV_TRACE_DEPTH	16

void env_stack_trace(void)
{
	void *trace[ENV_TRACE_DEPTH];
	char **messages = NULL;
	int i, size;

	size = backtrace(trace, ENV_TRACE_DEPTH);
	messages = backtrace_symbols(trace, size);
	printf("[stack trace]>>>\n");
	for (i = 0; i < size; ++i)
		printf("%s\n", messages[i]);
	printf("<<<[stack trace]\n");
	free(messages);
}

/* *** CRC *** */

uint32_t env_crc32(uint32_t crc, uint8_t const *data, size_t len)
{
	return crc32(crc, data, len);
}
