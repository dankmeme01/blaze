#pragma once
#include "assert.hpp"

#ifdef BLAZE_DEBUG

# define BLAZE_IF_DEBUG(...) __VA_ARGS__
# define BLAZE_TRACE(...) log::debug(__VA_ARGS__)
# define BLAZE_TRACE_NOISY(...) log::trace(__VA_ARGS__)

#else

# define BLAZE_IF_DEBUG(...)
# define BLAZE_TRACE(...) do {} while (0)
# define BLAZE_TRACE_NOISY(...) do {} while (0)

#endif
