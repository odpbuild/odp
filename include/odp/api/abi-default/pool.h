/* Copyright (c) 2017-2018, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#ifndef ODP_ABI_POOL_H_
#define ODP_ABI_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <odp/api/abi/event.h>

/** @internal Dummy type for strong typing */
typedef struct { char dummy; /**< @internal Dummy */ } _odp_abi_pool_t;

/** @ingroup odp_pool
 *  @{
 */

typedef _odp_abi_pool_t *odp_pool_t;

#define ODP_POOL_INVALID   ((odp_pool_t)0)

#define ODP_POOL_NAME_LEN  32

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
