#include "MapImagesRequest.h"

namespace io {
    namespace input {
        MapImagesRequest::MapImagesRequest(const communication::MapImagesRequest &thrift) {
            if (thrift.__isset.acquiredVersion) {
                acquiredVersion = thrift.acquiredVersion;
            }
        }
    }
}