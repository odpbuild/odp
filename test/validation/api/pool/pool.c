/* Copyright (c) 2014-2018, Linaro Limited
 * Copyright (c) 2020, Marvell
 * Copyright (c) 2020, Nokia
 * All rights reserved.
 *
 * SPDX-License-Identifier:	BSD-3-Clause
 */

#include <odp_api.h>
#include "odp_cunit_common.h"

#define BUF_SIZE 1500
#define BUF_NUM  1000
#define TMO_NUM  1000
#define VEC_NUM  1000
#define VEC_LEN  32
#define PKT_LEN  400
#define PKT_NUM  500
#define MAX_NUM_DEFAULT (10 * 1024 * 1024)

typedef struct {
	odp_barrier_t init_barrier;
	odp_atomic_u32_t index;
	uint32_t nb_threads;
	odp_pool_t pool;
} global_shared_mem_t;

static global_shared_mem_t *global_mem;

static odp_pool_capability_t global_pool_capa;
static odp_pool_param_t default_pool_param;

static void pool_create_destroy(odp_pool_param_t *param)
{
	odp_pool_t pool;

	pool = odp_pool_create(NULL, param);
	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);
	CU_ASSERT(odp_pool_to_u64(pool) !=
		  odp_pool_to_u64(ODP_POOL_INVALID));
	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void pool_test_create_destroy_buffer(void)
{
	odp_pool_param_t param;

	odp_pool_param_init(&param);

	param.type      = ODP_POOL_BUFFER;
	param.buf.size  = BUF_SIZE;
	param.buf.num   = BUF_NUM;

	pool_create_destroy(&param);
}

static void pool_test_create_destroy_packet(void)
{
	odp_pool_param_t param;

	odp_pool_param_init(&param);

	param.type    = ODP_POOL_PACKET;
	param.pkt.len = PKT_LEN;
	param.pkt.num = PKT_NUM;

	pool_create_destroy(&param);
}

static void pool_test_create_destroy_timeout(void)
{
	odp_pool_param_t param;

	odp_pool_param_init(&param);

	param.type    = ODP_POOL_TIMEOUT;
	param.tmo.num = TMO_NUM;

	pool_create_destroy(&param);
}

static void pool_test_create_destroy_vector(void)
{
	odp_pool_param_t param;
	odp_pool_capability_t capa;
	uint32_t max_num = VEC_NUM;

	CU_ASSERT_FATAL(odp_pool_capability(&capa) == 0);

	CU_ASSERT_FATAL(capa.vector.max_pools > 0);

	if (capa.vector.max_num && capa.vector.max_num < max_num)
		max_num = capa.vector.max_num;

	odp_pool_param_init(&param);
	param.type = ODP_POOL_VECTOR;
	param.vector.num = max_num;
	param.vector.max_size = capa.vector.max_size  < VEC_LEN ? capa.vector.max_size : VEC_LEN;

	pool_create_destroy(&param);
}

