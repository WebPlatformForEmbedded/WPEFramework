#include "ModeSet.h"

//TODO cplusplus
extern "C"
{
#include <fcntl.h>
#include <unistd.h>

#include <drm_fourcc.h>
}

#define DRM_MAX_DEVICES 16

struct ModeSet::FB ModeSet::FB = {nullptr, 0};

bool ModeSet::create()
{
    bool valid = false;

    std::vector<std::string> nodes = getnodes(static_cast<uint32_t>(DRM_NODE_PRIMARY));

    if(false != nodes.empty())
    {
        return false;
    }

    // Select the first from the list
    for (auto it = nodes.begin(), end = nodes.end(); it != end; it++)
    {
        if(true != it->empty())
        {
            // The node might be priviliged and the call will fail.
            _fd_ = open(it->c_str(), O_RDWR); // Do not close fd with exec functions! No O_CLOEXEC!

            break;
        }
    }

    if (-1 >= _fd_)
    {
        return valid;
    }

    // Clear nodes
    nodes.clear();

    // Only connected connectors are considered
    uint32_t connectors = getconnectors(static_cast<uint32_t>(DRM_MODE_CONNECTOR_HDMIA));

    // All CRTCs are considered for the given mode (valid / not valid)
    uint32_t crtcs = getcrtcs(true);

    drmModeResPtr resources = drmModeGetResources(_fd_);

    if(nullptr != resources)
    {

        for(int32_t i = 0; i < resources->count_connectors; i++)
        {

            if(static_cast<uint32_t>(1 << i) == (connectors & static_cast<uint32_t>(1 << i)))
            {
                drmModeConnectorPtr connector = drmModeGetConnector(_fd_, resources->connectors[i]);

                if(nullptr != connector)
                {

                    for(int32_t j = 0; j < connector->count_encoders; j++)
                    {
                        drmModeEncoderPtr encoder = drmModeGetEncoder(_fd_, connector->encoders[j]);

                        if(nullptr != encoder)
                        {

                            for(uint32_t matches = (encoder->possible_crtcs & crtcs), *pcrtc = resources->crtcs ; 0 < matches; matches = (matches >> 1), pcrtc++)
                            {

                                if (1 == (matches & 1))
                                {
                                    // A viable set found
                                    _crtc_ = *pcrtc;
// TODO : see uint32_t encoders = getencoders(static_cast<uint32_t>(DRM_MODE_ENCODER_NONE));
                                    _encoder_ = encoder->encoder_id;
                                    _connector_ = connector->connector_id;

                                    drmModeCrtcPtr pcrtc = drmModeGetCrtc(_fd_, _crtc_);
                                    if(nullptr != pcrtc)
                                    {
                                        valid = true;

                                        _fb_ = pcrtc->buffer_id;

                                        drmModeFreeCrtc(pcrtc);
                                    }

                                    i = resources->count_connectors;
                                    j = resources->count_encoders;

                                    break;
                                }

                            }

                            drmModeFreeEncoder(encoder);
                        }
                    }

                    drmModeFreeConnector(connector);
                }
            }
        }

        drmModeFreeResources(resources);
    }

    return (0 <= _fd_) && valid && InitializeScanOutBuffer();
}

bool ModeSet::destroy()
{
// TODO: Actual action depends on type INITIAL or CURRENT

    DeinitializeScanOutBuffer();

    _crtc_ = 0;
    _encoder_ = 0;
    _connector_ = 0;

    return 0 <= close(_fd_);
}

std::vector<std::string> ModeSet::getnodes(uint32_t type) const
{
    std::vector<std::string> list;

    drmDevicePtr devices[DRM_MAX_DEVICES];

    int device_count = drmGetDevices2(0 /* flags */, &devices[0], DRM_MAX_DEVICES);

    if (0 >= device_count)
    {
        return list;
    }

    for (int i = 0; i < device_count; i++)
    {
        switch (type)
        {
            case DRM_NODE_PRIMARY   :   // card<num>, always created, KMS, privileged
            case DRM_NODE_CONTROL   :   // ControlD<num>, currently unused
            case DRM_NODE_RENDER    :   // Solely for render clients, unprivileged
                                        {
                                            if ((1 << type) == (devices[i]->available_nodes & (1 << type)))
                                            {
                                                list.push_back( std::string(devices[i]->nodes[type]) );
                                            }

                                            break;
                                        }
            case DRM_NODE_MAX       :
            default                 :   // Unknown (new) node type
                                        ;
        }
    }

    drmFreeDevices(&devices[0], device_count);

    return list;
}

