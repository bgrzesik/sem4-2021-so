
#include "transport.h"


#if !defined(TRANSPORT_DGRAM) || defined(TRANSPORT_STREAM)
#error "Invalid macro defined"
#endif
