/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#if defined(__clang__) || defined(__GNUC__)
#define ACH_RESTRICT __restrict__
#define ACH_PREFETCH __builtin_prefetch
// MSVC support here later.
#endif