uint32_t ModeSet::getconnectors(uint32_t type) const
{
    uint32_t bitmask = 0;

    drmModeResPtr resources = drmModeGetResources(_fd_);

    if(nullptr != resources)
    {
        for (int i = 0; i < resources->count_connectors; i++)
        {
            drmModeConnectorPtr connector = drmModeGetConnector(_fd_, resources->connectors[i]);

            if(nullptr != connector)
            {
                if ((type == connector->connector_type) && (DRM_MODE_CONNECTED == connector->connection))
                {
                    bitmask = bitmask | (1 << i);
                }

                drmModeFreeConnector(connector);
            }
        }

        drmModeFreeResources(resources);
    }

    return bitmask;
}

uint32_t ModeSet::getencoders(uint32_t type) const
{
    uint32_t bitmask = 0;

    drmModeResPtr resources = drmModeGetResources(_fd_);

    if(nullptr != resources)
    {
        for(int i = 0; i < resources->count_encoders; i++)
        {
            drmModeEncoderPtr encoder = drmModeGetEncoder(_fd_, resources->encoders[i]);

            if(nullptr != encoder)
            {
                if((DRM_MODE_ENCODER_NONE == type) || (type == encoder->encoder_type))
                {
                    bitmask = bitmask | (1 << i);
                }
            }

            drmModeFreeEncoder(encoder);
        }

        drmModeFreeResources(resources);
    }

    return bitmask;
}

uint32_t ModeSet::getcrtcs(bool valid) const
{
    uint32_t bitmask = 0;

    drmModeResPtr resources = drmModeGetResources(_fd_);

    if(nullptr != resources)
    {
        for(int i = 0; i < resources->count_crtcs; i++)
        {
            drmModeCrtcPtr crtc = drmModeGetCrtc(_fd_, resources->crtcs[i]);

            if(nullptr != crtc)
            {
                if(valid == (1 == crtc->mode_valid ? true : false))
                {
                    bitmask = bitmask | (1 << i);
                }
            }

            drmModeFreeCrtc(crtc);
        }

        drmModeFreeResources(resources);
    }

    return bitmask;
}

