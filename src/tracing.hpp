#pragma once

#ifdef BLAZE_TRACY
# include <Tracy.hpp>
#else
# define ZoneScoped
# define ZoneScopedN(arg)
# define FrameMark
#endif