static void pool_test_lookup_info_print(void)
{
	odp_pool_t pool;
	const char pool_name[] = "pool_for_lookup_test";
	odp_pool_info_t info;
	odp_pool_param_t param;

	memset(&info, 0, sizeof(info));
	odp_pool_param_init(&param);

	param.type      = ODP_POOL_BUFFER;
	param.buf.size  = BUF_SIZE;
	param.buf.num   = BUF_NUM;

	pool = odp_pool_create(pool_name, &param);
	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	pool = odp_pool_lookup(pool_name);
	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	CU_ASSERT_FATAL(odp_pool_info(pool, &info) == 0);
	CU_ASSERT(strncmp(pool_name, info.name, sizeof(pool_name)) == 0);
	CU_ASSERT(param.buf.size <= info.params.buf.size);
	CU_ASSERT(param.buf.align <= info.params.buf.align);
	CU_ASSERT(param.buf.num <= info.params.buf.num);
	CU_ASSERT(param.type == info.params.type);

	odp_pool_print(pool);

	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void alloc_buffer(uint32_t cache_size)
{
	odp_pool_t pool;
	odp_pool_param_t param;
	uint32_t i, num;
	odp_buffer_t buf[BUF_NUM];

	odp_pool_param_init(&param);

	param.type     = ODP_POOL_BUFFER;
	param.buf.num  = BUF_NUM;
	param.buf.size = BUF_SIZE;
	param.pkt.cache_size = cache_size;

	pool = odp_pool_create(NULL, &param);

	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	num = 0;

	for (i = 0; i < PKT_NUM; i++) {
		buf[num] = odp_buffer_alloc(pool);
		CU_ASSERT(buf[num] != ODP_BUFFER_INVALID);

		if (buf[num] != ODP_BUFFER_INVALID)
			num++;
	}

	for (i = 0; i < num; i++)
		odp_buffer_free(buf[i]);

	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void pool_test_alloc_buffer(void)
{
	alloc_buffer(default_pool_param.buf.cache_size);
}

static void pool_test_alloc_buffer_min_cache(void)
{
	alloc_buffer(global_pool_capa.buf.min_cache_size);
}

static void pool_test_alloc_buffer_max_cache(void)
{
	alloc_buffer(global_pool_capa.buf.max_cache_size);
}

static void alloc_packet_vector(uint32_t cache_size)
{
	odp_pool_t pool;
	odp_pool_param_t param;
	odp_pool_capability_t capa;
	uint32_t i, num;
	odp_packet_vector_t pkt_vec[VEC_NUM];
	uint32_t max_num = VEC_NUM;

	CU_ASSERT_FATAL(odp_pool_capability(&capa) == 0);

	if (capa.vector.max_num && capa.vector.max_num < max_num)
		max_num = capa.vector.max_num;

	odp_pool_param_init(&param);
	param.type = ODP_POOL_VECTOR;
	param.vector.num = max_num;
	param.vector.max_size = capa.vector.max_size  < VEC_LEN ? capa.vector.max_size : VEC_LEN;
	param.vector.cache_size = cache_size;

	pool = odp_pool_create(NULL, &param);
	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	num = 0;
	for (i = 0; i < max_num; i++) {
		pkt_vec[num] = odp_packet_vector_alloc(pool);
		CU_ASSERT(pkt_vec[num] != ODP_PACKET_VECTOR_INVALID);

		if (pkt_vec[num] != ODP_PACKET_VECTOR_INVALID)
			num++;
	}

	for (i = 0; i < num; i++)
		odp_packet_vector_free(pkt_vec[i]);

	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void pool_test_alloc_packet_vector(void)
{
	alloc_packet_vector(default_pool_param.vector.cache_size);
}

static void pool_test_alloc_packet_vector_min_cache(void)
{
	alloc_packet_vector(global_pool_capa.vector.min_cache_size);
}

static void pool_test_alloc_packet_vector_max_cache(void)
{
	alloc_packet_vector(global_pool_capa.vector.max_cache_size);
}

static void alloc_packet(uint32_t cache_size)
{
	odp_pool_t pool;
	odp_pool_param_t param;
	uint32_t i, num;
	odp_packet_t pkt[PKT_NUM];

	odp_pool_param_init(&param);

	param.type    = ODP_POOL_PACKET;
	param.pkt.num = PKT_NUM;
	param.pkt.len = PKT_LEN;
	param.pkt.cache_size = cache_size;

	pool = odp_pool_create(NULL, &param);

	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	num = 0;

	for (i = 0; i < PKT_NUM; i++) {
		pkt[num] = odp_packet_alloc(pool, PKT_LEN);
		CU_ASSERT(pkt[num] != ODP_PACKET_INVALID);

		if (pkt[num] != ODP_PACKET_INVALID)
			num++;
	}

	for (i = 0; i < num; i++)
		odp_packet_free(pkt[i]);

	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void pool_test_alloc_packet(void)
{
	alloc_packet(default_pool_param.pkt.cache_size);
}

static void pool_test_alloc_packet_min_cache(void)
{
	alloc_packet(global_pool_capa.pkt.min_cache_size);
}

static void pool_test_alloc_packet_max_cache(void)
{
	alloc_packet(global_pool_capa.pkt.max_cache_size);
}

static void pool_test_alloc_packet_subparam(void)
{
	odp_pool_t pool;
	odp_pool_capability_t capa;
	odp_pool_param_t param;
	uint32_t i, j, num, num_sub;
	odp_packet_t pkt[PKT_NUM];

	CU_ASSERT_FATAL(odp_pool_capability(&capa) == 0);
	num_sub = capa.pkt.max_num_subparam;

	CU_ASSERT_FATAL(num_sub <= ODP_POOL_MAX_SUBPARAMS);

	odp_pool_param_init(&param);

	param.type             = ODP_POOL_PACKET;
	param.pkt.num          = PKT_NUM;
	param.pkt.len          = PKT_LEN;
	param.pkt.num_subparam = num_sub;

	for (i = 0; i < num_sub; i++) {
		param.pkt.sub[i].num = PKT_NUM;
		param.pkt.sub[i].len = PKT_LEN + (i * 100);
	}

	pool = odp_pool_create(NULL, &param);

	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	num = 0;

	for (i = 0; i < PKT_NUM; i++) {
		pkt[num] = odp_packet_alloc(pool, PKT_LEN);
		CU_ASSERT(pkt[num] != ODP_PACKET_INVALID);

		if (pkt[num] != ODP_PACKET_INVALID)
			num++;
	}

	for (i = 0; i < num; i++)
		odp_packet_free(pkt[i]);

	for (j = 0; j < num_sub; j++) {
		num = 0;

		for (i = 0; i < param.pkt.sub[j].num; i++) {
			pkt[num] = odp_packet_alloc(pool, param.pkt.sub[j].len);
			CU_ASSERT(pkt[num] != ODP_PACKET_INVALID);

			if (pkt[num] != ODP_PACKET_INVALID)
				num++;
		}

		for (i = 0; i < num; i++)
			odp_packet_free(pkt[i]);
	}

	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void alloc_timeout(uint32_t cache_size)
{
	odp_pool_t pool;
	odp_pool_param_t param;
	uint32_t i, num;
	odp_timeout_t tmo[TMO_NUM];

	odp_pool_param_init(&param);

	param.type     = ODP_POOL_TIMEOUT;
	param.tmo.num  = TMO_NUM;
	param.tmo.cache_size = cache_size;

	pool = odp_pool_create(NULL, &param);

	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	num = 0;

	for (i = 0; i < PKT_NUM; i++) {
		tmo[num] = odp_timeout_alloc(pool);
		CU_ASSERT(tmo[num] != ODP_TIMEOUT_INVALID);

		if (tmo[num] != ODP_TIMEOUT_INVALID)
			num++;
	}

	for (i = 0; i < num; i++)
		odp_timeout_free(tmo[i]);

	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void pool_test_alloc_timeout(void)
{
	alloc_timeout(default_pool_param.tmo.cache_size);
}

static void pool_test_alloc_timeout_min_cache(void)
{
	alloc_timeout(global_pool_capa.tmo.min_cache_size);
}

static void pool_test_alloc_timeout_max_cache(void)
{
	alloc_timeout(global_pool_capa.tmo.max_cache_size);
}

static void pool_test_info_packet(void)
{
	odp_pool_t pool;
	odp_pool_info_t info;
	odp_pool_param_t param;
	const char pool_name[] = "test_pool_name";

	memset(&info, 0, sizeof(info));
	odp_pool_param_init(&param);

	param.type     = ODP_POOL_PACKET;
	param.pkt.num  = PKT_NUM;
	param.pkt.len  = PKT_LEN;

	pool = odp_pool_create(pool_name, &param);
	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	memset(&info, 0, sizeof(odp_pool_info_t));
	CU_ASSERT_FATAL(odp_pool_info(pool, &info) == 0);

	CU_ASSERT(strncmp(pool_name, info.name, sizeof(pool_name)) == 0);
	CU_ASSERT(info.params.type    == ODP_POOL_PACKET);
	CU_ASSERT(info.params.pkt.num == param.pkt.num);
	CU_ASSERT(info.params.pkt.len == param.pkt.len);
	CU_ASSERT(info.pkt.max_num    >= param.pkt.num);

	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void pool_test_info_data_range(void)
{
	odp_pool_t pool;
	odp_pool_info_t info;
	odp_pool_param_t param;
	odp_packet_t pkt[PKT_NUM];
	uint32_t i, num;
	uintptr_t pool_len;

	memset(&info, 0, sizeof(info));
	odp_pool_param_init(&param);

	param.type     = ODP_POOL_PACKET;
	param.pkt.num  = PKT_NUM;
	param.pkt.len  = PKT_LEN;

	pool = odp_pool_create(NULL, &param);
	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	CU_ASSERT_FATAL(odp_pool_info(pool, &info) == 0);

	pool_len = info.max_data_addr - info.min_data_addr + 1;
	CU_ASSERT(pool_len >= PKT_NUM * PKT_LEN);

	num = 0;

	for (i = 0; i < PKT_NUM; i++) {
		pkt[num] = odp_packet_alloc(pool, PKT_LEN);
		CU_ASSERT(pkt[num] != ODP_PACKET_INVALID);

		if (pkt[num] != ODP_PACKET_INVALID)
			num++;
	}

	for (i = 0; i < num; i++) {
		uintptr_t pkt_data, pkt_data_end;
		uint32_t offset = 0;
		uint32_t seg_len = 0;
		uint32_t pkt_len = odp_packet_len(pkt[i]);

		while (offset < pkt_len) {
			pkt_data = (uintptr_t)odp_packet_offset(pkt[i], offset,
								&seg_len, NULL);
			pkt_data_end = pkt_data + seg_len - 1;
			CU_ASSERT((pkt_data >= info.min_data_addr) &&
				  (pkt_data_end <= info.max_data_addr));
			offset += seg_len;
		}

		odp_packet_free(pkt[i]);
	}

	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void pool_test_buf_max_num(void)
{
	odp_pool_t pool;
	odp_pool_param_t param;
	odp_pool_capability_t capa;
	uint32_t max_num, num, i;
	odp_shm_t shm;
	odp_buffer_t *buf;

	CU_ASSERT_FATAL(odp_pool_capability(&capa) == 0);

	max_num = MAX_NUM_DEFAULT;
	if (capa.buf.max_num)
		max_num = capa.buf.max_num;

	odp_pool_param_init(&param);

	param.type     = ODP_POOL_BUFFER;
	param.buf.num  = max_num;
	param.buf.size = 10;

	pool = odp_pool_create("test_buf_max_num", &param);

	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	shm = odp_shm_reserve("test_max_num_shm",
			      max_num * sizeof(odp_buffer_t),
			      sizeof(odp_buffer_t), 0);

	CU_ASSERT_FATAL(shm != ODP_SHM_INVALID);

	buf = odp_shm_addr(shm);

	num = 0;
	for (i = 0; i < max_num; i++) {
		buf[num] = odp_buffer_alloc(pool);

		if (buf[num] != ODP_BUFFER_INVALID)
			num++;
	}

	CU_ASSERT(num == max_num);

	for (i = 0; i < num; i++)
		odp_buffer_free(buf[i]);

	CU_ASSERT(odp_shm_free(shm) == 0);
	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void pool_test_pkt_max_num(void)
{
	odp_pool_t pool;
	odp_pool_param_t param;
	odp_pool_capability_t capa;
	uint32_t max_num, num, i;
	odp_shm_t shm;
	odp_packet_t *pkt;
	uint32_t len = 10;

	CU_ASSERT_FATAL(odp_pool_capability(&capa) == 0);

	max_num = MAX_NUM_DEFAULT;
	if (capa.pkt.max_num)
		max_num = capa.pkt.max_num;

	odp_pool_param_init(&param);

	param.type         = ODP_POOL_PACKET;
	param.pkt.num      = max_num;
	param.pkt.max_num  = max_num;
	param.pkt.len      = len;
	param.pkt.max_len  = len;
	param.pkt.headroom = 0;

	pool = odp_pool_create("test_packet_max_num", &param);

	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	shm = odp_shm_reserve("test_max_num_shm",
			      max_num * sizeof(odp_packet_t),
			      sizeof(odp_packet_t), 0);

	CU_ASSERT_FATAL(shm != ODP_SHM_INVALID);

	pkt = odp_shm_addr(shm);

	num = 0;
	for (i = 0; i < max_num; i++) {
		pkt[num] = odp_packet_alloc(pool, len);

		if (pkt[num] != ODP_PACKET_INVALID)
			num++;
	}

	CU_ASSERT(num == max_num);

	for (i = 0; i < num; i++)
		odp_packet_free(pkt[i]);

	CU_ASSERT(odp_shm_free(shm) == 0);
	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void pool_test_packet_vector_max_num(void)
{
	odp_pool_t pool;
	odp_pool_param_t param;
	odp_pool_capability_t capa;
	uint32_t num, i;
	odp_shm_t shm;
	odp_packet_vector_t *pktv;
	uint32_t max_num = VEC_NUM;

	CU_ASSERT_FATAL(odp_pool_capability(&capa) == 0);

	if (capa.vector.max_num)
		max_num = capa.vector.max_num;

	odp_pool_param_init(&param);

	param.type = ODP_POOL_VECTOR;
	param.vector.num = max_num;
	param.vector.max_size = 1;

	pool = odp_pool_create("test_packet_vector_max_num", &param);
	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	shm = odp_shm_reserve("test_max_num_shm", max_num * sizeof(odp_packet_vector_t),
			      sizeof(odp_packet_vector_t), 0);
	CU_ASSERT_FATAL(shm != ODP_SHM_INVALID);

	pktv = odp_shm_addr(shm);
	CU_ASSERT_FATAL(pktv != NULL);

	num = 0;
	for (i = 0; i < max_num; i++) {
		pktv[num] = odp_packet_vector_alloc(pool);

		if (pktv[num] != ODP_PACKET_VECTOR_INVALID)
			num++;
	}

	CU_ASSERT(num == max_num);

	for (i = 0; i < num; i++)
		odp_packet_vector_free(pktv[i]);

	CU_ASSERT(odp_shm_free(shm) == 0);
	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void pool_test_pkt_seg_len(void)
{
	uint32_t len = 1500;
	uint32_t min_seg_len = 42;
	uint32_t max_num = 10;
	uint32_t num = 0;
	uint32_t i;
	odp_packet_t pkt_tbl[max_num];
	odp_pool_t pool;
	odp_pool_param_t param;

	odp_pool_param_init(&param);

	param.type         = ODP_POOL_PACKET;
	param.pkt.num      = max_num;
	param.pkt.len      = len;
	param.pkt.max_len  = len;
	param.pkt.seg_len =  min_seg_len;

	pool = odp_pool_create("test_packet_seg_len", &param);
	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	for (i = 0; i < max_num; i++) {
		pkt_tbl[i] = odp_packet_alloc(pool, len);

		if (pkt_tbl[i] != ODP_PACKET_INVALID)
			num++;
	}

	CU_ASSERT(num == max_num);

	for (i = 0; i < num; i++) {
		CU_ASSERT(odp_packet_seg_len(pkt_tbl[i]) >= min_seg_len);
		odp_packet_free(pkt_tbl[i]);
	}

	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void pool_test_tmo_max_num(void)
{
	odp_pool_t pool;
	odp_pool_param_t param;
	odp_pool_capability_t capa;
	uint32_t max_num, num, i;
	odp_shm_t shm;
	odp_timeout_t *tmo;

	CU_ASSERT_FATAL(odp_pool_capability(&capa) == 0);

	max_num = MAX_NUM_DEFAULT;
	if (capa.tmo.max_num)
		max_num = capa.tmo.max_num;

	odp_pool_param_init(&param);

	param.type    = ODP_POOL_TIMEOUT;
	param.tmo.num = max_num;

	pool = odp_pool_create("test_tmo_max_num", &param);

	CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);

	shm = odp_shm_reserve("test_max_num_shm",
			      max_num * sizeof(odp_packet_t),
			      sizeof(odp_packet_t), 0);

	CU_ASSERT_FATAL(shm != ODP_SHM_INVALID);

	tmo = odp_shm_addr(shm);

	num = 0;
	for (i = 0; i < max_num; i++) {
		tmo[num] = odp_timeout_alloc(pool);

		if (tmo[num] != ODP_TIMEOUT_INVALID)
			num++;
	}

	CU_ASSERT(num == max_num);

	for (i = 0; i < num; i++)
		odp_timeout_free(tmo[i]);

	CU_ASSERT(odp_shm_free(shm) == 0);
	CU_ASSERT(odp_pool_destroy(pool) == 0);
}

static void buffer_alloc_loop(odp_pool_t pool, int num, int buffer_size)
{
	int allocs;

	/* Allocate, modify, and free buffers */
	for (allocs = 0; allocs < num;) {
		odp_buffer_t buf;
		uint8_t *data;
		int i;

		buf = odp_buffer_alloc(pool);
		if (buf == ODP_BUFFER_INVALID)
			continue;

		data = odp_buffer_addr(buf);

		for (i = 0; i < buffer_size; i++)
			data[i] = i;

		odp_buffer_free(buf);
		allocs++;
	}
}

static int run_pool_test_create_after_fork(void *arg ODP_UNUSED)
{
	int thr_index;

	thr_index = odp_atomic_fetch_inc_u32(&global_mem->index);

	/* Thread 0 allocates the shared pool */
	if (thr_index == 0) {
		odp_pool_t pool;
		odp_pool_param_t param;

		odp_pool_param_init(&param);

		param.type      = ODP_POOL_BUFFER;
		param.buf.size  = BUF_SIZE;
		param.buf.num   = BUF_NUM;

		pool = odp_pool_create(NULL, &param);
		CU_ASSERT_FATAL(pool != ODP_POOL_INVALID);
		global_mem->pool = pool;
	}

	odp_barrier_wait(&global_mem->init_barrier);

	buffer_alloc_loop(global_mem->pool, BUF_NUM, BUF_SIZE);

	return CU_get_number_of_failures();
}

static void pool_test_create_after_fork(void)
{
	odp_shm_t shm;
	odp_cpumask_t unused;
	pthrd_arg thrdarg;

	/* No single VA required since reserve is done before fork */
	shm = odp_shm_reserve(NULL, sizeof(global_shared_mem_t), 0, 0);
	CU_ASSERT_FATAL(shm != ODP_SHM_INVALID);
	global_mem = odp_shm_addr(shm);
	CU_ASSERT_PTR_NOT_NULL_FATAL(global_mem);

	thrdarg.numthrds = odp_cpumask_default_worker(&unused, 0);
	if (thrdarg.numthrds > MAX_WORKERS)
		thrdarg.numthrds = MAX_WORKERS;

	global_mem->nb_threads = thrdarg.numthrds;
	global_mem->pool = ODP_POOL_INVALID;
	odp_barrier_init(&global_mem->init_barrier, thrdarg.numthrds + 1);
	odp_atomic_init_u32(&global_mem->index, 0);

	/* Fork here */
	odp_cunit_thread_create(run_pool_test_create_after_fork, &thrdarg);

	/* Wait until thread 0 has created the test pool */
	odp_barrier_wait(&global_mem->init_barrier);

	buffer_alloc_loop(global_mem->pool, BUF_NUM, BUF_SIZE);

	/* Wait for all thread endings */
	CU_ASSERT(odp_cunit_thread_exit(&thrdarg) >= 0);

	CU_ASSERT(!odp_pool_destroy(global_mem->pool));

	CU_ASSERT(!odp_shm_free(shm));
}

static int pool_suite_init(void)
{
	memset(&global_pool_capa, 0, sizeof(odp_pool_capability_t));
	memset(&default_pool_param, 0, sizeof(odp_pool_param_t));

	if (odp_pool_capability(&global_pool_capa) < 0) {
		printf("pool_capability failed in suite init\n");
		return -1;
	}

	odp_pool_param_init(&default_pool_param);

	return 0;
}

odp_testinfo_t pool_suite[] = {
	ODP_TEST_INFO(pool_test_create_destroy_buffer),
	ODP_TEST_INFO(pool_test_create_destroy_packet),
	ODP_TEST_INFO(pool_test_create_destroy_timeout),
	ODP_TEST_INFO(pool_test_create_destroy_vector),
	ODP_TEST_INFO(pool_test_alloc_buffer),
	ODP_TEST_INFO(pool_test_alloc_buffer_min_cache),
	ODP_TEST_INFO(pool_test_alloc_buffer_max_cache),
	ODP_TEST_INFO(pool_test_alloc_packet_vector),
	ODP_TEST_INFO(pool_test_alloc_packet_vector_min_cache),
	ODP_TEST_INFO(pool_test_alloc_packet_vector_max_cache),
	ODP_TEST_INFO(pool_test_alloc_packet),
	ODP_TEST_INFO(pool_test_alloc_packet_min_cache),
	ODP_TEST_INFO(pool_test_alloc_packet_max_cache),
	ODP_TEST_INFO(pool_test_alloc_packet_subparam),
	ODP_TEST_INFO(pool_test_alloc_timeout),
	ODP_TEST_INFO(pool_test_alloc_timeout_min_cache),
	ODP_TEST_INFO(pool_test_alloc_timeout_max_cache),
	ODP_TEST_INFO(pool_test_info_packet),
	ODP_TEST_INFO(pool_test_lookup_info_print),
	ODP_TEST_INFO(pool_test_info_data_range),
	ODP_TEST_INFO(pool_test_buf_max_num),
	ODP_TEST_INFO(pool_test_pkt_max_num),
	ODP_TEST_INFO(pool_test_packet_vector_max_num),
	ODP_TEST_INFO(pool_test_pkt_seg_len),
	ODP_TEST_INFO(pool_test_tmo_max_num),
	ODP_TEST_INFO(pool_test_create_after_fork),
	ODP_TEST_INFO_NULL,
};

odp_suiteinfo_t pool_suites[] = {
	{ .name         = "Pool tests",
	  .testinfo_tbl = pool_suite,
	  .init_func    = pool_suite_init,
	},
	ODP_SUITE_INFO_NULL,
};

int main(int argc, char *argv[])
{
	int ret;

	/* parse common options: */
	if (odp_cunit_parse_options(argc, argv))
		return -1;

	ret = odp_cunit_register(pool_suites);

	if (ret == 0)
		ret = odp_cunit_run();

	return ret;
}