bool ModeSet::InitializeScanOutBuffer()
{
// TODO: should check if a gbm_bo is present; then nothing is to be done because mode should have been set

    if(false != valid())
    {
        /*struct gbm_device**/_device_ = gbm_create_device(fd());

        if(nullptr != _device_)
        {
            drmModeConnectorPtr pconnector = drmModeGetConnector(fd(), connector());

            if(nullptr != pconnector)
            {
                uint64_t area[2] = {0, 0};
                uint32_t modeindex = 0;

                for(int32_t i = 0; i < pconnector->count_modes; i++)
                {
                    uint32_t type = pconnector->modes[i].type;

                    if(DRM_MODE_TYPE_DRIVER == (DRM_MODE_TYPE_DRIVER & type))
                    {
                        // Calculate screen area
                         area[1] = pconnector->modes[i].hdisplay * pconnector->modes[i].vdisplay;
                    }

                    // At least one preferred mode should be set by the driver, but dodgy EDID parsing might not provide it
                    if(DRM_MODE_TYPE_PREFERRED == (DRM_MODE_TYPE_PREFERRED & type))
                    {
                        modeindex = i;

                        // Found a suitable mode; break the loop
                        break;
                    }
                    else
                    {
                        if(area[0] < area[1])
                        {
                            modeindex = i;
                            area[0] = area[1];
                        }
                        else
                        {
                            if(area[0] == area[1])
                            {
                                // Use another selection criterium
                                // Select highest clock and vertical refresh rate

                                if(pconnector->modes[i].clock > pconnector->modes[modeindex].clock)
                                {
                                    modeindex = i;
                                }
                                else
                                {
                                    if(pconnector->modes[i].clock == pconnector->modes[modeindex].clock)
                                    {
                                        if(pconnector->modes[i].vrefresh > pconnector->modes[modeindex].vrefresh)
                                        {
                                            modeindex = i;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if(area[0] > area[1])
                                {
                                    // Nothing to do
                                }
                            }
                        }
                    }
                }

                // True if at least one mode is available

                uint32_t _width_ = pconnector->modes[modeindex].hdisplay;
                uint32_t _height_ = pconnector->modes[modeindex].vdisplay;

                /* uint32_t */ mode(modeindex, true);

                /* void */ drmModeFreeConnector(pconnector);

                // A large enough initial buffer for scan out
                struct gbm_bo* bo = gbm_bo_create(_device_, _width_, _height_, DRM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT /* presented on a screen */ | GBM_BO_USE_RENDERING /* used for rendering */);

                if(nullptr != bo)
                {
                    // Associate a frame buffer with this bo

                    int32_t fd = gbm_device_get_fd(_device_);

                    uint32_t stride = gbm_bo_get_stride(bo);
                    uint32_t height = gbm_bo_get_height(bo);
                    uint32_t width = gbm_bo_get_width(bo);
                    uint32_t handle = gbm_bo_get_handle(bo).u32;

                    uint32_t id = 0;

                    int32_t ret = drmModeAddFB(fd, width, height, 24, 32, stride, handle, &id);

                    if(0 == ret)
                    {
                        // The id is now different than the initial frame buffer if any existed, but that one is stored in _platform_[ModeSet::TYPE::INITIAL].fb()
                        id = fb(id, true);

                        _buffer_ = bo;

                        // Everything is now setup (correctly)
                    }
                }

            }

            return valid();
        }
    }

    return false;
}

bool ModeSet::DeinitializeScanOutBuffer()
{
//TODO: single display system assumed

    if(false != valid())
    {
        // Destroy the initial buffer if one exists
        if(nullptr != _buffer_)
        {
//TOOD: remove the FB associated with this bo
            /* void */ gbm_bo_destroy(static_cast<struct gbm_bo*>(static_cast<struct gbm_bo*>(_buffer_)));
        }

        if(nullptr != _device_)
        {
            /* void */ gbm_device_destroy(_device_);
        }

        _device_= nullptr;

        return (true != valid());
    }

    return false;
}

extern "C"{
void page_flip_handler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void* data)
{
    int *waiting_for_flip = reinterpret_cast<int*>(data);
    *waiting_for_flip = 0;
}
}
bool ModeSet::ScanOut(const struct gbm_surface* surface)
{
    // Global scope to understand exit strategy of the waiting loop
    bool  waiting_for_flip = true;

    if((surface != nullptr) && (valid() != false)) {
        // Do not forget to release buffers, otherwise memory pressure will increase

        // Buffer object representing our front buffer
        struct gbm_bo* bo = gbm_surface_lock_front_buffer(const_cast<struct gbm_surface*>(surface));

        if(bo != nullptr) {
            // Use the created bo buffer  to setup for a scan out

//            int32_t fd = gbm_device_get_fd(display);
//            fd should match modeset.fd

            uint32_t stride = gbm_bo_get_stride(bo);
            uint32_t height = gbm_bo_get_height(bo);
            uint32_t width = gbm_bo_get_width(bo);
            uint32_t handle = gbm_bo_get_handle(bo).u32;

            // Create an associated frame buffer (for each buffer) for scan out
            uint32_t id = 0;
            int32_t ret = drmModeAddFB(fd(), width, height, 24, 32, stride, handle, &id);

            if(ret == 0) {
                // A new FB and a new bo exist; time to make something visible

                if(int32_t ret = drmModePageFlip(fd(), crtc(), id, DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip) != 0) {
                    // Error

                    // Many causes, but the most obvious is a busy resource
                    // There is nothing to be done about it; notify the user and just continue
                }
                else {
                    // Use the magic constant here because the struct is versioned!
                    drmEventContext context = { .version = 2, . vblank_handler = nullptr, .page_flip_handler = page_flip_handler };

                    fd_set fds;

                    // Wait up to max 1 second
                    struct timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };

                    while(waiting_for_flip != false) {
                        FD_ZERO(&fds);
                        FD_SET(0, &fds);
                        FD_SET(fd(), &fds);

                        // Race free
                        ret = pselect(fd() + 1, &fds, nullptr, nullptr, &timeout, nullptr);

                        if(ret < 0) {
                            // Error; break the loop
                            break;
                        }
                        else {
                            if(ret == 0) {
                                // Timeout; retry
                            }
                            else { // ret > 0

                                if (FD_ISSET(0, &fds) != 0) {
                                    // User interrupted; break the loop if possible
                                    break;
                                }
                                else {
                                    if(FD_ISSET(fd(), &fds) != 0) {
                                        // Node is readable
                                        if(drmHandleEvent(fd(), &context) != 0) {
                                           // Error; break the loop

                                            break;
                                        }

                                       // Flip probably occured already otherwise it loops again
                                    }
                                }
                            }
                        }
                    }
                }

                // One FB should be released
                /* int */ drmModeRmFB(fd(), /* old id */ FB.fb);

                // One bo should be released
                if(nullptr != FB.bo)
                {
                    /* void */ gbm_surface_release_buffer(const_cast<struct gbm_surface*>(surface), FB.bo);
                }

                // Unconditionally; there is nothing to be done if the systems fail to remove the (frame) buffers
                FB.fb = id;
                FB.bo = bo;

                // id should match FB.id
                id = fb(id, true);
            }
            else {
                // Release only bo and do not update FB
                /* void */ gbm_surface_release_buffer(const_cast<struct gbm_surface*>(surface), bo);
            }
        }

        // The surface must have at least one buffer left free for rendering otherwise something has not been properly released / removed
        int count = gbm_surface_has_free_buffers(const_cast<struct gbm_surface*>(surface));

        // The value of waiting_for_flip provides a hint on the exit strategy applied
        return (true != waiting_for_flip) && (0 < count);
    }

    return false;
}

bool EnumeratedModeSets::Initialize()
{
    if(1 == drmAvailable())
    {
        // Both should be identical at tthis point

        // The application has to be master to be able to do any mode setting
        // - implicitly via the first open
        // - explicitly via drmSetMaster

        (*this)[ModeSet::TYPE::INITIAL];

        // Implicit open() involved!
        if((false != (*this)[ModeSet::TYPE::INITIAL].create()) && (false != (*this)[ModeSet::TYPE::INITIAL].valid()))
        {
            if(0 == drmSetMaster((*this)[ModeSet::TYPE::INITIAL].fd()))
            {
                (*this)[ModeSet::TYPE::CURRENT] = (*this)[ModeSet::TYPE::INITIAL];
            }
        }

        if( (*this)[ModeSet::TYPE::INITIAL].valid() && (*this)[ModeSet::TYPE::CURRENT].valid())
        {
            return EnableModeSet((*this)[ModeSet::TYPE::CURRENT]);
        }
    }

    return false;
}

bool EnumeratedModeSets::Deinitialize()
{
//TODO: what happens with the 'identical' file descriptor

    if( (*this)[ModeSet::TYPE::INITIAL].valid() && (*this)[ModeSet::TYPE::CURRENT].valid())
    {
        EnableModeSet((*this)[ModeSet::TYPE::INITIAL]);
    }

    // The order of destruction is important
    return (*this)[ModeSet::TYPE::CURRENT].destroy() && (*this)[ModeSet::TYPE::INITIAL].destroy();
}

bool EnumeratedModeSets::EnableModeSet(const ModeSet& set)
{
    bool ret = false;

    uint32_t ctor = set.connector();

    drmModeConnectorPtr ptr = drmModeGetConnector(set.fd(), ctor);

    if(nullptr != ptr)
    {
        uint32_t fd = set.fd();
        uint32_t crtc = set.crtc();
        uint32_t fb = const_cast<ModeSet&>(set).fb();
        uint32_t mode = const_cast<ModeSet&>(set).mode();

        int32_t ret = (0 == drmModeSetCrtc(fd, crtc, fb, 0, 0, &ctor, 1, &(ptr->modes[mode])));

        drmModeFreeConnector(ptr);
    }

    return ret;
}
