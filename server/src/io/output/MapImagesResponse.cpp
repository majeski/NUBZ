#include "MapImagesResponse.h"

namespace io {
    namespace output {
        communication::MapImagesResponse MapImagesResponse::toThrift() const {
            communication::MapImagesResponse ret;
            ret.levelImageUrls = levelImageUrls;
            ret.version = version;
            return ret;
        }
    }
}