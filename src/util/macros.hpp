#pragma once

#ifdef BLAZE_DEBUG

# define BLAZE_IF_DEBUG(...) __VA_ARGS__
# define BLAZE_TRACE(...) log::debug(__VA_ARGS__)

#else

# define BLAZE_IF_DEBUG(...)
# define BLAZE_TRACE(...) do {} while (0)

#endif
