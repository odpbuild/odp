/* Copyright (c) 2015-2018, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

/**
 * @file
 *
 * ODP pool
 */

#ifndef ODP_API_ABI_POOL_H_
#define ODP_API_ABI_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <odp/api/std_types.h>
#include <odp/api/plat/strong_types.h>
#include <odp/api/abi/event.h>

/** @ingroup odp_pool
 *  @{
 */

typedef ODP_HANDLE_T(odp_pool_t);

#define ODP_POOL_INVALID _odp_cast_scalar(odp_pool_t, 0)

#define ODP_POOL_NAME_LEN  32

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
